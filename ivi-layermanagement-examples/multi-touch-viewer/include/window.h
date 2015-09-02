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
#ifndef WINDOW_H
#define WINDOW_H

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "ivi-application-client-protocol.h"

#define _UNUSED_(arg) (void)arg;

struct WaylandDisplay;
struct WaylandEglWindow;

typedef void (*window_redraw_handler_t)
    (struct WaylandEglWindow *p_window, void *p_data);

typedef void (*display_global_handler_t)
    (struct WaylandDisplay *p_display, uint32_t name, const char *p_interface,
     uint32_t version, void *p_data);

struct Task
{
    void (*run)(struct Task *p_task, uint32_t events);
    struct wl_list link;
};

struct WaylandDisplay
{
    struct Task           display_task;
    struct wl_display    *p_display;
    struct wl_registry   *p_registry;
    struct wl_compositor *p_compositor;
    struct wl_shell      *p_shell;
    struct ivi_application *p_ivi_application;
    EGLDisplay            egldisplay;
    EGLConfig             eglconfig;
    EGLContext            eglcontext;

    int                   running;
    int                   epoll_fd;
    int                   display_fd;
    uint32_t              display_fd_events;

    display_global_handler_t global_handler;

    struct wl_list        global_list;
    struct wl_list        surface_list;
    struct wl_list        deferred_list;

    void                 *p_user_data;
};

struct WaylandEglWindow
{
    struct Task              redraw_task;
    struct wl_list           link;
    struct WaylandDisplay   *p_display;
    struct wl_surface       *p_surface;
    struct wl_shell_surface *p_shell_surface;
    struct ivi_surface      *p_ivi_surface;
    struct wl_egl_window    *p_egl_window;
    struct wl_callback      *p_frame_cb;
    EGLSurface               eglsurface;
    struct {
        GLuint rotation_uniform;
        GLuint pos;
        GLuint col;
    } gl;
    struct {
        int width;
        int height;
    } geometry;
    int configured;
    int redraw_scheduled;
    int redraw_needed;
    window_redraw_handler_t redraw_handler;
    uint32_t time;
    int type;
    void *p_user_data;
};

struct WaylandDisplay*
CreateDisplay(int argc, char **argv);

void
DestroyDisplay(struct WaylandDisplay *p_display);

struct WaylandEglWindow*
CreateEglWindow(struct WaylandDisplay *p_display, const char *p_window_title,
                uint32_t surface_id);

void
WindowScheduleRedraw(struct WaylandEglWindow *p_window);

void
WindowScheduleResize(struct WaylandEglWindow *p_window, int width, int height);

void
DisplayRun(struct WaylandDisplay *p_display);

void
DisplayExit(struct WaylandDisplay *p_display);

int
DisplayAcquireWindowSurface(struct WaylandDisplay *p_display,
    struct WaylandEglWindow *p_window);

void
DisplaySetGlobalHandler(struct WaylandDisplay *p_display,
    display_global_handler_t handler);

void
DisplaySetUserData(struct WaylandDisplay *p_display, void *p_data);

void
WindowCreateSurface(struct WaylandEglWindow *p_window);

#endif
