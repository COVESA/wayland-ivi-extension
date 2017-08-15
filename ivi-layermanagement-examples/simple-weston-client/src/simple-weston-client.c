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

#include <wayland-cursor.h>
#include <ivi-application-client-protocol.h>

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
    struct wl_buffer        *wlBkgndBuffer;
    struct wl_cursor_theme  *cursor_theme;
    struct wl_cursor        *cursor;
    void                    *bkgnddata;
    uint32_t                formats;
}WaylandContextStruct;

static const char *left_ptrs[] = {
    "left_ptr",
    "default",
    "top_left_arrow",
    "left-arrow"
};

static BkGndSettingsStruct*
get_bkgnd_settings(void)
{
    BkGndSettingsStruct* bkgnd_settings;
    char *option;
    char *end;
    int len;

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

    for (j = 0; !wlcontext->cursor && j < 4; ++j)
        wlcontext->cursor = wl_cursor_theme_get_cursor(wlcontext->cursor_theme, left_ptrs[j]);

    if (!wlcontext->cursor)
    {
        fprintf(stderr, "could not load cursor '%s'\n", left_ptrs[j]);
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

static struct wl_pointer_listener pointer_listener = {
    PointerHandleEnter,
    PointerHandleLeave,
    PointerHandleMotion,
    PointerHandleButton,
    PointerHandleAxis
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

static struct wl_seat_listener seat_Listener = {
    seat_handle_capabilities,
};

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
        wlcontext->wl_seat =
                wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(wlcontext->wl_seat, &seat_Listener, data);
    }
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
    struct ivi_surface *ivisurf = NULL;
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
    struct ivi_surface *ivisurf = NULL;
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

    ivisurf = ivi_application_surface_create(wlcontext->ivi_application,
                                             bkgnd_settings->surface_id,
                                             wlcontext->wlBkgndSurface);

    return 0;
}

void destroy_bkgnd_surface(WaylandContextStruct* wlcontext)
{
    if (wlcontext->wlBkgndSurface)
        wl_surface_destroy(wlcontext->wlBkgndSurface);
}

int main (int argc, const char * argv[])
{
    WaylandContextStruct* wlcontext;
    BkGndSettingsStruct* bkgnd_settings;
    int offset = 0;
    int ret;

    /*get bkgnd settings and create shm-surface*/
    bkgnd_settings = get_bkgnd_settings();

    /*init wayland context*/
    wlcontext = (WaylandContextStruct*)calloc(1, sizeof(WaylandContextStruct));
    wlcontext->bkgnd_settings = bkgnd_settings;
    if (init_wayland_context(wlcontext)) {
        fprintf(stderr, "init_wayland_context failed\n");
        return -1;
    }

    if (create_bkgnd_surface(wlcontext)) {
        fprintf(stderr, "create_bkgnd_surface failed\n");
        goto Error;
    }

    wl_display_roundtrip(wlcontext->wl_display);

    /*draw the bkgnd display*/
    draw_bkgnd_surface(wlcontext);

    while (ret != -1)
        ret = wl_display_dispatch(wlcontext->wl_display);

Error:
    destroy_bkgnd_surface(wlcontext);
    destroy_wayland_context(wlcontext);

    free(bkgnd_settings);
    free(wlcontext);

    return 0;
}
