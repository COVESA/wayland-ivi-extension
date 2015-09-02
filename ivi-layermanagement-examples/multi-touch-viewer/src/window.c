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
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <wayland-client-protocol.h>
#include "window.h"
#include "util.h"

#ifndef ARRAY_LENGTH
#  define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])
#endif

struct Global
{
    uint32_t       name;
    char          *p_interface;
    uint32_t       version;
    struct wl_list link;
};

enum {
    TYPE_NONE,
    TYPE_TOPLEVEL
#if 0 /* not use */
    TYPE_FULLSCREEN,
    TYPE_MAXIMIZED,
    TYPE_TRANSIENT
#endif
};

/*** Event handlers ***********************************************************/

/**
 * wl_registry event handlers
 */
static void
registry_handle_global(void *p_data, struct wl_registry *p_registry,
    uint32_t id, const char *p_interface, uint32_t version)
{
    struct WaylandDisplay *p_display = (struct WaylandDisplay*)p_data;
    struct Global *p_global;

    p_global = (struct Global*)allocate(sizeof(struct Global), 0);
    if (NULL != p_global)
    {
        p_global->name        = id;
        p_global->p_interface = strdup(p_interface);
        p_global->version     = version;
        wl_list_insert(p_display->global_list.prev, &p_global->link);
    }

    if (0 == strcmp(p_interface, "wl_compositor"))
    {
        p_display->p_compositor = wl_registry_bind(p_registry, id,
            &wl_compositor_interface, 1);
    }
    else if (0 == strcmp(p_interface, "wl_shell"))
    {
        p_display->p_shell = wl_registry_bind(p_registry, id,
            &wl_shell_interface, 1);
    }
    else if (0 == strcmp(p_interface, "ivi_application"))
    {
        p_display->p_ivi_application = wl_registry_bind(p_registry, id,
            &ivi_application_interface, 1);
    }

    if (p_display->global_handler)
    {
        p_display->global_handler(p_display, id, p_interface, version,
            p_display->p_user_data);
    }
}

static void
registry_handle_remove(void *p_data, struct wl_registry *p_registry,
    uint32_t id)
{
    _UNUSED_(p_data);
    _UNUSED_(p_registry);
    _UNUSED_(id);
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_remove
};

/**
 * wl_surface event handlers
 */
static void
surface_enter(void *p_data, struct wl_surface *p_surface,
    struct wl_output *p_output)
{
    _UNUSED_(p_data);
    _UNUSED_(p_surface);
    _UNUSED_(p_output);
}

static void
surface_leave(void *p_data, struct wl_surface *p_surface,
    struct wl_output *p_output)
{
    _UNUSED_(p_data);
    _UNUSED_(p_surface);
    _UNUSED_(p_output);
}

static const struct wl_surface_listener surface_listener = {
    surface_enter,
    surface_leave
};

/**
 * wl_shell_surface event handlers
 */
static void
shell_surface_handle_ping(void *p_data,
    struct wl_shell_surface *p_shell_surface, uint32_t serial)
{
    _UNUSED_(p_data);

    wl_shell_surface_pong(p_shell_surface, serial);
}

static void
shell_surface_handle_configure(void *p_data,
    struct wl_shell_surface *p_shell_surface, uint32_t edges,
    int32_t width, int32_t height)
{
    _UNUSED_(p_shell_surface);
    _UNUSED_(edges);

    WindowScheduleResize((struct WaylandEglWindow*)p_data, width, height);
}

static void
shell_surface_handle_popup_done(void *p_data,
    struct wl_shell_surface *p_shell_surface)
{
    _UNUSED_(p_data);
    _UNUSED_(p_shell_surface);
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    shell_surface_handle_ping,
    shell_surface_handle_configure,
    shell_surface_handle_popup_done
};

/**
 * wl_callback event handler
 */
static void
frame_callback(void *p_data, struct wl_callback *p_cb, uint32_t time)
{
    struct WaylandEglWindow *p_window = (struct WaylandEglWindow*)p_data;

    assert(p_cb == p_window->p_frame_cb);

    wl_callback_destroy(p_cb);

    p_window->p_frame_cb = NULL;
    p_window->redraw_scheduled = 0;
    p_window->time = time;

    if (0 != p_window->redraw_needed)
    {
        WindowScheduleRedraw(p_window);
    }
}

static const struct wl_callback_listener frame_listener = {
    frame_callback
};

static void
ivi_surface_configure(void *p_data, struct ivi_surface *p_ivi_surface,
                      int32_t width, int32_t height)
{
    _UNUSED_(p_data);
    _UNUSED_(p_ivi_surface);
    _UNUSED_(width);
    _UNUSED_(height);
}

static const struct ivi_surface_listener ivi_surface_event_listener = {
    ivi_surface_configure
};

/*** Static functions *********************************************************/
static int
set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
    return -1;
}

int
os_epoll_create_cloexec()
{
    int fd = epoll_create1(EPOLL_CLOEXEC);

    if (fd >= 0)
    {
        return fd;
    }

    if (errno != EINVAL)
    {
        return -1;
    }

    fd = epoll_create(1);
    return set_cloexec_or_close(fd);
}

static void
handle_display_data(struct Task *p_task, uint32_t events)
{
    struct WaylandDisplay *p_display = (struct WaylandDisplay*)p_task;
    struct epoll_event ep;
    int ret;

    p_display->display_fd_events = events;

    if ((events & EPOLLERR) || (events & EPOLLHUP))
    {
        DisplayExit(p_display);
        return;
    }

    if (events & EPOLLIN)
    {
        ret = wl_display_dispatch(p_display->p_display);
        if (ret == -1)
        {
            DisplayExit(p_display);
            return;
        }
    }

    if (events & EPOLLOUT)
    {
        ret = wl_display_flush(p_display->p_display);
        if (ret == 0)
        {
            ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            ep.data.ptr = &p_display->display_task;
            epoll_ctl(p_display->epoll_fd,
                EPOLL_CTL_MOD, p_display->display_fd, &ep);
        }
        else if (ret == -1 && errno != EAGAIN)
        {
            DisplayExit(p_display);
            return;
        }
    }
}

void
display_watch_fd(struct WaylandDisplay *p_display, int fd, uint32_t events,
    struct Task *p_task)
{
    struct epoll_event ep;

    ep.events = events;
    ep.data.ptr = p_task;
    epoll_ctl(p_display->epoll_fd, EPOLL_CTL_ADD, fd, &ep);
}

static int
init_egl(struct WaylandDisplay *p_display)
{
    EGLint iMajor, iMinor;
    EGLint n;

    static const EGLint argb_config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE,   1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE,  1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    p_display->egldisplay = eglGetDisplay(p_display->p_display);

    if (!eglInitialize(p_display->egldisplay, &iMajor, &iMinor))
    {
        fprintf(stderr, "failed to initialize EGL\n");
        return -1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API))
    {
        fprintf(stderr, "failed to bind EGL client API\n");
        return -1;
    }

    if (!eglChooseConfig(p_display->egldisplay, argb_config_attribs,
        &p_display->eglconfig, 1, &n))
    {
        fprintf(stderr, "failed to choose argb EGL config\n");
        return -1;
    }

    p_display->eglcontext = eglCreateContext(p_display->egldisplay,
        p_display->eglconfig, EGL_NO_CONTEXT, context_attribs);
    if (NULL == p_display->eglcontext)
    {
        fprintf(stderr, "failed to create EGL context\n");
        return -1;
    }
#if 0
    if (!eglMakeCurrent(p_display->egldisplay, NULL, NULL,
        p_display->eglcontext))
    {
        fprintf(stderr, "failed to make EGL context current\n");
        return -1;
    }
#endif
    return 0;
}

static void
finish_egl(struct WaylandDisplay *p_display)
{
    eglMakeCurrent(p_display->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
        EGL_NO_CONTEXT);

    eglTerminate(p_display->egldisplay);
    eglReleaseThread();
}

static void
window_create_surface(struct WaylandEglWindow *p_window)
{
    if (NULL != p_window->p_egl_window)
    {
        return;
    }

    p_window->p_egl_window = wl_egl_window_create(p_window->p_surface,
        p_window->geometry.width, p_window->geometry.height);

    p_window->eglsurface = eglCreateWindowSurface(
        p_window->p_display->egldisplay, p_window->p_display->eglconfig,
        p_window->p_egl_window, NULL);

    eglMakeCurrent(p_window->p_display->egldisplay, p_window->eglsurface,
        p_window->eglsurface, p_window->p_display->eglcontext);
}

static void
idle_redraw(struct Task *p_task, uint32_t events)
{
    struct WaylandEglWindow *p_window = (struct WaylandEglWindow*)p_task;

    _UNUSED_(events);

    window_create_surface(p_window);

    if (NULL != p_window->redraw_handler)
    {
        p_window->redraw_handler(p_window, p_window->p_user_data);
    }

    p_window->redraw_needed = 0;
    wl_list_init(&p_window->redraw_task.link);

    if (p_window->type == TYPE_NONE)
    {
        p_window->type = TYPE_TOPLEVEL;
        if (NULL != p_window->p_shell_surface)
        {
            wl_shell_surface_set_toplevel(p_window->p_shell_surface);
        }
    }

    p_window->p_frame_cb = wl_surface_frame(p_window->p_surface);
    wl_callback_add_listener(p_window->p_frame_cb, &frame_listener,
        p_window);

    eglSwapBuffers(p_window->p_display->egldisplay, p_window->eglsurface);
}

/*** Public functions *********************************************************/

void
WindowCreateSurface(struct WaylandEglWindow *p_window)
{
    window_create_surface(p_window);
}

void
DisplaySetGlobalHandler(struct WaylandDisplay *p_display,
    display_global_handler_t handler)
{
    struct Global *p_global;

    p_display->global_handler = handler;
    if (NULL == handler)
    {
        return;
    }

    wl_list_for_each(p_global, &p_display->global_list, link)
    {
        p_display->global_handler(p_display, p_global->name,
            p_global->p_interface, p_global->version, p_display->p_user_data);
    }
}

void
DisplaySetUserData(struct WaylandDisplay *p_display, void *p_data)
{
    p_display->p_user_data = p_data;
}

void
WindowScheduleRedraw(struct WaylandEglWindow *p_window)
{
    p_window->redraw_needed = 1;
    if (0 == p_window->redraw_scheduled)
    {
        p_window->redraw_task.run = idle_redraw;
        wl_list_insert(&p_window->p_display->deferred_list, &p_window->redraw_task.link);
        p_window->redraw_scheduled = 1;
    }
}

void
WindowScheduleResize(struct WaylandEglWindow *p_window, int width, int height)
{
    p_window->geometry.width  = width;
    p_window->geometry.height = height;

    WindowScheduleRedraw(p_window);
}

int
DisplayAcquireWindowSurface(struct WaylandDisplay *p_display,
    struct WaylandEglWindow *p_window)
{
    if (!eglMakeCurrent(p_display->egldisplay, p_window->eglsurface,
        p_window->eglsurface, p_display->eglcontext))
    {
        fprintf(stderr, "[ERR] failed to make surface current\n");
    }

    return 0;
}

struct WaylandDisplay *
CreateDisplay(int argc, char **argv)
{
    struct WaylandDisplay *p_display;

    _UNUSED_(argc);
    _UNUSED_(argv);

    p_display = (struct WaylandDisplay*)malloc(sizeof(struct WaylandDisplay));
    if (NULL == p_display)
    {
        return NULL;
    }

    p_display->p_display = wl_display_connect(NULL);
    if (NULL == p_display->p_display)
    {
        fprintf(stderr, "[ERR] failed to connect to wayland: %m\n");
        free(p_display);
        return NULL;
    }

    p_display->epoll_fd = os_epoll_create_cloexec();
    p_display->display_fd = wl_display_get_fd(p_display->p_display);
    p_display->display_task.run = handle_display_data;
    display_watch_fd(p_display, p_display->display_fd,
        EPOLLIN | EPOLLERR | EPOLLHUP, &p_display->display_task);

    wl_list_init(&p_display->global_list);
    wl_list_init(&p_display->surface_list);
    wl_list_init(&p_display->deferred_list);

    p_display->p_registry = wl_display_get_registry(p_display->p_display);
    wl_registry_add_listener(p_display->p_registry, &registry_listener,
        p_display);

    if (0 > wl_display_dispatch(p_display->p_display))
    {
        fprintf(stderr, "[ERR] failed to process wayland connection: %m\n");
        free(p_display);
        return NULL;
    }

    if (0 > init_egl(p_display))
    {
        fprintf(stderr, "[ERR] EGL does not seem to work\n");
        free(p_display);
        return NULL;
    }

    return p_display;
}

void
DestroyDisplay(struct WaylandDisplay *p_display)
{
    finish_egl(p_display);

    if (NULL != p_display->p_shell)
        wl_shell_destroy(p_display->p_shell);

    if (NULL != p_display->p_ivi_application)
        ivi_application_destroy(p_display->p_ivi_application);

    if (NULL != p_display->p_compositor)
        wl_compositor_destroy(p_display->p_compositor);

    if (NULL != p_display->p_registry)
        wl_registry_destroy(p_display->p_registry);

    if (NULL != p_display->p_display)
        wl_display_disconnect(p_display->p_display);

    free(p_display);
}

struct WaylandEglWindow *
CreateEglWindow(struct WaylandDisplay *p_display, const char *p_window_title,
                uint32_t surface_id)
{
    struct WaylandEglWindow *p_window;

    p_window =
        (struct WaylandEglWindow*)malloc(sizeof(struct WaylandEglWindow));
    if (NULL == p_window)
    {
        return NULL;
    }
    memset(p_window, 0x00, sizeof(struct WaylandEglWindow));

    p_window->p_display = p_display;
    p_window->p_surface = wl_compositor_create_surface(p_display->p_compositor);

    wl_surface_add_listener(p_window->p_surface, &surface_listener, p_window);

    if (NULL != p_display->p_shell)
    {
        p_window->p_shell_surface =
            wl_shell_get_shell_surface(p_display->p_shell, p_window->p_surface);

        wl_shell_surface_set_user_data(p_window->p_shell_surface, p_window);

        wl_shell_surface_add_listener(p_window->p_shell_surface,
            &shell_surface_listener, p_window);

        wl_shell_surface_set_title(p_window->p_shell_surface, p_window_title);
    }

    if (NULL != p_display->p_ivi_application)
    {
        /* force sync to compositor */
        wl_display_roundtrip(p_display->p_display);
        p_window->p_ivi_surface =
            ivi_application_surface_create(p_display->p_ivi_application,
                                           surface_id,
                                           p_window->p_surface);
        ivi_surface_add_listener(p_window->p_ivi_surface,
                                 &ivi_surface_event_listener, p_window);
    }

    wl_list_insert(p_display->surface_list.prev, &p_window->link);

    return p_window;
}

void
DisplayRun(struct WaylandDisplay *p_display)
{
    struct Task *p_task;
    struct epoll_event ep[16];
    int i, count, ret;

    p_display->running = 1;
    while (1)
    {
        while (0 == wl_list_empty(&p_display->deferred_list))
        {
            /* avoid lint warning */
            const __typeof__(((struct Task*)0)->link) *p_ptr = p_display->deferred_list.prev;
            p_task = (struct Task*)((unsigned long)p_ptr - offsetof(struct Task, link));

            wl_list_remove(&p_task->link);
            p_task->run(p_task, 0);
        }

        wl_display_dispatch_pending(p_display->p_display);

        if (0 == p_display->running)
        {
            break;
        }

        ret = wl_display_flush(p_display->p_display);
        if (0 > ret && EAGAIN == errno)
        {
            ep[0].events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
            ep[0].data.ptr = &p_display->display_task;
            epoll_ctl(p_display->epoll_fd, EPOLL_CTL_MOD,
                p_display->display_fd, &ep[0]);
        }
        else if (0 > ret)
        {
            break;
        }

        count = epoll_wait(p_display->epoll_fd, ep, ARRAY_LENGTH(ep), 1);
        for (i = 0; i < count; ++i)
        {
            p_task = ep[i].data.ptr;
            p_task->run(p_task, ep[i].events);
        }
    }
}

void
DisplayExit(struct WaylandDisplay *p_display)
{
    if (NULL != p_display)
    {
        p_display->running = 0;
    }
}
