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
#include <signal.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "ivi-application-client-protocol.h"
#include "ivi-share-client-protocol.h"

struct window;

struct display {
    struct wl_display      *display;
    struct wl_registry     *registry;
    struct wl_compositor   *compositor;
    struct wl_seat         *seat;
    struct wl_touch        *touch;
    struct ivi_application *ivi_application;
    struct ivi_share       *ivi_share;

    struct {
        EGLDisplay egldisplay;
        EGLConfig  eglconfig;
        EGLContext eglcontext;
    } egl;

    struct window *window;
};

struct window {
    struct wl_surface        *surface;
    struct wl_egl_window     *egl_window;
    struct ivi_surface       *ivi_surface;
    struct ivi_share_surface *share_surface;

    EGLSurface eglsurface;

    uint32_t surface_id;

    struct {
        int width;
        int height;
    } geometry;

    struct {
        GLuint tex_id;
        GLuint pos;
        GLuint tex_pos;
    } gl;

    struct {
        uint32_t    share_surface_id;
        uint32_t    width;
        uint32_t    height;
        uint32_t    stride;
        uint32_t    name;
        uint32_t    input_caps;
        EGLImageKHR eglimage;
    } share_buffer;

    struct display *display;
};

static int running = 1;

/*----------------------------------------------------------------------------*/

static PFNEGLCREATEIMAGEKHRPROC            pfEglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC           pfEglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC pfGLEglImageTargetTexture2DOES;

static const char *vert_shader_text =
    "attribute vec4 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 texcoord_out;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    texcoord_out = texcoord;\n"
    "}\n";

static const char *frag_shader_text =
    "precision mediump float;\n"
    "varying vec2 texcoord_out;\n"
    "uniform mediump sampler2D tex_unit;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex_unit, texcoord_out);\n"
    "}\n";

static const GLfloat vertices[][2] = {
    { 1.0, -1.0},
    { 1.0,  1.0},
    {-1.0,  1.0},
    {-1.0, -1.0}
};

static const GLfloat tex_coords[][2] = {
    {1.0, 1.0},
    {1.0, 0.0},
    {0.0, 0.0},
    {0.0, 1.0}
};

/*----------------------------------------------------------------------------*/

static void
redraw(struct window *window, struct wl_callback *callback, uint32_t time)
{
    struct display *display = window->display;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, window->geometry.width, window->geometry.height);

    glEnableVertexAttribArray(window->gl.pos);
    glEnableVertexAttribArray(window->gl.tex_pos);

    glBindTexture(GL_TEXTURE_2D, window->gl.tex_id);

    if (window->share_buffer.eglimage != NULL) {
        pfGLEglImageTargetTexture2DOES(
            GL_TEXTURE_2D, window->share_buffer.eglimage);
    }

    glVertexAttribPointer(window->gl.pos,     2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(window->gl.tex_pos, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(window->gl.pos);
    glDisableVertexAttribArray(window->gl.tex_pos);

    eglSwapBuffers(display->egl.egldisplay, window->eglsurface);
}

static void
handle_share_surface_damage(void *data, struct ivi_share_surface *share_surface,
                            uint32_t name)
{
    struct window *window = data;
    struct display *display = window->display;
    EGLint img_attribs[] = {
        EGL_WIDTH,                  window->share_buffer.width,
        EGL_HEIGHT,                 window->share_buffer.height,
        EGL_DRM_BUFFER_STRIDE_MESA, window->share_buffer.stride,
        EGL_DRM_BUFFER_FORMAT_MESA, EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
        EGL_NONE
    };

    if (name == 0) {
        return;
    }

    if (window->share_buffer.name != name) {
        if (window->share_buffer.eglimage != NULL) {
            pfEglDestroyImageKHR(display->egl.egldisplay,
                                 window->share_buffer.eglimage);
        }

        window->share_buffer.eglimage =
            pfEglCreateImageKHR(display->egl.egldisplay, EGL_NO_CONTEXT,
                                EGL_DRM_BUFFER_MESA, (EGLClientBuffer)name,
                                img_attribs);
        if (window->share_buffer.eglimage == NULL) {
            fprintf(stderr, "failed to create EGLImageKHR: 0x%04x\n",
                eglGetError());
        }

        /* update */
        window->share_buffer.name = name;
    }
}

static void
handle_share_surface_configure(void *data, struct ivi_share_surface *share_surface,
                               uint32_t type, uint32_t width, uint32_t height,
                               uint32_t stride, uint32_t format)
{
    struct window *window = data;

    if ((width == 0) ||  (height == 0))
        return;

    if ((window->geometry.width  != width) ||
        (window->geometry.height != height)) {
        /* update window geometry */
        if (window->egl_window) {
            wl_egl_window_resize(window->egl_window, width, height, 0, 0);
        }
        window->geometry.width  = width;
        window->geometry.height = height;
    }

    window->share_buffer.name   = 0;
    window->share_buffer.width  = width;
    window->share_buffer.height = height;
    window->share_buffer.stride = stride;
}

static void
handle_share_surface_input_caps(void *data, struct ivi_share_surface *share_surface,
                                uint32_t caps)
{
    struct window *window = data;
    window->share_buffer.input_caps = caps;
}

static void
handle_share_surface_state(void *data, struct ivi_share_surface *share_surface,
                           uint32_t state)
{
    struct window *window = data;

    switch (state) {
    case IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_NOT_EXIST:
        fprintf(stderr, "Specified surface (ID:%d) does not exist\n",
                window->share_buffer.share_surface_id);
        break;
    case IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_INVALID_SURFACE:
        fprintf(stderr, "Specified surface (ID:%d) is invalid\n",
                window->share_buffer.share_surface_id);
        break;
    case IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_DESTROYED:
        fprintf(stderr, "Specified surface (ID:%d) was destroyed\n",
                window->share_buffer.share_surface_id);
        ivi_share_surface_destroy(window->share_surface);
        window->share_surface = NULL;
        break;
    }
}

static const struct ivi_share_surface_listener share_surface_listener = {
    handle_share_surface_damage,
    handle_share_surface_configure,
    handle_share_surface_input_caps,
    handle_share_surface_state
};

static int
get_share_surface(struct window *window, struct display *display)
{
    window->share_surface =
        ivi_share_get_ivi_share_surface(display->ivi_share,
                                        window->share_buffer.share_surface_id);

    ivi_share_surface_add_listener(window->share_surface,
                                   &share_surface_listener, window);
}

static GLuint
create_shader(const char *source, GLenum shader_type)
{
    GLuint shader;
    GLint status;

    shader = glCreateShader(shader_type);
    if (shader == 0) {
        fprintf(stderr, "failed to create shader\n");
        return 0;
    }

    glShaderSource(shader, 1, (const char**)&source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (0 == status) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        fprintf(stderr, "[ERR] compiling %s:\n%*s\n",
            shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment", len, log);
        return 0;
    }

    return shader;
}

static int
init_gl(struct window *window)
{
    GLuint vert, frag;
    GLuint program;
    GLint status;

    pfGLEglImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
        eglGetProcAddress("glEGLImageTargetTexture2DOES");
    pfEglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)
        eglGetProcAddress("eglCreateImageKHR");
    pfEglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)
        eglGetProcAddress("eglDestroyImageKHR");

    if (!pfGLEglImageTargetTexture2DOES &&
        !pfEglCreateImageKHR            &&
        !pfEglDestroyImageKHR) {
        fprintf(stderr, "GL and EGL extension APIs are not available\n");
        return -1;
    }

    vert = create_shader(vert_shader_text, GL_VERTEX_SHADER);
    frag = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);
    if (vert == 0 || frag == 0) {
        return -1;
    }

    program = glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (0 == status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        fprintf(stderr, "[ERR] linking:\n%s\n", log);
        return -1;
    }

    glUseProgram(program);

    window->gl.pos     = 0;
    window->gl.tex_pos = 1;
    glBindAttribLocation(program, window->gl.pos, "pos");
    glBindAttribLocation(program, window->gl.tex_pos, "tex_pos");
    glLinkProgram(program);

    glEnableVertexAttribArray(window->gl.pos);
    glEnableVertexAttribArray(window->gl.tex_pos);

    glGenTextures(1, &window->gl.tex_id);
    glBindTexture(GL_TEXTURE_2D, window->gl.tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

static void
handle_ivi_surface_configure(void *data, struct ivi_surface *ivi_surface,
                             int32_t width, int32_t height)
{
    /* no op */
}

static const struct ivi_surface_listener ivi_surface_listener = {
    handle_ivi_surface_configure
};

static void
create_ivi_surface(struct window *window, struct display *display)
{
    window->ivi_surface =
        ivi_application_surface_create(display->ivi_application,
                                       window->surface_id, window->surface);

    ivi_surface_add_listener(window->ivi_surface, &ivi_surface_listener,
                             window);
}

static int
create_window(struct window *window)
{
    struct display *display = window->display;
    EGLBoolean ret;

    window->surface = wl_compositor_create_surface(display->compositor);

    window->egl_window =
        wl_egl_window_create(window->surface,
                             window->geometry.width,
                             window->geometry.height);

    window->eglsurface =
        eglCreateWindowSurface(display->egl.egldisplay, display->egl.eglconfig,
                               window->egl_window, NULL);

    create_ivi_surface(window, display);

    ret = eglMakeCurrent(display->egl.egldisplay, window->eglsurface,
                         window->eglsurface, display->egl.eglcontext);
    if (ret != EGL_TRUE) {
        fprintf(stderr, "failed to make EGL context current\n");
        return -1;
    }

    if (init_gl(window) < 0) {
        return -1;
    }

    return 0;
}

static int
init_egl(struct display *display)
{
    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE,   1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE,  1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLint major, minor, n, count, i, size;

    display->egl.egldisplay = eglGetDisplay(display->display);

    if (!eglInitialize(display->egl.egldisplay, &major, &minor)) {
        fprintf(stderr, "failed to initialize EGL\n");
        return -1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf(stderr, "failed to bind EGL client API\n");
        return -1;
    }

    if (!eglChooseConfig(display->egl.egldisplay, config_attribs,
                         &display->egl.eglconfig, 1, &n)) {
        fprintf(stderr, "failed to choose EGL config\n");
        return -1;
    }

    if ((display->egl.eglcontext =
            eglCreateContext(display->egl.egldisplay, display->egl.eglconfig,
                             EGL_NO_CONTEXT, context_attribs)) == NULL) {
        fprintf(stderr, "failed to create EGL context\n");
        return -1;
    }

    return 0;
}

static void
touch_handle_down(void *data, struct wl_touch *touch, uint32_t serial,
                  uint32_t time, struct wl_surface *surface, int32_t id,
                  wl_fixed_t x, wl_fixed_t y)
{
    struct window *window = data;

    if ((window->share_surface != NULL) &&
        (window->share_buffer.input_caps & IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH)) {
        ivi_share_surface_redirect_touch_down(window->share_surface,
                                              serial, id, x, y);
    }
}

static void
touch_handle_up(void *data, struct wl_touch *touch, uint32_t serial,
                uint32_t time, int32_t id)
{
    struct window *window = data;

    if ((window->share_surface != NULL) &&
        (window->share_buffer.input_caps & IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH)) {
        ivi_share_surface_redirect_touch_up(window->share_surface,
                                            serial, id);
    }
}

static void
touch_handle_motion(void *data, struct wl_touch *touch, uint32_t time,
                    int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    struct window *window = data;

    if ((window->share_surface != NULL) &&
        (window->share_buffer.input_caps & IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH)) {
        ivi_share_surface_redirect_touch_motion(window->share_surface,
                                                id, x, y);
    }
}

static void
touch_handle_frame(void *data, struct wl_touch *touch)
{
    struct window *window = data;

    if ((window->share_surface != NULL) &&
        (window->share_buffer.input_caps & IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH)) {
        ivi_share_surface_redirect_touch_frame(window->share_surface);
    }
}

static void
touch_handle_cancel(void *data, struct wl_touch *touch)
{
    struct window *window = data;

    if ((window->share_surface != NULL) &&
        (window->share_buffer.input_caps & IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH)) {
        ivi_share_surface_redirect_touch_cancel(window->share_surface);
    }
}

static const struct wl_touch_listener touch_listener = {
    touch_handle_down,
    touch_handle_up,
    touch_handle_motion,
    touch_handle_frame,
    touch_handle_cancel
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
    struct display *display = data;

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !display->touch) {
        display->touch = wl_seat_get_touch(seat);
        wl_touch_add_listener(display->touch, &touch_listener,
                              display->window);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && display->touch) {
        wl_touch_destroy(display->touch);
        display->touch = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t name, const char *interface, uint32_t version)
{
    struct display *display = data;

    if (strcmp(interface, "wl_compositor") == 0) {
        display->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "ivi_application") == 0) {
        display->ivi_application =
            wl_registry_bind(registry, name, &ivi_application_interface, 1);
    } else if (strcmp(interface, "ivi_share") == 0) {
        display->ivi_share =
            wl_registry_bind(registry, name, &ivi_share_interface, 1);
    } else if (strcmp(interface, "wl_seat") == 0) {
        display->seat =
            wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(display->seat, &seat_listener, display);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
    /* no op */
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

static int
create_display(struct display *display)
{
    display->display = wl_display_connect(NULL);
    if (!display->display) {
        fprintf(stderr, "wayland display connect failed\n");
        return -1;
    }

    display->registry = wl_display_get_registry(display->display);

    wl_registry_add_listener(display->registry, &registry_listener, display);

    wl_display_dispatch(display->display);
    wl_display_roundtrip(display->display);

    if (init_egl(display) < 0) {
        return -1;
    }

    return 0;
}

static void
signal_handler(int signum)
{
    running = 0;
};

static void
destroy_window(struct window *window)
{
    eglMakeCurrent(window->display->egl.egldisplay,
                   EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglDestroySurface(window->display->egl.egldisplay, window->eglsurface);
    wl_egl_window_destroy(window->egl_window);

    if (window->share_surface)
        ivi_share_surface_destroy(window->share_surface);

    if (window->surface)
        wl_surface_destroy(window->surface);

    eglTerminate(window->display->egl.egldisplay);
    eglReleaseThread();
}

static void
destroy_display(struct display *display)
{
    if (display->ivi_share)
        ivi_share_destroy(display->ivi_share);

    if (display->ivi_application)
        ivi_application_destroy(display->ivi_application);

    if (display->compositor)
        wl_compositor_destroy(display->compositor);

    wl_registry_destroy(display->registry);
    wl_display_flush(display->display);
    wl_display_disconnect(display->display);
}

int
main(int argc, char **argv)
{
    struct display display;
    struct window window;
    uint32_t share_surface_id;
    int ret = 0;
    struct sigaction sigact;

    if (argc != 2) {
        fprintf(stderr, "usage: simple-ivi-share <surface ID to share buffer>\n");
        return 1;
    }

    memset(&display, 0x00, sizeof display);
    memset(&window, 0x00, sizeof window);

    /* initialize */
    display.window = &window;
    window.display = &display;
    window.surface_id = 1000;
    window.share_buffer.share_surface_id = atoi(argv[1]);
    window.geometry.width = 250;
    window.geometry.height = 250;

    if (create_display(&display) < 0) {
        return 1;
    }

    if (create_window(&window) < 0) {
        destroy_display(&display);
        return 1;
    }

    if (get_share_surface(&window, &display) < 0) {
        destroy_window(&window);
        destroy_display(&display);
        return 1;
    }

    /* signal settings */
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigact, NULL);

    /* run */
    while (running && ret != -1) {
        ret = wl_display_dispatch_pending(display.display);
        redraw(&window, NULL, 0);
    }

    printf("exit simple-ivi-share\n");
    destroy_window(&window);
    destroy_display(&display);

    return 0;
}
