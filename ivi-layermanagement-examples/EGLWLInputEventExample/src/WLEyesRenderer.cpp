/**************************************************************************
 *
 * Copyright 2010, 2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "WLEGLSurface.h"
#include "WLEyes.h"
#include "WLEyesRenderer.h"
#include <poll.h>

#define WL_UNUSED(A) (A)=(A)

extern int gNeedRedraw;
extern int gPointerX;
extern int gPointerY;

const struct wl_pointer_listener PointerListener = {
    PointerHandleEnter,
    PointerHandleLeave,
    PointerHandleMotion,
    PointerHandleButton,
    PointerHandleAxis
};

const struct wl_keyboard_listener KeyboardListener = {
    KeyboardHandleKeymap,
    KeyboardHandleEnter,
    KeyboardHandleLeave,
    KeyboardHandleKey,
    KeyboardHandleModifiers,
};

const struct wl_touch_listener TouchListener = {
    TouchHandleDown,
    TouchHandleUp,
    TouchHandleMotion,
    TouchHandleFrame,
    TouchHandleCancel,
};

void WaitForEvent(struct wl_display* wlDisplay, int fd)
{
    int err;

    /* Execute every pending event in the display and stop any other thread from placing
    * events into this display */
    while (wl_display_prepare_read(wlDisplay) != 0) {
        wl_display_dispatch_pending(wlDisplay);
    }

    if (wl_display_flush(wlDisplay) == -1)
    {
        err = wl_display_get_error(wlDisplay);
        printf("Error communicating with wayland: %d",err);
        return;
    }

    /*Wait till an event occurs */
    struct pollfd pfd[1];
    pfd[0].fd = fd;
    pfd[0].events = POLLIN;
    int pollret = poll(pfd, 1, -1);

    if (pollret != -1 && (pfd[0].revents & POLLIN))
    {
        /* Read the upcoming events from the file descriptor and execute them */
        wl_display_read_events(wlDisplay);
        wl_display_dispatch_pending(wlDisplay);
    }
    else {
        /*Unblock other threads, if an error happens */
        wl_display_cancel_read(wlDisplay);
    }
}

//////////////////////////////////////////////////////////////////////////////

void
PointerHandleEnter(void* data, struct wl_pointer* wlPointer, uint32_t serial,
                   struct wl_surface* wlSurface, wl_fixed_t sx, wl_fixed_t sy)
{
    WL_UNUSED(data);
    WL_UNUSED(wlPointer);
    WL_UNUSED(serial);
    WL_UNUSED(wlSurface);
    WL_UNUSED(sx);
    WL_UNUSED(sy);
    printf("ENTER EGLWLINPUT PointerHandleEnter: x(%d), y(%d)\n", sx, sy);
}

void
PointerHandleLeave(void* data, struct wl_pointer* wlPointer, uint32_t serial,
                   struct wl_surface* wlSurface)
{
    WL_UNUSED(data);
    WL_UNUSED(wlPointer);
    WL_UNUSED(serial);
    WL_UNUSED(wlSurface);
    printf("ENTER EGLWLINPUT PointerHandleLeave: serial(%d)\n", serial);
}

void
PointerHandleMotion(void* data, struct wl_pointer* wlPointer, uint32_t time,
                    wl_fixed_t sx, wl_fixed_t sy)
{
    WL_UNUSED(data);
    WL_UNUSED(wlPointer);
    WL_UNUSED(time);
    gPointerX = (int)wl_fixed_to_double(sx);
    gPointerY = (int)wl_fixed_to_double(sy);
    printf("ENTER EGLWLINPUT PointerHandleMotion: x(%d), y(%d)\n", gPointerX, gPointerY);
    gNeedRedraw = 1;
}

void
PointerHandleButton(void* data, struct wl_pointer* wlPointer, uint32_t serial,
                    uint32_t time, uint32_t button, uint32_t state)
{
    WL_UNUSED(data);
    WL_UNUSED(wlPointer);
    WL_UNUSED(serial);
    WL_UNUSED(time);
    WL_UNUSED(button);
    WL_UNUSED(state);
    printf("ENTER EGLWLINPUT PointerHandleButton: button(%d), state(%d)\n", button, state);
}

void
PointerHandleAxis(void* data, struct wl_pointer* wlPointer, uint32_t time,
                  uint32_t axis, wl_fixed_t value)
{
    WL_UNUSED(data);
    WL_UNUSED(wlPointer);
    WL_UNUSED(time);
    WL_UNUSED(axis);
    WL_UNUSED(value);
    printf("ENTER EGLWLINPUT PointerHandleAxis: axis(%d), value(%d)\n", axis, wl_fixed_to_int(value));
}

//////////////////////////////////////////////////////////////////////////////

void
KeyboardHandleKeymap(void* data, struct wl_keyboard* keyboard,
                     uint32_t format, int fd, uint32_t size)
{
    WL_UNUSED(data);
    WL_UNUSED(keyboard);
    WL_UNUSED(format);
    WL_UNUSED(fd);
    WL_UNUSED(size);
    printf("ENTER EGLWLINPUT KeyboardHandleKeymap: format(%d), fd(%d), size(%d)\n",
        format, fd, size);
}

void
KeyboardHandleEnter(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                    struct wl_surface* surface, struct wl_array* keys)
{
    WL_UNUSED(data);
    WL_UNUSED(keyboard);
    WL_UNUSED(serial);
    WL_UNUSED(surface);
    WL_UNUSED(keys);
    printf("ENTER EGLWLINPUT KeyboardHandleEnter: serial(%d), surface(%p)\n",
        serial, surface);
}

void
KeyboardHandleLeave(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                    struct wl_surface* surface)
{
    WL_UNUSED(data);
    WL_UNUSED(keyboard);
    WL_UNUSED(serial);
    WL_UNUSED(surface);
    printf("ENTER EGLWLINPUT KeyboardHandleLeave: serial(%d), surface(%p)\n",
        serial, surface);
}

void
KeyboardHandleKey(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                  uint32_t time, uint32_t key, uint32_t state_w)
{
    WL_UNUSED(data);
    WL_UNUSED(keyboard);
    WL_UNUSED(serial);
    WL_UNUSED(time);
    WL_UNUSED(key);
    WL_UNUSED(state_w);
    printf("ENTER EGLWLINPUT KeyboardHandleKey: serial(%d), time(%d), key(%d), state_w(%d)\n",
        serial, time, key, state_w);
}

void
KeyboardHandleModifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                        uint32_t mods_depressed, uint32_t mods_latched,
                        uint32_t mods_locked, uint32_t group)
{
    WL_UNUSED(data);
    WL_UNUSED(keyboard);
    WL_UNUSED(serial);
    WL_UNUSED(mods_depressed);
    WL_UNUSED(mods_latched);
    WL_UNUSED(mods_locked);
    WL_UNUSED(group);
    printf("ENTER EGLWLINPUT KeyboardHandleModifiers: serial(%d), mods_depressed(%d)\n"
          "                               mods_latched(%d), mods_locked(%d)\n"
           "                               group(%d)\n",
        serial, mods_depressed, mods_latched, mods_locked, group);
}

//////////////////////////////////////////////////////////////////////////////

void
TouchHandleDown(void* data, struct wl_touch* touch, uint32_t serial, uint32_t time,
                struct wl_surface* surface, int32_t id, wl_fixed_t xw, wl_fixed_t yw)
{
    WL_UNUSED(data);
    WL_UNUSED(touch);
    WL_UNUSED(serial);
    WL_UNUSED(time);
    WL_UNUSED(surface);
    WL_UNUSED(id);
    WL_UNUSED(xw);
    WL_UNUSED(yw);
    gPointerX = (int)wl_fixed_to_double(xw);
    gPointerY = (int)wl_fixed_to_double(yw);
    gNeedRedraw = 1;
    printf("ENTER EGLWLINPUT TouchHandleDown id: %d, x: %d,y: %d \n",id,gPointerX,gPointerY);
}

void
TouchHandleUp(void* data, struct wl_touch* touch, uint32_t serial, uint32_t time, int32_t id)
{
    WL_UNUSED(data);
    WL_UNUSED(touch);
    WL_UNUSED(serial);
    WL_UNUSED(time);
    WL_UNUSED(id);
    printf("ENTER EGLWLINPUT TouchHandleUp\n");
}

void
TouchHandleMotion(void* data, struct wl_touch* touch, uint32_t time, int32_t id,
                  wl_fixed_t xw, wl_fixed_t yw)
{
    WL_UNUSED(data);
    WL_UNUSED(touch);
    WL_UNUSED(time);
    WL_UNUSED(id);
    WL_UNUSED(xw);
    WL_UNUSED(yw);
    gPointerX = (int)wl_fixed_to_double(xw);
    gPointerY = (int)wl_fixed_to_double(yw);
    printf("ENTER EGLWLINPUT TouchHandleMotion id: %d, x: %d,y: %d \n",id,gPointerX,gPointerY);
    gNeedRedraw = 1;
}

void
TouchHandleFrame(void* data, struct wl_touch* touch)
{
    WL_UNUSED(data);
    WL_UNUSED(touch);
}

void
TouchHandleCancel(void* data, struct wl_touch* touch)
{
    WL_UNUSED(data);
    WL_UNUSED(touch);
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _es2shaderobject {
    GLuint fragmentShaderId;
    GLuint vertexShaderId;
    GLuint shaderProgramId;
    GLint  matrixLocation;
    GLint  colorLocation;
} ES2ShaderObject;

typedef struct _es2vertexbufferobject {
    GLuint vbo;
} ES2VertexBuffer;

static ES2ShaderObject gShaderObject;
static ES2VertexBuffer gVertexBuffer;

// Fragment shader code
const char* sourceFragShader = "  \
    uniform mediump vec4 u_color; \
    void main(void)               \
    {                             \
        gl_FragColor = u_color;   \
    }";

// Vertex shader code
const char* sourceVertexShader = " \
    attribute highp vec4 a_vertex; \
    uniform mediump mat4 u_matrix; \
    void main(void)                \
    {                              \
        gl_Position = u_matrix * a_vertex; \
    }";

//////////////////////////////////////////////////////////////////////////////

bool
InitRenderer()
{
    glViewport(0, 0, 400, 240);

    if (!InitShader()){
        return false;
    }

    if (!InitVertexBuffer()){
        return false;
    }

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    return true;
}

void
TerminateRenderer()
{
    glDeleteProgram(gShaderObject.shaderProgramId);
    glDeleteShader(gShaderObject.fragmentShaderId);
    glDeleteShader(gShaderObject.vertexShaderId);

    glDeleteBuffers(1, &gVertexBuffer.vbo);
}

char*
BuildShaderErrorLog(GLuint id)
{
    int l, nChar;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &l);

    char* info = (char*)malloc(sizeof(char) * l);
    glGetShaderInfoLog(id, l, &nChar, info);

    return info;
}

bool
InitShader()
{
    GLint result = 0;
    char* log = NULL;

    // Create the fragment shader object
    gShaderObject.fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gShaderObject.fragmentShaderId, 1,
        (const char**)&sourceFragShader, NULL);
    glCompileShader(gShaderObject.fragmentShaderId);

    glGetShaderiv(gShaderObject.fragmentShaderId, GL_COMPILE_STATUS, &result);
    if (!result){
        log = BuildShaderErrorLog(gShaderObject.fragmentShaderId);
        fprintf(stderr, "Failed to compile fragment shader: %s\n", log);
        if (log) free(log);
        return false;
    }

    // Create the vertex shader object
    gShaderObject.vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gShaderObject.vertexShaderId, 1,
        (const char**)&sourceVertexShader, NULL);
    glCompileShader(gShaderObject.vertexShaderId);

    glGetShaderiv(gShaderObject.vertexShaderId, GL_COMPILE_STATUS, &result);
    if (!result){
        log = BuildShaderErrorLog(gShaderObject.vertexShaderId);
        fprintf(stderr, "Failed to compile fragment shader: %s\n", log);
        if (log) free(log);
        return false;
    }

    gShaderObject.shaderProgramId = glCreateProgram();

    glAttachShader(gShaderObject.shaderProgramId, gShaderObject.fragmentShaderId);
    glAttachShader(gShaderObject.shaderProgramId, gShaderObject.vertexShaderId);

    glBindAttribLocation(gShaderObject.shaderProgramId, 0, "a_vertex");

    glLinkProgram(gShaderObject.shaderProgramId);

    glGetProgramiv(gShaderObject.shaderProgramId, GL_LINK_STATUS, &result);
    if (!result){
        log = BuildShaderErrorLog(gShaderObject.shaderProgramId);
        fprintf(stderr, "Failed to compile fragment shader: %s\n", log);
        if (log) free(log);
        return false;
    }

    glUseProgram(gShaderObject.shaderProgramId);
    gShaderObject.matrixLocation = glGetUniformLocation(
        gShaderObject.shaderProgramId, "u_matrix");
    gShaderObject.colorLocation = glGetUniformLocation(
        gShaderObject.shaderProgramId, "u_color");

    return true;
}

bool
InitVertexBuffer()
{
    glGenBuffers(1, &gVertexBuffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer.vbo);

    unsigned int uiSize = 100 * (sizeof(GLfloat) * 2);
    glBufferData(GL_ARRAY_BUFFER, uiSize, NULL, GL_STATIC_DRAW);

    return true;
}

void
AttachVertexBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void
DetachVertexBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void FrameListenerFunc(void*, struct wl_callback* cb, uint32_t)
{
    if (cb){
        wl_callback_destroy(cb);
    }
}

static const struct wl_callback_listener FrameListener = {
    FrameListenerFunc,
};

void
DrawFillPoly(const int nPoint, const float* points, const float color[4])
{
    glUniform4fv(gShaderObject.colorLocation, 1, color);

    unsigned int uiSize = nPoint * (sizeof(GLfloat) * 2);
    glBufferSubData(GL_ARRAY_BUFFER, 0, uiSize, points);

    glDrawArrays(GL_TRIANGLE_FAN, 0, nPoint);
}

void
DrawPoly(const int nPoint, const float* points, const float color[4], int width)
{
    glLineWidth(width);

    glUniform4fv(gShaderObject.colorLocation, 1, color);

    unsigned int uiSize = nPoint * (sizeof(GLfloat) * 2);
    glBufferSubData(GL_ARRAY_BUFFER, 0, uiSize, points);

    glDrawArrays(GL_LINE_STRIP, 0, nPoint);
}

bool
DrawEyes(WLEGLSurface* surface, WLEyes* eyes)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gShaderObject.shaderProgramId);

    AttachVertexBuffer();
    {
        float a = 2.0 / 400.0;
        float b = 2.0 / 240.0;
        float mtx[16] = {   a,  0.0, 0.0, 0.0,
                          0.0,    b, 0.0, 0.0,
                          0.0,  0.0, 1.0, 0.0,
                         -1.0, -1.0, 0.0, 1.0};

        glUniformMatrix4fv(gShaderObject.matrixLocation, 1, GL_FALSE, mtx);

        eyes->SetPointOfView(gPointerX, gPointerY);

        float black[] = {0.0, 0.0, 0.0, 1.0};
        float white[] = {1.0, 1.0, 1.0, 1.0};
        int nPoint = 0;
        float* points = NULL;
        for (int ii = 0; ii < 2; ++ii){
            // eye liner
            eyes->GetEyeLinerGeom(ii, &nPoint, &points);
            if (nPoint <= 0 || points == NULL){
                continue;
            }
            DrawFillPoly(nPoint, points, black);

            // eye shape (white eye)
            eyes->GetWhiteEyeGeom(ii, &nPoint, &points);
            if (nPoint <= 0 || points == NULL){
                continue;
            }
            DrawFillPoly(nPoint, points, white);

            // pupil
            eyes->GetPupilGeom(ii, &nPoint, &points);
            if (nPoint <= 0 || points == NULL){
                continue;
            }
            DrawFillPoly(nPoint, points, black);
            /*same array is used for both eyes, execute finish for each eye*/
            glFinish();
        }
    }
    DetachVertexBuffer();
    eglSwapBuffers(surface->GetEGLDisplay(), surface->GetEGLSurface());

    struct wl_callback* cb = wl_surface_frame(surface->GetWLSurface());
    wl_callback_add_listener(cb, &FrameListener, NULL);
    wl_surface_commit(surface->GetWLSurface());
    wl_display_flush(surface->GetWLDisplay());

    return true;
}
