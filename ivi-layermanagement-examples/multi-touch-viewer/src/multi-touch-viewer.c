/***************************************************************************
 *
 * Copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wayland-client-protocol.h>
#include "multi-touch-viewer.h"

#define WINDOW_TITLE "multi_touch_viewer"
#define WINDOW_WIDTH  1080
#define WINDOW_HEIGHT 1920
#define SURFACE_ID    4000
#define LAYER_ID      4000

#ifndef _UNUSED_
#    define _UNUSED_(x) (void)x
#endif

static const char *gp_vert_shader_text =
    "uniform mediump float uX;                     \n"
    "uniform mediump float uY;                     \n"
    "uniform mediump float uWidth;                 \n"
    "uniform mediump float uHeight;                \n"
    "uniform vec3 uColor;                          \n"
    "attribute vec4 pos;                           \n"
    "varying vec4 v_color;                         \n"
    "void main() {                                 \n"
    "    mediump vec4 position;                    \n"
    "    position.xy = vec2(uX + uWidth  * pos.x,  \n"
    "                       uY + uHeight * pos.y); \n"
    "    position.xy = 2.0 * position.xy - 1.0;    \n"
    "    position.zw = vec2(0.0, 1.0);             \n"
    "    gl_Position = position;                   \n"
    "    v_color = vec4(uColor, 1.0);              \n"
    "}                                             \n";

static const char *gp_frag_shader_text =
    "precision mediump float;        \n"
    "varying vec4 v_color;           \n"
    "void main() {                   \n"
    "    gl_FragColor = v_color;     \n"
    "}                               \n";

static struct touch_event_test_params *gp_test_params = NULL;

static int g_is_print_log = 0;

/******************************************************************************/

static void frame_callback(void *, struct wl_callback *, uint32_t);

static void
update_touch_point(struct touch_event_test_params *p_params, int ev,
                   float x, float y, int32_t id, uint32_t time)
{
    struct touch_point_params *p_point_params;
    struct event_log elog;

    if (NULL == p_params)
    {
        return;
    }

    wl_list_for_each(p_point_params, &p_params->touch_point_list, link)
    {
        if (p_point_params->id != id)
            continue;

        switch (ev) {
        case TOUCH_DOWN:
        case TOUCH_MOTION:
            p_point_params->display = 1;
            p_point_params->x = x;
            p_point_params->y = y;
            break;
        case TOUCH_UP:
            p_point_params->display = 0;
            break;
        default:
            break;
        }
        break;
    }

    if (g_is_print_log)
    {
        printf("[%d] %8.2f %8.2f (%s)\n", id, x, y,
               (ev == TOUCH_MOTION) ? "MOTION":
               (ev == TOUCH_DOWN)   ? "DOWN"  : "UP");
    }

    /* store event log */
    elog.event = ev;
    elog.x     = x;
    elog.y     = y;
    elog.id    = id;
    elog.time  = time;
    log_array_add(&p_params->log_array, &elog);
}

/******************************************************************************/

static void
frame_callback(void *p_data, struct wl_callback *p_cb, uint32_t time)
{
    _UNUSED_(time);

    WindowScheduleRedraw((struct WaylandEglWindow*)p_data);

    if (NULL != p_cb)
        wl_callback_destroy(p_cb);
}

static const struct wl_callback_listener frame_listener = {
    frame_callback
};

static void
touch_handle_down(void *p_data, struct wl_touch *p_touch, uint32_t serial,
                  uint32_t time, struct wl_surface *p_surface, int32_t id,
                  wl_fixed_t x_w, wl_fixed_t y_w)
{
    _UNUSED_(p_touch);
    _UNUSED_(serial);
    _UNUSED_(p_surface);

    update_touch_point((struct touch_event_test_params*)p_data,
                       TOUCH_DOWN,
                       (float)wl_fixed_to_double(x_w),
                       (float)wl_fixed_to_double(y_w),
                       id, time);
}

static void
touch_handle_up(void *p_data, struct wl_touch *p_touch, uint32_t serial,
                uint32_t time, int32_t id)
{
    _UNUSED_(p_touch);
    _UNUSED_(serial);

    update_touch_point((struct touch_event_test_params*)p_data,
                       TOUCH_UP,
                       0,
                       0,
                       id, time);
}

static void
touch_handle_motion(void *p_data, struct wl_touch *p_touch, uint32_t time,
                    int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    _UNUSED_(p_touch);

    update_touch_point((struct touch_event_test_params*)p_data,
                       TOUCH_MOTION,
                       (float)wl_fixed_to_double(x_w),
                       (float)wl_fixed_to_double(y_w),
                       id, time);
}

static void
touch_handle_frame(void *p_data, struct wl_touch *p_touch)
{
    _UNUSED_(p_data);
    _UNUSED_(p_touch);

    update_touch_point(NULL, TOUCH_FRAME, 0, 0, 0, 0);
}

static void
touch_handle_cancel(void *p_data, struct wl_touch *p_touch)
{
    _UNUSED_(p_data);
    _UNUSED_(p_touch);

    update_touch_point(NULL, TOUCH_CANCEL, 0, 0, 0, 0);
}

static const struct wl_touch_listener touch_listener = {
    touch_handle_down,
    touch_handle_up,
    touch_handle_motion,
    touch_handle_frame,
    touch_handle_cancel
};

static void
seat_capabilities(void *p_data, struct wl_seat *p_seat, uint32_t caps)
{
    struct touch_event_test_params *p_params =
        (struct touch_event_test_params*)p_data;

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !p_params->p_touch)
    {
        p_params->p_touch = wl_seat_get_touch(p_seat);
        wl_touch_add_listener(p_params->p_touch, &touch_listener, p_data);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && p_params->p_touch)
    {
        wl_touch_destroy(p_params->p_touch);
        p_params->p_touch = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_capabilities,
    NULL /* name: since version 2 */
};

static void
display_global_handler(struct WaylandDisplay *p_display, uint32_t name,
                       const char *p_interface, uint32_t version, void *p_data)
{
    _UNUSED_(version);
    struct touch_event_test_params *p_params =
        (struct touch_event_test_params*)p_data;

    if (0 == strcmp(p_interface, "wl_seat"))
    {
        p_params->p_seat = wl_registry_bind(p_display->p_registry,
                                            name, &wl_seat_interface, 1);
        wl_seat_add_listener(p_params->p_seat, &seat_listener, p_data);
    }
}

/******************************************************************************/

static void
draw_grid(int direction, GLfloat width, GLfloat height, float step,
          struct touch_event_test_params *p_params)
{
    GLfloat coord       = step;
    GLfloat uX          = 0.0f;
    GLfloat uY          = 0.0f;
    GLfloat dest_width  = 0.0f;
    GLfloat dest_height = 0.0f;
    GLfloat dest_coord;

    GLfloat vertices[2][2] = {
        {0.0f, 0.0f},
        {0.0f, 0.0f}
    };
#if 0
    GLfloat line_color[3] = {0.25f, 0.28f, 0.80f};
#else
    GLfloat line_color[3] = {0.1, 0.1, 0.1};
#endif

    switch (direction)
    {
    case 0: /* verticality   */
        vertices[1][1] = 1.0f;
        dest_coord = width;
        break;
    case 1: /* horizontality */
        vertices[1][0] = 1.0f;
        dest_coord = height;
        break;
    default:
        return;
    }

    glLineWidth(2.0f);

    coord = step;
    while (coord < dest_coord)
    {
        if (direction == 0)
        {
            uX          = coord / width;
            dest_height = height;
        }
        else
        {
            uY         = 1.0f - coord / height;
            dest_width = width;
        }

        glUniform1fv(p_params->gl.loc_x, 1, &uX);
        glUniform1fv(p_params->gl.loc_y, 1, &uY);
        glUniform1fv(p_params->gl.loc_w, 1, &dest_width);
        glUniform1fv(p_params->gl.loc_h, 1, &dest_height);
        glUniform3fv(p_params->gl.loc_col, 1, line_color);

        glVertexAttribPointer(p_params->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);

        glDrawArrays(GL_LINES, 0, 2);

        coord += step;
    }
}

static void
redraw_handler(struct WaylandEglWindow *p_window, void *p_data)
{
    GLfloat w, h, uX, uY;
    GLfloat view_width, view_height;
    GLfloat dest_width, dest_height;
    struct touch_event_test_params *p_params = (struct touch_event_test_params*)p_data;
    struct touch_point_params *p_point_params;

    DisplayAcquireWindowSurface(p_window->p_display, p_window);

    view_width  = p_window->geometry.width;
    view_height = p_window->geometry.height;

    glViewport(0, 0, (int)view_width, (int)view_height);

#if 0
    glClearColor(0.60, 0.84, 0.91, 1.0);
#else
    glClearColor(0.7, 0.7, 0.7, 1.0);
#endif
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableVertexAttribArray(p_params->gl.pos);

    /* readraw touch areas */
    draw_grid(0, view_width, view_height, 100.0f, p_params);
    draw_grid(1, view_width, view_height, 100.0f, p_params);

    /* redraw pointer circles */
    wl_list_for_each(p_point_params, &p_params->touch_point_list, link)
    {
        if (0 == p_point_params->display)
            continue;

        w = h = p_point_params->r * 2.0;

        uX = (p_point_params->x - p_point_params->r)/ view_width;
        uY = 1.0f - (p_point_params->y + p_point_params->r) / view_height;

        dest_width  = w / view_width;
        dest_height = h / view_height;

        glUniform1fv(p_params->gl.loc_x, 1, &uX);
        glUniform1fv(p_params->gl.loc_y, 1, &uY);
        glUniform1fv(p_params->gl.loc_w, 1, &dest_width);
        glUniform1fv(p_params->gl.loc_h, 1, &dest_height);
        glUniform3fv(p_params->gl.loc_col, 1, p_point_params->color);

        glVertexAttribPointer(p_params->gl.pos, 2,
            GL_FLOAT, GL_FALSE, 0, p_point_params->p_vertices);

        glDrawArrays(GL_TRIANGLE_FAN, 0, p_point_params->n_vtx);
    }

    glDisableVertexAttribArray(p_params->gl.pos);

    struct wl_callback *p_cb = wl_surface_frame(p_window->p_surface);
    wl_callback_add_listener(p_cb, &frame_listener, p_window);
}

/******************************************************************************/

static GLuint
create_shader(const char *p_source, GLenum shader_type)
{
    GLuint shader;
    GLint status;

    shader = glCreateShader(shader_type);
    assert(shader != 0);

    glShaderSource(shader, 1, (const char**)&p_source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (0 == status)
    {
        char gllog[1000];
        GLsizei len;
        glGetShaderInfoLog(shader, sizeof(gllog), &len, gllog);
        LOG_ERROR("Shader Compiling Failed (%s):\n%*s\n",
            shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment", len, gllog);
        exit(-1);
    }

    return shader;
}

static void
init_gl(struct touch_event_test_params *p_params)
{
    GLuint vert, frag;
    GLuint program;
    GLint status;

    vert = create_shader(gp_vert_shader_text, GL_VERTEX_SHADER);
    frag = create_shader(gp_frag_shader_text, GL_FRAGMENT_SHADER);

    program = glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (0 == status)
    {
        char gllog[1000];
        GLsizei len;
        glGetProgramInfoLog(program, sizeof(gllog), &len, gllog);
        LOG_ERROR("Shader Linking Failed:\n%*s\n", len, gllog);
        exit(-1);
    }

    glUseProgram(program);

    p_params->gl.pos = 0;

    glBindAttribLocation(program, p_params->gl.pos, "pos");

    p_params->gl.loc_x   = glGetUniformLocation(program, "uX");
    p_params->gl.loc_y   = glGetUniformLocation(program, "uY");
    p_params->gl.loc_w   = glGetUniformLocation(program, "uWidth");
    p_params->gl.loc_h   = glGetUniformLocation(program, "uHeight");
    p_params->gl.loc_col = glGetUniformLocation(program, "uColor");
}

#define NUM_OF_TOUCH_POINT_COLOR 5

static int
setup_point_params(struct touch_event_test_params *p_params)
{
    struct touch_point_params *p_point_params;
    int i, j;
    int n_vtx = 13;
    float rad = 0.0;
    float cx  = 0.5;
    float cy  = 0.5;
    float step = (float)(M_PI / 6.0);
    GLfloat colors[NUM_OF_TOUCH_POINT_COLOR][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}
    };

    for (i = 0; i < NUM_OF_TOUCH_POINT_COLOR; ++i)
    {
        p_point_params =
            (struct touch_point_params*)allocate(sizeof *p_point_params, 1);
        if (NULL == p_point_params)
        {
            LOG_ERROR("Memory allocation failed\n");
            return -1;
        }

        p_point_params->display = 0;
        p_point_params->id = i;
        p_point_params->x  = 0.0;
        p_point_params->y  = 0.0;
        p_point_params->r  = 15.0;
        p_point_params->n_vtx = n_vtx;
        p_point_params->p_vertices = (GLfloat*)allocate(sizeof(GLfloat) * n_vtx * 2, 0);
        assert(NULL != p_point_params->p_vertices);

        for (j = 0; j < (n_vtx - 1); ++j)
        {
            p_point_params->p_vertices[j*2  ] = cx + (float)(0.5 * cos(rad));
            p_point_params->p_vertices[j*2+1] = cy + (float)(0.5 * sin(rad));
            rad += step;
        }
        p_point_params->p_vertices[j*2  ] = p_point_params->p_vertices[0];
        p_point_params->p_vertices[j*2+1] = p_point_params->p_vertices[1];

        int idx = (i % NUM_OF_TOUCH_POINT_COLOR);
        memcpy(p_point_params->color, colors[idx], sizeof(GLfloat) * 3);

        wl_list_insert(p_params->touch_point_list.prev, &p_point_params->link);
    }

    return 0;
}

static int
setup_touch_event_test(struct touch_event_test_params *p_params)
{
    if (0 > setup_point_params(p_params))
    {
        return -1;
    }

    init_gl(p_params);
    return 0;
}

/******************************************************************************/

static void
signal_handler(int signum)
{
    switch (signum) {
    case SIGINT:
        LOG_INFO("Caught SIGINT - Interrupt\n");
        LOG_INFO("Recieved touch event information:\n");
        {
            int n_down = 0, n_up = 0, n_motion = 0;
            size_t i;
            for (i = 0; i < gp_test_params->log_array.n_log; ++i)
            {
                switch (gp_test_params->log_array.p_logs[i].event)
                {
                case TOUCH_DOWN:
                    ++n_down;
                    break;
                case TOUCH_MOTION:
                    ++n_motion;
                    break;
                case TOUCH_UP:
                    ++n_up;
                    break;
                }
            }
            LOG_INFO(" - DOWN   event  %d\n", n_down);
            LOG_INFO(" - MOTION event  %d\n", n_motion);
            LOG_INFO(" - UP     event  %d\n", n_up);
        }
        break;
    default:
        LOG_INFO("Caught unknown signal(%d), exit\n", signum);
        break;
    }

    DisplayExit(gp_test_params->p_display);
}

static void
setup_signal()
{
    struct sigaction sigact;

    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
}

/******************************************************************************/

int
touch_event_test_main(struct touch_event_test_params *p_params)
{
    struct WaylandDisplay   *p_display;
    struct WaylandEglWindow *p_window;

    setup_signal();

    p_display = CreateDisplay(0, NULL);
    if (NULL == p_display)
    {
        LOG_ERROR("Failed to create display\n");
        return -1;
    }
    p_params->p_display = p_display;

    DisplaySetUserData(p_display, p_params);
    DisplaySetGlobalHandler(p_display, display_global_handler);

    p_window = CreateEglWindow(p_display, WINDOW_TITLE, SURFACE_ID);
    if (NULL == p_window)
    {
        LOG_ERROR("Failed to create EGL window\n");
        DestroyDisplay(p_display);
        return -1;
    }
    p_params->p_window = p_window;

    /**
     * TODO: Create setter function to window.c
     */
    p_window->redraw_handler = redraw_handler;
    p_window->p_user_data = p_params;

    WindowScheduleResize(p_window, WINDOW_WIDTH, WINDOW_HEIGHT);
    WindowCreateSurface(p_window);

    if (0 > setup_touch_event_test(p_params))
    {
        LOG_ERROR("Failed to setup this viewer\n");
        DestroyDisplay(p_display);
        return -1;
    }

    DisplayRun(p_display);

    /* Print event information */
    LOG_INFO("Number of events: %d\n", p_params->log_array.n_log);

    log_array_release(&p_params->log_array);

    return 0;
}

void
usage(int status)
{
    printf("usage: multi-touch-viewer [OPTION]\n");
    printf("  -p : print received touch point\n");
    exit(status);
}

int
main(int argc, char **argv)
{
    _UNUSED_(argc);
    _UNUSED_(argv);
    struct touch_event_test_params params;

    memset(&params, 0x00, sizeof params);

    if (argc == 2)
    {
        if (0 == strcmp(argv[1], "-p"))
        {
            g_is_print_log = 1;
        }
        else
        {
            usage(EXIT_SUCCESS);
        }
    }

    gp_test_params = &params;

    wl_list_init(&params.touch_point_list);

    log_array_init(&params.log_array, 500);

    return touch_event_test_main(&params);
}
