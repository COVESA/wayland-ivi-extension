/*
 * Copyright (C) 2015 Advanced Driver Information Technology Joint Venture GmbH
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>

#include <wayland-cursor.h>
#include <ivi-application-client-protocol.h>

#ifdef LIBWESTON_DEBUG_PROTOCOL
#include "weston-debug-client-protocol.h"
#define MAXSTRLEN 1024
#endif

#ifdef DLT
#include "dlt.h"

#define WESTON_DLT_APP_DESC "messages from weston debug protocol"
#define WESTON_DLT_CONTEXT_DESC "weston debug context"

#define WESTON_DLT_APP "WESN"
#define WESTON_DLT_CONTEXT "WESC"
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

static int running = 1;

typedef struct _BkGndSettings
{
    uint32_t surface_id;
    uint32_t surface_width;
    uint32_t surface_height;
    uint32_t surface_stride;
}BkGndSettingsStruct;

typedef struct _WaylandContext {
    struct wl_display       *wl_display;
    struct wl_registry      *wl_registry;
    struct wl_compositor    *wl_compositor;
    struct wl_shm           *wl_shm;
    struct wl_seat          *wl_seat;
    struct wl_pointer       *wl_pointer;
    struct wl_surface       *wl_pointer_surface;
    struct ivi_application  *ivi_application;
    BkGndSettingsStruct     *bkgnd_settings;
    struct wl_surface       *wlBkgndSurface;
    struct ivi_surface      *ivi_surf;
    struct wl_buffer        *wlBkgndBuffer;
    struct wl_cursor_theme  *cursor_theme;
    struct wl_cursor        *cursor;
    void                    *bkgnddata;
    uint32_t                formats;
#ifdef LIBWESTON_DEBUG_PROTOCOL
    struct weston_debug_v1  *debug_iface;
    struct wl_list          stream_list;
    int                     debug_fd;
    char                    thread_running;
    pthread_t               dlt_ctx_thread;
    int                     pipefd[2];
#endif
    uint8_t                 enable_cursor;
}WaylandContextStruct;

struct debug_stream {
    struct wl_list link;
    int should_bind;
    char *name;
    struct weston_debug_stream_v1 *obj;
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const char *left_ptrs[] = {
    "left_ptr",
    "default",
    "top_left_arrow",
    "left-arrow"
};

static BkGndSettingsStruct*
get_bkgnd_settings_cursor_info(WaylandContextStruct* wlcontext)
{
    BkGndSettingsStruct* bkgnd_settings;
    char *option;
    char *end;

    option = getenv("IVI_CLIENT_ENABLE_CURSOR");
    if(option)
        wlcontext->enable_cursor = (uint8_t)strtol(option, &end, 0);
    else
        wlcontext->enable_cursor = 0;

    bkgnd_settings =
            (BkGndSettingsStruct*)calloc(1, sizeof(BkGndSettingsStruct));

    option = getenv("IVI_CLIENT_SURFACE_ID");
    if(option)
        bkgnd_settings->surface_id = strtol(option, &end, 0);
    else
        bkgnd_settings->surface_id = 10;

    bkgnd_settings->surface_width = 1;
    bkgnd_settings->surface_height = 1;
    bkgnd_settings->surface_stride = bkgnd_settings->surface_width * 4;

    return bkgnd_settings;
}

#ifdef LIBWESTON_DEBUG_PROTOCOL
static struct debug_stream *
stream_alloc(WaylandContextStruct* wlcontext, const char *name)
{
    struct debug_stream *stream;

    stream = calloc(1, (sizeof *stream));
    if (!stream)
        return NULL;

    stream->name = strdup(name);
    if (!stream->name) {
        free(stream);
        return NULL;
    }

    stream->should_bind = 0;

    wl_list_insert(wlcontext->stream_list.prev, &stream->link);

    return stream;
}

static void
stream_destroy(struct debug_stream *stream)
{
    if (stream->obj)
        weston_debug_stream_v1_destroy(stream->obj);

    wl_list_remove(&stream->link);
    free(stream->name);
    free(stream);
}

static void
destroy_streams(WaylandContextStruct* wlcontext)
{
    struct debug_stream *stream;
    struct debug_stream *tmp;

    wl_list_for_each_safe(stream, tmp, &wlcontext->stream_list, link) {
        stream_destroy(stream);
    }
}

static void
handle_stream_complete(void *data, struct weston_debug_stream_v1 *obj)
{
    struct debug_stream *stream = data;

    assert(stream->obj == obj);

    stream_destroy(stream);
}

static void
handle_stream_failure(void *data, struct weston_debug_stream_v1 *obj,
		      const char *msg)
{
    struct debug_stream *stream = data;

    assert(stream->obj == obj);

    fprintf(stderr, "Debug stream '%s' aborted: %s\n", stream->name, msg);

    stream_destroy(stream);
}

static const struct weston_debug_stream_v1_listener stream_listener = {
    handle_stream_complete,
    handle_stream_failure
};

static void
start_streams(WaylandContextStruct* wlcontext)
{
    struct debug_stream *stream;

    wl_list_for_each(stream, &wlcontext->stream_list, link) {
        if (stream->should_bind) {
        stream->obj = weston_debug_v1_subscribe(wlcontext->debug_iface,
                            stream->name,
                            wlcontext->debug_fd);
        weston_debug_stream_v1_add_listener(stream->obj,
                            &stream_listener, stream);
        }
    }
}

static void
get_debug_streams(WaylandContextStruct* wlcontext)
{
    char *stream_names;
    char *stream;
    const char separator[2] = " ";

    stream_names = getenv("IVI_CLIENT_DEBUG_STREAM_NAMES");

    if(NULL == stream_names)
        return;

    /* get the first stream */
    stream = strtok(stream_names, separator);

    /* walk through other streams */
    while( stream != NULL ) {
        stream_alloc(wlcontext, stream);
        stream = strtok(NULL, separator);
    }
}
#endif

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    WaylandContextStruct* wlcontext = data;

    wlcontext->formats |= (1 << format);
}

static struct wl_shm_listener shm_listenter = {
    shm_format
};

static int create_cursors(WaylandContextStruct* wlcontext) {
    int size = 32;
    unsigned int j;

    wlcontext->cursor_theme = wl_cursor_theme_load(NULL, size, wlcontext->wl_shm);
    if (!wlcontext->cursor_theme) {
        fprintf(stderr, "could not load default theme\n");
        return -1;
    }

    wlcontext->cursor = NULL;

    for (j = 0; !wlcontext->cursor && j < ARRAY_SIZE(left_ptrs); ++j)
        wlcontext->cursor = wl_cursor_theme_get_cursor(wlcontext->cursor_theme, left_ptrs[j]);

    if (!wlcontext->cursor)
    {
        fprintf(stderr, "could not load any cursor\n");
        return -1;
    }

    return 0;
}

static int set_pointer_image(WaylandContextStruct* wlcontext)
{
    struct wl_cursor *cursor = NULL;
    struct wl_cursor_image *image = NULL;
    struct wl_buffer *buffer = NULL;

    if (!wlcontext->wl_pointer || !wlcontext->cursor) {
        fprintf(stderr, "no wl_pointer or no wl_cursor\n");
        return -1;
    }

    cursor = wlcontext->cursor;
    image = cursor->images[0];
    buffer = wl_cursor_image_get_buffer(image);
    if (!buffer) {
        fprintf(stderr, "buffer for cursor not available\n");
        return -1;
    }

    wl_pointer_set_cursor(wlcontext->wl_pointer, 0,
                          wlcontext->wl_pointer_surface,
                          image->hotspot_x, image->hotspot_y);
    wl_surface_attach(wlcontext->wl_pointer_surface, buffer, 0, 0);
    wl_surface_damage(wlcontext->wl_pointer_surface, 0, 0,
                      image->width, image->height);
    wl_surface_commit(wlcontext->wl_pointer_surface);

    return 0;
}

static void
PointerHandleEnter(void *data, struct wl_pointer *wlPointer, uint32_t serial,
                   struct wl_surface *wlSurface, wl_fixed_t sx, wl_fixed_t sy)
{
    WaylandContextStruct *wlcontext = (WaylandContextStruct*)data;

    set_pointer_image(wlcontext);

    printf("ENTER EGLWLINPUT PointerHandleEnter: x(%d), y(%d)\n",
           wl_fixed_to_int(sx), wl_fixed_to_int(sy));
}

static void
PointerHandleLeave(void *data, struct wl_pointer *wlPointer, uint32_t serial,
                   struct wl_surface *wlSurface)
{

}

static void
PointerHandleMotion(void *data, struct wl_pointer *wlPointer, uint32_t time,
                    wl_fixed_t sx, wl_fixed_t sy)
{

}

static void
PointerHandleAxis(void *data, struct wl_pointer *wlPointer, uint32_t time,
                  uint32_t axis, wl_fixed_t value)
{

}

static void
PointerHandleButton(void *data, struct wl_pointer *wlPointer, uint32_t serial,
                    uint32_t time, uint32_t button, uint32_t state)
{

}

static void
PointerHandleFrame(void *data, struct wl_pointer *wl_pointer)
{

}

static void
PointerHandleAxisSource(void *data, struct wl_pointer *wl_pointer,
                        uint32_t axis_source)
{

}

static void
PointerHandleAxisStop(void *data, struct wl_pointer *wl_pointer,
                      uint32_t time, uint32_t axis)
{

}

static void
PointerHandleAxisDiscrete(void *data, struct wl_pointer *wl_pointer,
                          uint32_t axis, int32_t discrete)
{

}

static struct wl_pointer_listener pointer_listener = {
    PointerHandleEnter,
    PointerHandleLeave,
    PointerHandleMotion,
    PointerHandleButton,
    PointerHandleAxis,
    PointerHandleFrame,
    PointerHandleAxisSource,
    PointerHandleAxisStop,
    PointerHandleAxisDiscrete
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
    WaylandContextStruct *wlcontext = (WaylandContextStruct*)data;
    struct wl_seat *wl_seat = wlcontext->wl_seat;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wlcontext->wl_pointer) {
        wlcontext->wl_pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(wlcontext->wl_pointer,
                                &pointer_listener, data);
        /*create cursors*/
        create_cursors(wlcontext);
        wlcontext->wl_pointer_surface =
                wl_compositor_create_surface(wlcontext->wl_compositor);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlcontext->wl_pointer) {
        wl_pointer_destroy(wlcontext->wl_pointer);
        wlcontext->wl_pointer = NULL;

        if (wlcontext->wl_pointer_surface) {
            wl_surface_destroy(wlcontext->wl_pointer_surface);
            wlcontext->wl_pointer_surface = NULL;
        }

        if (wlcontext->cursor_theme)
            wl_cursor_theme_destroy(wlcontext->cursor_theme);
    }
}

static void
seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{

}

static struct wl_seat_listener seat_Listener = {
    seat_handle_capabilities,
    seat_name
};

#ifdef LIBWESTON_DEBUG_PROTOCOL
static void
stream_find(WaylandContextStruct* wlcontext, const char *name, const char *desc)
{
    struct debug_stream *stream;
    wl_list_for_each(stream, &wlcontext->stream_list, link)
        if (strcmp(stream->name, name) == 0) {
            stream->should_bind = 1;
        }
}

static void
debug_advertise(void *data, struct weston_debug_v1 *debug, const char *name,
                const char *desc)
{
        WaylandContextStruct* wlcontext = data;
        stream_find(wlcontext, name, desc);
}

static const struct weston_debug_v1_listener debug_listener = {
        debug_advertise
};
#endif

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version)
{
    WaylandContextStruct* wlcontext = (WaylandContextStruct*)data;

    if (!strcmp(interface, "wl_compositor")) {
        wlcontext->wl_compositor =
                wl_registry_bind(registry, name,
                                 &wl_compositor_interface, 1);
    }
    else if (!strcmp(interface, "wl_shm")) {
        wlcontext->wl_shm =
                wl_registry_bind(registry, name, &wl_shm_interface, 1);
        wl_shm_add_listener(wlcontext->wl_shm, &shm_listenter, wlcontext);
    }
    else if (!strcmp(interface, "ivi_application")) {
        wlcontext->ivi_application =
                wl_registry_bind(registry, name,
                                 &ivi_application_interface, 1);
    }
    else if (!strcmp(interface, "wl_seat")) {
        if (wlcontext->enable_cursor) {
            wlcontext->wl_seat =
                wl_registry_bind(registry, name, &wl_seat_interface, 1);
            wl_seat_add_listener(wlcontext->wl_seat, &seat_Listener, data);
        }
    }
#ifdef LIBWESTON_DEBUG_PROTOCOL
    else if (!strcmp(interface, weston_debug_v1_interface.name)) {
        uint32_t myver;

        if (wlcontext->debug_iface || wl_list_empty(&wlcontext->stream_list))
            return;

        myver = MIN(1, version);
        wlcontext->debug_iface =
                wl_registry_bind(registry, name,
                    &weston_debug_v1_interface, myver);
        weston_debug_v1_add_listener(wlcontext->debug_iface, &debug_listener,
                                             wlcontext);
    }
#endif
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{

}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

int init_wayland_context(WaylandContextStruct* wlcontext)
{
    wlcontext->wl_display = wl_display_connect(NULL);
    if (NULL == wlcontext->wl_display) {
        printf("Error: wl_display_connect failed\n");
        return -1;
    }

    wlcontext->wl_registry = wl_display_get_registry(wlcontext->wl_display);
    wl_registry_add_listener(wlcontext->wl_registry,
                             &registry_listener, wlcontext);
    wl_display_roundtrip(wlcontext->wl_display);

    if (wlcontext->wl_shm == NULL) {
        fprintf(stderr, "No wl_shm global\n");
        return -1;
    }

    wl_display_roundtrip(wlcontext->wl_display);

    if (!(wlcontext->formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
        fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
        return -1;
    }

#ifdef LIBWESTON_DEBUG_PROTOCOL
    if (!wl_list_empty(&wlcontext->stream_list) &&
        (wlcontext->debug_iface == NULL)) {
        fprintf(stderr, "WARNING: weston_debug protocol is not available,"
                " missed enabling --debug option to weston ?\n");
    }
#endif

    return 0;
}

void destroy_wayland_context(WaylandContextStruct* wlcontext)
{
    if(wlcontext->wl_compositor)
        wl_compositor_destroy(wlcontext->wl_compositor);

    if(wlcontext->wl_display)
        wl_display_disconnect(wlcontext->wl_display);
}

int
create_file(int size)
{
    static const char template[] = "/weston-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;
    long flags;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name)
        return -1;

    strcpy(name, path);
    strcat(name, template);

    fd = mkstemp(name);
    if (fd >= 0) {
        flags = fcntl(fd, F_GETFD);
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        unlink(name);
    }

    free(name);

    if (fd < 0)
        return -1;

    ret = ftruncate(fd, size);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static int
create_shm_buffer(WaylandContextStruct* wlcontext)
{
    struct wl_shm_pool *pool;
    BkGndSettingsStruct* bkgnd_settings = wlcontext->bkgnd_settings;

    int fd = -1;
    int size = 0;
    int stride = 0;

    stride = bkgnd_settings->surface_stride;
    size = stride * bkgnd_settings->surface_height;
    fd = create_file(size);
    if (fd < 0) {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n",
                size);
        return -1;
    }

    wlcontext->bkgnddata =
            mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (MAP_FAILED == wlcontext->bkgnddata) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        return -1;
    }

    pool = wl_shm_create_pool(wlcontext->wl_shm, fd, size);
    wlcontext->wlBkgndBuffer = wl_shm_pool_create_buffer(pool, 0,
                                bkgnd_settings->surface_width,
                                bkgnd_settings->surface_height,
                                stride,
                                WL_SHM_FORMAT_ARGB8888);
    if (NULL == wlcontext->wlBkgndBuffer) {
        fprintf(stderr, "wl_shm_create_buffer failed: %m\n");
        close(fd);
        return -1;
    }

    wl_shm_pool_destroy(pool);
    close(fd);

    return 0;
}

int draw_bkgnd_surface(WaylandContextStruct* wlcontext)
{
    BkGndSettingsStruct *bkgnd_settings = wlcontext->bkgnd_settings;
    uint32_t* pixels;

    pixels = (uint32_t*)wlcontext->bkgnddata;
    *pixels = 0x0;

    wl_surface_attach(wlcontext->wlBkgndSurface, wlcontext->wlBkgndBuffer, 0, 0);
    wl_surface_damage(wlcontext->wlBkgndSurface, 0, 0,
                      bkgnd_settings->surface_width, bkgnd_settings->surface_height);
    wl_surface_commit(wlcontext->wlBkgndSurface);

    return 0;
}

int create_bkgnd_surface(WaylandContextStruct* wlcontext)
{
    BkGndSettingsStruct *bkgnd_settings = wlcontext->bkgnd_settings;

    wlcontext->wlBkgndSurface =
            wl_compositor_create_surface(wlcontext->wl_compositor);
    if (NULL == wlcontext->wlBkgndSurface) {
        printf("Error: wl_compositor_create_surface failed.\n");
        return -1;
    }

    if (create_shm_buffer(wlcontext)) {
        printf("Error: shm buffer creation failed.\n");
        return -1;
    }

    wlcontext->ivi_surf = ivi_application_surface_create(wlcontext->ivi_application,
                                                         bkgnd_settings->surface_id,
                                                         wlcontext->wlBkgndSurface);

    return 0;
}

void destroy_bkgnd_surface(WaylandContextStruct* wlcontext)
{
    if (wlcontext->ivi_surf)
        ivi_surface_destroy(wlcontext->ivi_surf);

    if (wlcontext->wlBkgndSurface)
        wl_surface_destroy(wlcontext->wlBkgndSurface);
}

#ifdef LIBWESTON_DEBUG_PROTOCOL
static void *
weston_dlt_thread_function(void *data)
{
    WaylandContextStruct* wlcontext;

#ifdef DLT
    /*init dlt*/
    char apid[DLT_ID_SIZE];
    char ctid[DLT_ID_SIZE];
    DLT_DECLARE_CONTEXT(weston_dlt_context)
    dlt_set_id(apid, WESTON_DLT_APP);
    dlt_set_id(ctid, WESTON_DLT_CONTEXT);

    DLT_REGISTER_APP(apid, WESTON_DLT_APP_DESC);
    DLT_REGISTER_CONTEXT(weston_dlt_context, ctid, WESTON_DLT_CONTEXT_DESC);
#endif
    wlcontext = (WaylandContextStruct*)data;
    /*make the stdin as read end of the pipe*/
    dup2(wlcontext->pipefd[0], STDIN_FILENO);

    while (running && wlcontext->thread_running)
    {
        char str[MAXSTRLEN] = {0};
        int i = -1;

        /* read from std-in(read end of pipe) till newline char*/
        do {
            i++;
            if(read(wlcontext->pipefd[0], &str[i], 1) < 0)
                printf("read failed : %s", strerror(errno));
        } while (str[i] != '\n');

        if (strcmp(str,"")!=0)
        {
#ifdef DLT
            DLT_LOG(weston_dlt_context, DLT_LOG_INFO, DLT_STRING(str));
#else
            fprintf(stderr,"%s",str);
#endif
        }
    }

#ifdef DLT
    DLT_UNREGISTER_CONTEXT(weston_dlt_context);
    DLT_UNREGISTER_APP();
#endif

    pthread_exit(NULL);
}
#endif

static void
signal_int(int signum)
{
    running = 0;
}

static int
display_poll(struct wl_display *display, short int events)
{
    int ret;
    struct pollfd pfd[1];

    pfd[0].fd = wl_display_get_fd(display);
    pfd[0].events = events;
    do {
        ret = poll(pfd, 1, -1);
    } while (ret == -1 && errno == EINTR && running);

    if(0 == running)
        ret = -1;

    return ret;
}

/* implemented local API for dispatch the default queue
 * to handle the Ctrl+C signal properly, with the wl_display_dispatch
 * the poll is continuing because the generated errno is EINTR,
 * so added running flag also to decide whether to continue polling or not */
static int
display_dispatch(struct wl_display *display)
{
    int ret;

    if (wl_display_prepare_read(display) == -1)
        return wl_display_dispatch_pending(display);

    while (1) {
        ret = wl_display_flush(display);

        if (ret != -1 || errno != EAGAIN)
            break;

        if (display_poll(display, POLLOUT) == -1) {
            wl_display_cancel_read(display);
            return -1;
        }
    }

    /* Don't stop if flushing hits an EPIPE; continue so we can read any
     * protocol error that may have triggered it. */
    if (ret < 0 && errno != EPIPE) {
        wl_display_cancel_read(display);
        return -1;
    }

    if (display_poll(display, POLLIN) == -1) {
        wl_display_cancel_read(display);
        return -1;
    }

    if (wl_display_read_events(display) == -1)
        return -1;

    return wl_display_dispatch_pending(display);
}

int main (int argc, const char * argv[])
{
    WaylandContextStruct* wlcontext;
    BkGndSettingsStruct* bkgnd_settings;

    struct sigaction sigint;
    int ret = 0;

    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigaction(SIGINT, &sigint, NULL);
    sigaction(SIGTERM, &sigint, NULL);
    sigaction(SIGSEGV, &sigint, NULL);

    wlcontext = (WaylandContextStruct*)calloc(1, sizeof(WaylandContextStruct));

    /*get bkgnd settings and create shm-surface*/
    bkgnd_settings = get_bkgnd_settings_cursor_info(wlcontext);

    /*init wayland context*/
    wlcontext->bkgnd_settings = bkgnd_settings;

#ifdef LIBWESTON_DEBUG_PROTOCOL
    /*init debug stream list*/
    wl_list_init(&wlcontext->stream_list);
    get_debug_streams(wlcontext);
    wlcontext->debug_fd = STDOUT_FILENO;
#else
    fprintf(stderr, "WARNING: weston_debug protocol is not available\n");
#endif

    if (init_wayland_context(wlcontext)) {
        fprintf(stderr, "init_wayland_context failed\n");
        goto ErrorContext;
    }

    if (create_bkgnd_surface(wlcontext)) {
        fprintf(stderr, "create_bkgnd_surface failed\n");
        goto Error;
    }

    wl_display_roundtrip(wlcontext->wl_display);

#ifdef LIBWESTON_DEBUG_PROTOCOL
    if (!wl_list_empty(&wlcontext->stream_list) &&
            wlcontext->debug_iface) {
        /* create the pipe b/w stdout and stdin
         * stdout - write end
         * stdin - read end
         * weston will write to stdout and the
         * dlt_ctx_thread will read from stdin */
        if((pipe(wlcontext->pipefd)) < 0)
            printf("Error in pipe() processing : %s", strerror(errno));

        dup2(wlcontext->pipefd[1], STDOUT_FILENO);

        wlcontext->thread_running = 1;
        pthread_create(&wlcontext->dlt_ctx_thread, NULL,
                weston_dlt_thread_function, wlcontext);
        start_streams(wlcontext);
    }
#endif

    /*draw the bkgnd display*/
    draw_bkgnd_surface(wlcontext);

    while (running && (ret != -1))
        ret = display_dispatch(wlcontext->wl_display);

Error:
#ifdef LIBWESTON_DEBUG_PROTOCOL
    if(wlcontext->debug_iface)
        weston_debug_v1_destroy(wlcontext->debug_iface);

    destroy_streams(wlcontext);
    wl_display_roundtrip(wlcontext->wl_display);

    if(wlcontext->thread_running)
    {
        close(wlcontext->pipefd[1]);
        close(STDOUT_FILENO);
        wlcontext->thread_running = 0;
        pthread_join(wlcontext->dlt_ctx_thread, NULL);
        close(wlcontext->pipefd[0]);
    }
#endif

    destroy_bkgnd_surface(wlcontext);
ErrorContext:
    destroy_wayland_context(wlcontext);

    free(bkgnd_settings);
    free(wlcontext);

    return 0;
}
