/*
 * Copyright (C) 2013 DENSO CORPORATION
 * Copyright (C) 2016 Advanced Driver Information Technology Joint Venture GmbH
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
/*
 * This is implementation of ivi-controller.xml. This implementation uses
 * ivi-extension APIs, which uses ivi_controller_interface pvoided by
 * ivi-layout.c in weston.
 */

#include "config.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include <weston.h>
#include "ivi-wm-server-protocol.h"
#include "ivi-controller.h"

#include "wayland-util.h"

#define IVI_CLIENT_SURFACE_ID_ENV_NAME "IVI_CLIENT_SURFACE_ID"
#define IVI_CLIENT_DEBUG_SCOPES_ENV_NAME "IVI_CLIENT_DEBUG_STREAM_NAMES"
#define IVI_CLIENT_ENABLE_CURSOR_ENV_NAME "IVI_CLIENT_ENABLE_CURSOR"

struct ivilayer;
struct iviscreen;

struct notification {
    struct wl_list link;
    struct wl_resource *resource;
    struct wl_list layout_link;
};

struct ivilayer {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_layer *layout_layer;
    const struct ivi_layout_layer_properties *prop;
    struct wl_listener property_changed;
    struct wl_list notification_list;
};

struct iviscreen {
    struct wl_list link;
    struct ivishell *shell;
    uint32_t id_screen;
    struct weston_output *output;
    struct wl_list resource_list;
};

struct ivicontroller {
    struct wl_resource *resource;
    uint32_t id;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;

    struct wl_list layer_notifications;
    struct wl_list surface_notifications;
};

struct screenshot_frame_listener {
    struct wl_listener frame_listener;
    struct wl_listener output_destroyed;
    struct wl_resource *screenshot;
    struct weston_output *output;
};

struct screen_id_info {
    char *screen_name;
    uint32_t screen_id;
};

static void
clear_notification_list(struct wl_list* notification_list)
{
    struct notification *not, *next;

    wl_list_for_each_safe(not, next, notification_list, link) {
         wl_list_remove(&not->layout_link);
         wl_list_remove(&not->link);
         free(not);
    }
}

static void
destroy_ivicontroller_screen(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
unbind_resource_controller(struct wl_resource *resource)
{
    struct ivicontroller *controller = wl_resource_get_user_data(resource);

    wl_list_remove(&controller->link);

    clear_notification_list(&controller->layer_notifications);
    clear_notification_list(&controller->surface_notifications);

    free(controller);
    controller = NULL;
}

static struct ivisurface*
get_surface(struct wl_list *list_surf, struct ivi_layout_surface *layout_surface)
{
    struct ivisurface *ivisurf = NULL;

    wl_list_for_each(ivisurf, list_surf, link) {
        if (layout_surface == ivisurf->layout_surface) {
            return ivisurf;
        }
    }

    return NULL;
}

static struct ivilayer*
get_layer(struct wl_list *list_layer, struct ivi_layout_layer *layout_layer)
{
    struct ivilayer *ivilayer = NULL;

    wl_list_for_each(ivilayer, list_layer, link) {
        if (layout_layer == ivilayer->layout_layer) {
            return ivilayer;
        }
    }

    return NULL;
}

static void
send_surface_configure_event(struct ivicontroller * ctrl,
                             struct ivi_layout_surface *layout_surface,
                             uint32_t surface_id)
{
    struct weston_surface *surface;
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;


    surface = lyt->surface_get_weston_surface(layout_surface);

    if (!surface)
        return;

    if ((surface->width == 0) || (surface->height == 0))
        return;

    ivi_wm_send_surface_size(ctrl->resource, surface_id,
                             surface->width, surface->height);
}

static void
send_surface_event(struct ivicontroller * ctrl,
                   struct ivi_layout_surface *layout_surface,
                   uint32_t surface_id,
                   const struct ivi_layout_surface_properties *prop,
                   uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_wm_send_surface_opacity(ctrl->resource, surface_id, prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_wm_send_surface_source_rectangle(ctrl->resource, surface_id,
            prop->source_x, prop->source_y,
            prop->source_width, prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_wm_send_surface_destination_rectangle(ctrl->resource, surface_id,
            prop->dest_x, prop->dest_y,
            prop->dest_width, prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_wm_send_surface_visibility(ctrl->resource, surface_id, prop->visibility);
    }
    if (mask & IVI_NOTIFICATION_CONFIGURE) {
        send_surface_configure_event(ctrl, layout_surface, surface_id);
    }
}

static void
send_surface_prop(struct wl_listener *listener, void *data)
{
    struct ivisurface *ivisurf =
             wl_container_of(listener, ivisurf,
                    property_changed);
    (void)data;
    enum ivi_layout_notification_mask mask;
    struct ivicontroller *ctrl;
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    struct notification *not;
    uint32_t surface_id;

    mask = ivisurf->prop->event_mask;

    surface_id = lyt->get_id_of_surface(ivisurf->layout_surface);

    wl_list_for_each(not, &ivisurf->notification_list, layout_link) {
        ctrl = wl_resource_get_user_data(not->resource);
        send_surface_event(ctrl, ivisurf->layout_surface, surface_id, ivisurf->prop, mask);
    }
}

static void
send_layer_event(struct ivicontroller * ctrl,
                 struct ivi_layout_layer *layout_layer,
                 uint32_t layer_id,
                 const struct ivi_layout_layer_properties *prop,
                 uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_wm_send_layer_opacity(ctrl->resource, layer_id, prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_wm_send_layer_source_rectangle(ctrl->resource, layer_id,
                                           prop->source_x, prop->source_y,
                                           prop->source_width, prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_wm_send_layer_destination_rectangle(ctrl->resource, layer_id,
                                                prop->dest_x, prop->dest_y,
                                                prop->dest_width, prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_wm_send_layer_visibility(ctrl->resource, layer_id, prop->visibility);
    }
}

static void
send_layer_prop(struct wl_listener *listener, void *data)
{
    struct ivilayer *ivilayer =
           wl_container_of(listener, ivilayer, property_changed);
    (void)data;
    enum ivi_layout_notification_mask mask;
    struct ivicontroller *ctrl;
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    struct notification *not;
    uint32_t layer_id;

    mask = ivilayer->prop->event_mask;

    layer_id = lyt->get_id_of_layer(ivilayer->layout_layer);

    wl_list_for_each(not, &ivilayer->notification_list, layout_link) {
        ctrl = wl_resource_get_user_data(not->resource);
        send_layer_event(ctrl, ivilayer->layout_layer, layer_id, ivilayer->prop, mask);
    }
}

static void
controller_set_surface_opacity(struct wl_client *client,
                   struct wl_resource *resource,
                   uint32_t surface_id,
                   wl_fixed_t opacity)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_set_opacity: the surface with given id does not exist");
        return;
    }

    lyt->surface_set_opacity(layout_surface, opacity);
}

static void
controller_set_surface_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   uint32_t surface_id,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;
    const struct ivi_layout_surface_properties *prop;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_set_source_rectangle: the surface with given id does not exist");
        return;
    }

    prop = lyt->get_properties_of_surface(layout_surface);

    if (x < 0)
        x = prop->source_x;
    if (y < 0)
        y = prop->source_y;
    if (width < 0)
        width = prop->source_width;
    if (height < 0)
        height = prop->source_height;

    lyt->surface_set_source_rectangle(layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_set_surface_destination_rectangle(struct wl_client *client,
                     struct wl_resource *resource,
                     uint32_t surface_id,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;
    const struct ivi_layout_surface_properties *prop;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_set_destination_rectangle: the surface with given id does not exist");
        return;
    }

    prop = lyt->get_properties_of_surface(layout_surface);

    // TODO: create set transition type protocol
    lyt->surface_set_transition(layout_surface,
                                     IVI_LAYOUT_TRANSITION_NONE,
                                     300); // ms

    if (x < 0)
        x = prop->dest_x;
    if (y < 0)
        y = prop->dest_y;
    if (width < 0)
        width = prop->dest_width;
    if (height < 0)
        height = prop->dest_height;


    lyt->surface_set_destination_rectangle(layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_set_surface_visibility(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t surface_id,
                      uint32_t visibility)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_set_visibility: the surface with given id does not exist");
        return;
    }

    lyt->surface_set_visibility(layout_surface, visibility);
}

static int
create_screenshot_file(off_t size) {
    const char template[] = "/ivi-shell-screenshot-XXXXXX";
    const char *runtimedir;
    char *tmpname;
    int fd;

    runtimedir = getenv("XDG_RUNTIME_DIR");
    if (runtimedir == NULL)
        return -1;

    tmpname = malloc(strlen(runtimedir) + sizeof(template));
    if (tmpname == NULL)
        return -1;

    fd = mkstemp(strcat(strcpy(tmpname, runtimedir), template));

    if (fd < 0) {
    	free(tmpname);
        return -1;
    }

    unlink(tmpname);
    free(tmpname);

#ifdef HAVE_POSIX_FALLOCATE
    if (posix_fallocate(fd, 0, size)) {
#else
    if (ftruncate(fd, size) < 0) {
#endif
        close(fd);
        return -1;
    }

    return fd;
}

static void
controller_surface_screenshot(struct wl_client *client,
                              struct wl_resource *resource,
                              uint32_t screenshot_id,
                              uint32_t surface_id)
{
    int32_t result = IVI_FAILED;
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct weston_surface *weston_surface = NULL;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    int32_t size = 0;
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    struct ivi_layout_surface *layout_surface;
    char *buffer = NULL;
    struct weston_compositor *compositor = ctrl->shell->compositor;
    // assuming ABGR32 is always written by surface_dump
    uint32_t format = WL_SHM_FORMAT_ABGR8888;
    struct wl_resource *screenshot;
    struct timespec stamp;
    uint32_t stamp_ms;
    int fd;

    screenshot =
        wl_resource_create(client, &ivi_screenshot_interface, 1, screenshot_id);

    if (screenshot == NULL) {
        wl_client_post_no_memory(client);
        return;
    }

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_screenshot_send_error(
            screenshot, IVI_SCREENSHOT_ERROR_NO_SURFACE,
            "surface_screenshot: the surface with given id does not exist");
        goto err;
    }

    result = lyt->surface_get_size(layout_surface, &width,
                                   &height, &stride);
    if ((result != IVI_SUCCEEDED) || !width || !height || !stride) {
        ivi_screenshot_send_error(
            screenshot, IVI_SCREENSHOT_ERROR_NO_CONTENT,
            "surface_screenshot: surface does not have content");
        goto err;
    }

    size = stride * height;

    fd = create_screenshot_file(size);
    if (fd < 0) {
        weston_log(
            "surface_screenshot: failed to create file of %d bytes: %m\n",
            size);
        ivi_screenshot_send_error(
            screenshot, IVI_SCREENSHOT_ERROR_IO_ERROR,
            "failed to create screenshot file");
        goto err;
    }

    buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        weston_log("surface_screenshot: failed to mmap %d bytes: %m\n", size);
        ivi_screenshot_send_error(screenshot, IVI_SCREENSHOT_ERROR_IO_ERROR,
                                  "failed to create screenshot");
        goto err_mmap;
    }

    weston_surface = lyt->surface_get_weston_surface(layout_surface);

    result = lyt->surface_dump(weston_surface, buffer, size, 0, 0,
                               width, height);

    if (result != IVI_SUCCEEDED) {
        ivi_screenshot_send_error(
            resource, IVI_SCREENSHOT_ERROR_NOT_SUPPORTED,
            "surface_screenshot: surface dumping is not supported by renderer");
        goto err_readpix;
    }

    // get current timestamp
    weston_compositor_read_presentation_clock(compositor, &stamp);
    stamp_ms = stamp.tv_sec * 1000 + stamp.tv_nsec / 1000000;

    ivi_screenshot_send_done(screenshot, fd, width, height, stride, format,
                             stamp_ms);

err_readpix:
    munmap(buffer, size);
err_mmap:
    close(fd);
err:
    wl_resource_destroy(screenshot);
}


static void
send_surface_stats(struct ivicontroller *ctrl,
                   struct ivi_layout_surface *layout_surface,
                   uint32_t surface_id)
{
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    struct ivisurface *ivisurf;
    struct weston_surface *surface;
    struct wl_client* target_client;
    pid_t pid;
    uid_t uid;
    gid_t gid;

    ivisurf = get_surface(&ctrl->shell->list_surface, layout_surface);

    /* Get pid that creates surface */
    surface = lyt->surface_get_weston_surface(layout_surface);

    target_client = wl_resource_get_client(surface->resource);

    wl_client_get_credentials(target_client, &pid, &uid, &gid);

    ivi_wm_send_surface_stats(ctrl->resource, surface_id, ivisurf->frame_count, pid);
}

static void
controller_surface_sync(struct wl_client *client,
                              struct wl_resource *resource,
                              uint32_t surface_id,
                              int32_t sync_state)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    struct ivi_layout_surface *layout_surface;
    struct ivisurface *ivisurf;
    (void)client;
    struct notification *not;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_sync: the surface with given id does not exist");
        return;
    }

    ivisurf = get_surface(&ctrl->shell->list_surface, layout_surface);

    switch (sync_state) {
    case IVI_WM_SYNC_ADD:
        /*Check if a notification for the surface is already initialized*/
        not= calloc(1, sizeof *not);
        if (not == NULL) {
            wl_resource_post_no_memory(resource);
            return;
        }

        wl_list_insert(&ctrl->surface_notifications, &not->link);
        wl_list_insert(&ivisurf->notification_list, &not->layout_link);
        not->resource = resource;
        break;
    case IVI_WM_SYNC_REMOVE:
        wl_list_for_each(not, &ivisurf->notification_list, layout_link)
        {
            if (not->resource == resource) {
                wl_list_remove(&not->link);
                wl_list_remove(&not->layout_link);
                free(not);
                break;
            }
        }
        break;
    default:
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_BAD_PARAM,
                                  "surface_sync: invalid sync_state parameter");
    }
}

static void
controller_set_surface_type(struct wl_client *client, struct wl_resource *resource,
                            uint32_t surface_id, int32_t type)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;
    struct ivisurface *ivisurf;

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_set_type: the surface with given id does not exist");
        return;
    }

    ivisurf = get_surface(&ctrl->shell->list_surface, layout_surface);
    ivisurf->type = type;
}

static int32_t
convert_protocol_enum(int32_t param)
{
    int32_t mask = 0;

    if (param & IVI_WM_PARAM_OPACITY)
        mask |= IVI_NOTIFICATION_OPACITY;

    if (param & IVI_WM_PARAM_VISIBILITY)
        mask |= IVI_NOTIFICATION_VISIBILITY;

    if (param & IVI_WM_PARAM_SIZE) {
        mask |= IVI_NOTIFICATION_SOURCE_RECT;
        mask |= IVI_NOTIFICATION_DEST_RECT;
        mask |= IVI_NOTIFICATION_CONFIGURE;
    }

    return mask;
}

static void
controller_surface_get(struct wl_client *client, struct wl_resource *resource,
                            uint32_t surface_id, int32_t param)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_surface *layout_surface;
    enum ivi_layout_notification_mask mask;
    const struct ivi_layout_surface_properties *prop;

    mask = convert_protocol_enum(param);

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_surface_error(resource, surface_id,
                                  IVI_WM_SURFACE_ERROR_NO_SURFACE,
                                  "surface_get: the surface with given id does not exist");
        return;
    }

    prop = lyt->get_properties_of_surface(layout_surface);

    send_surface_event(ctrl, layout_surface, surface_id, prop, mask);
    send_surface_stats(ctrl, layout_surface, surface_id);
}

static void
controller_set_layer_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   uint32_t layer_id,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;
    const struct ivi_layout_layer_properties *prop;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "set_layer_source_rectangle: the layer with given id does not exist");
        return;
    }

    prop = lyt->get_properties_of_layer(layout_layer);

    if (x < 0)
        x = prop->source_x;
    if (y < 0)
        y = prop->source_y;
    if (width < 0)
        width = prop->source_width;
    if (height < 0)
        height = prop->source_height;

    lyt->layer_set_source_rectangle(layout_layer,
           (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_set_layer_destination_rectangle(struct wl_client *client,
                 struct wl_resource *resource,
                 uint32_t layer_id,
                 int32_t x,
                 int32_t y,
                 int32_t width,
                 int32_t height)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;
    const struct ivi_layout_layer_properties *prop;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "set_layer_destination_rectangle: the layer with given id does not exist");
        return;
    }

    prop = lyt->get_properties_of_layer(layout_layer);

    if (x < 0)
        x = prop->dest_x;
    if (y < 0)
        y = prop->dest_y;
    if (width < 0)
        width = prop->dest_width;
    if (height < 0)
        height = prop->dest_height;

    lyt->layer_set_destination_rectangle(layout_layer,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_set_layer_visibility(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t layer_id,
                    uint32_t visibility)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_set_visibility: the layer with given id does not exist");
        return;
    }

    lyt->layer_set_visibility(layout_layer, visibility);
}

static void
controller_set_layer_opacity(struct wl_client *client,
                 struct wl_resource *resource,
                 uint32_t layer_id,
                 wl_fixed_t opacity)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_set_opacity: the layer with given id does not exist");
        return;
    }

    lyt->layer_set_opacity(layout_layer, opacity);
}

static void
controller_layer_clear(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t layer_id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_clear: the layer with given id does not exist");
        return;
    }

    lyt->layer_set_render_order(layout_layer, NULL, 0);
}

static void
calc_trans_matrix(struct weston_geometry *source_rect,
		   struct weston_geometry *dest_rect,
		   struct weston_matrix *m)
{
    float source_center_x;
    float source_center_y;
    float scale_x;
    float scale_y;
    float translate_x;
    float translate_y;

    source_center_x = source_rect->x + source_rect->width * 0.5f;
    source_center_y = source_rect->y + source_rect->height * 0.5f;
    weston_matrix_translate(m, -source_center_x, -source_center_y, 0.0f);

    if ((dest_rect->width != source_rect->width) ||
        (dest_rect->height != source_rect->height))
    {
        scale_x = ((float)dest_rect->width) / source_rect->width;
        scale_y = ((float)dest_rect->height) / source_rect->height;

        weston_matrix_scale(m, scale_x, scale_y, 1.0f);
    }

    translate_x = dest_rect->width * 0.5f + dest_rect->x;
    translate_y = dest_rect->height * 0.5f + dest_rect->y;
    weston_matrix_translate(m, translate_x, translate_y, 0.0f);
}

void
set_bkgnd_surface_prop(struct ivishell *shell)
{
    struct weston_view *view;
    struct weston_compositor *compositor;
    struct weston_output *output;
    struct weston_surface *w_surface;
    struct weston_geometry source_rect = {0};
    struct weston_geometry dest_rect = {0};
    int32_t src_width = 0;
    int32_t src_height = 0;
    int32_t dest_width = 0;
    int32_t dest_height = 0;
    uint32_t count = 0;
    int32_t x = 0;
    int32_t y = 0;

    view = shell->bkgnd_view;
    compositor = shell->compositor;

    wl_list_remove(&shell->bkgnd_transform.link);
    weston_matrix_init(&shell->bkgnd_transform.matrix);

    /*find the available screen's resolution*/
    wl_list_for_each(output, &compositor->output_list, link) {
        if (!count)
        {
            x = output->x;
            y = output->y;
            count++;
        }
        dest_width = output->x + output->width;
        if (output->height > dest_height)
            dest_height = output->height;
        weston_log("set_bkgnd_surface_prop: o_name:%s x:%d y:%d o_width:%d o_height:%d\n",
                   output->name, output->x, output->y, output->width, output->height);
    }

    w_surface = view->surface;
    src_width = w_surface->width;
    src_height = w_surface->height;

    source_rect.width = src_width;
    source_rect.height = src_height;
    dest_rect.width = dest_width;
    dest_rect.height = dest_height;

    calc_trans_matrix(&source_rect, &dest_rect,
                      &shell->bkgnd_transform.matrix);
    weston_matrix_translate(&shell->bkgnd_transform.matrix, x, y, 0.0f);

    weston_log("set_bkgnd_surface_prop: x:%d y:%d s_width:%d s_height:%d d_width:%d d_height:%d\n",
               x, y, src_width, src_height, dest_width, dest_height);

    wl_list_insert(&view->geometry.transformation_list,
                   &shell->bkgnd_transform.link);
    weston_view_update_transform(view);
    weston_surface_schedule_repaint(w_surface);
}

static void
controller_layer_add_surface(struct wl_client *client,
                 struct wl_resource *resource,
                 uint32_t layer_id,
                 uint32_t surface_id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;
    struct ivi_layout_surface *layout_surface;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_add_surface: the layer with given id does not exist");
        return;
    }

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_layer_error(resource, surface_id,
                                IVI_WM_LAYER_ERROR_NO_SURFACE,
                                "layer_add_surface: the surface with given id does not exist");
        return;
    }

    lyt->layer_add_surface(layout_layer, layout_surface);
}

static void
controller_layer_remove_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t layer_id,
                    uint32_t surface_id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;
    struct ivi_layout_surface *layout_surface;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_remove_surface: the layer with given id does not exist");
        return;
    }

    layout_surface = lyt->get_surface_from_id(surface_id);
    if (!layout_surface) {
        ivi_wm_send_layer_error(resource, surface_id,
                                IVI_WM_LAYER_ERROR_NO_SURFACE,
                                "layer_remove_surface: the surface with given id does not exist");
        return;
    }

    lyt->layer_remove_surface(layout_layer, layout_surface);
}

static void
controller_layer_sync(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t layer_id,
                      int32_t sync_state)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    struct ivi_layout_layer *layout_layer;
    struct ivilayer *ivilayer;
    (void)client;
    struct notification *not;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer sync: the layer with given id does not exist");
        return;
    }

    ivilayer = get_layer(&ctrl->shell->list_layer, layout_layer);

    switch (sync_state) {
    case IVI_WM_SYNC_ADD:
        /*Check if a notification for the surface is already initialized*/
        not= calloc(1, sizeof *not);
        if (not == NULL) {
            wl_resource_post_no_memory(resource);
            return;
        }

        wl_list_insert(&ctrl->layer_notifications, &not->link);
        wl_list_insert(&ivilayer->notification_list, &not->layout_link);
        not->resource = resource;
        break;
    case IVI_WM_SYNC_REMOVE:
        ivilayer = get_layer(&ctrl->shell->list_layer, layout_layer);

        wl_list_for_each(not, &ivilayer->notification_list, layout_link)
        {
            if (not->resource == resource) {
                wl_list_remove(&not->link);
                wl_list_remove(&not->layout_link);
                free(not);
                break;
            }
        }
        break;
    default:
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_BAD_PARAM,
                                "layer sync: invalid sync_state param");
        return;
    }
}

static void
controller_create_layout_layer(struct wl_client *client,
                    struct wl_resource *resource, uint32_t layer_id,
                    int width, int height)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;

     if(lyt->layer_create_with_dimension(layer_id, width, height) == NULL) {
         wl_resource_post_no_memory(resource);
     }
}

static void
controller_destroy_layout_layer(struct wl_client *client,
                    struct wl_resource *resource, uint32_t layer_id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "destroy_layout_layer: the layer with given id does not exist");
        return;
    }

    lyt->layer_destroy(layout_layer);
}

static void
controller_layer_get(struct wl_client *client, struct wl_resource *resource,
                     uint32_t layer_id, int32_t param)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ctrl->shell->interface;
    (void)client;
    struct ivi_layout_layer *layout_layer;
    const struct ivi_layout_layer_properties *prop;
    enum ivi_layout_notification_mask mask;
    struct ivi_layout_surface **surf_list = NULL;
    int32_t surface_count, i;
    uint32_t id;

    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_send_layer_error(resource, layer_id,
                                IVI_WM_LAYER_ERROR_NO_LAYER,
                                "layer_get: the layer with given id does not exist");
        return;
    }

    mask = convert_protocol_enum(param);
    prop = lyt->get_properties_of_layer(layout_layer);
    send_layer_event(ctrl, layout_layer, layer_id, prop, mask);

    if (param & IVI_WM_PARAM_RENDER_ORDER) {
        lyt->get_surfaces_on_layer(layout_layer, &surface_count, &surf_list);
        for (i = 0; i < surface_count; i++) {
            id = lyt->get_id_of_surface(surf_list[i]);
            ivi_wm_send_layer_surface_added(ctrl->resource, layer_id, id);
        }

        free(surf_list);
    }
}

static void
controller_screen_destroy(struct wl_client *client,
                          struct wl_resource *resource)
{
    (void)client;
    wl_resource_destroy(resource);
}

static void
controller_screen_clear(struct wl_client *client,
                struct wl_resource *resource)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt;
    (void)client;

    if (!iviscrn) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_SCREEN,
                                 "the output is already destroyed");
        return;
    }

    lyt = iviscrn->shell->interface;
    lyt->screen_set_render_order(iviscrn->output, NULL, 0);
}

static void
controller_screen_add_layer(struct wl_client *client,
                struct wl_resource *resource,
                uint32_t layer_id)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    if (!iviscrn) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_SCREEN,
                                 "the output is already destroyed");
        return;
    }

    lyt = iviscrn->shell->interface;
    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_LAYER,
                                 "the layer with given id does not exist");
        weston_log("ivi-controller: an ivi-layer with id: %d does not exist\n", layer_id);
        return;
    }

    lyt->screen_add_layer(iviscrn->output, layout_layer);
}

static void
controller_screen_remove_layer(struct wl_client *client,
                struct wl_resource *resource,
                uint32_t layer_id)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt;
    (void)client;
    struct ivi_layout_layer *layout_layer;

    if (!iviscrn) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_SCREEN,
                                 "the output is already destroyed");
        return;
    }

    lyt = iviscrn->shell->interface;
    layout_layer = lyt->get_layer_from_id(layer_id);
    if (!layout_layer) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_LAYER,
                                 "the layer with given id does not exist");
        weston_log("ivi-controller: an ivi-layer with id: %d does not exist\n", layer_id);
        return;
    }

    lyt->screen_remove_layer(iviscrn->output, layout_layer);
}

static void
flip_y(int32_t stride, int32_t height, uint32_t *data) {
    int i, y, p, q;
    // assuming stride aligned to 4 bytes
    int pitch = stride / sizeof(*data);
    for (y = 0; y < height / 2; ++y) {
        p = y * pitch;
        q = (height - y - 1) * pitch;
        for (i = 0; i < pitch; ++i) {
            uint32_t tmp = data[p + i];
            data[p + i] = data[q + i];
            data[q + i] = tmp;
        }
    }
}

static void
controller_screenshot_notify(struct wl_listener *listener, void *data)
{
    struct screenshot_frame_listener *l =
        wl_container_of(listener, l, frame_listener);

    struct weston_output *output = l->output;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    uint32_t *readpixs = NULL;
    uint32_t shm_format;
    int fd;
    size_t size;
    pixman_format_code_t format = output->compositor->read_format;

    --output->disable_planes;

    // map to shm buffer format
    switch (format) {
    case PIXMAN_a8r8g8b8:
        shm_format = WL_SHM_FORMAT_ARGB8888;
        break;
    case PIXMAN_x8r8g8b8:
        shm_format = WL_SHM_FORMAT_XRGB8888;
        break;
    case PIXMAN_a8b8g8r8:
        shm_format = WL_SHM_FORMAT_ABGR8888;
        break;
    case PIXMAN_x8b8g8r8:
        shm_format = WL_SHM_FORMAT_XBGR8888;
        break;
    default:
        ivi_screenshot_send_error(l->screenshot,
                                  IVI_SCREENSHOT_ERROR_NOT_SUPPORTED,
                                  "unsupported pixel format");
        goto err_fd;
    }

    width = output->current_mode->width;
    height = output->current_mode->height;
    stride = width * (PIXMAN_FORMAT_BPP(format) / 8);
    size = stride * height;

    fd = create_screenshot_file(size);
    if (fd < 0) {
        weston_log("screenshot: failed to create file of %zu bytes: %m\n",
                   size);
        ivi_screenshot_send_error(l->screenshot, IVI_SCREENSHOT_ERROR_IO_ERROR,
                                  "failed to create screenshot file");
        goto err_fd;
    }

    readpixs = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (readpixs == MAP_FAILED) {
        weston_log("screenshot: failed to mmap %zu bytes: %m\n", size);
        ivi_screenshot_send_error(l->screenshot, IVI_SCREENSHOT_ERROR_IO_ERROR,
                                  "failed to create screenshot");
        goto err_mmap;
    }

    if (output->compositor->renderer->read_pixels(output, format, readpixs,
                                                  0, 0, width, height) < 0) {
        ivi_screenshot_send_error(
            l->screenshot, IVI_SCREENSHOT_ERROR_NOT_SUPPORTED,
            "screenshot of given output is not supported by renderer");
        goto err_readpix;
    }

    if (output->compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP)
        flip_y(stride, height, readpixs);

    ivi_screenshot_send_done(l->screenshot, fd, width, height, stride,
                             shm_format, timespec_to_msec(&output->frame_time));

err_readpix:
    munmap(readpixs, size);
err_mmap:
    close(fd);
err_fd:
    wl_resource_destroy(l->screenshot);
}

static void
screenshot_output_destroyed(struct wl_listener *listener, void *data)
{
    struct screenshot_frame_listener *l =
        wl_container_of(listener, l, output_destroyed);

    ivi_screenshot_send_error(l->screenshot, IVI_SCREENSHOT_ERROR_NO_OUTPUT,
                              "the output has been destroyed");
    wl_resource_destroy(l->screenshot);
}

static void
screenshot_frame_listener_destroy(struct wl_resource *resource)
{
    struct screenshot_frame_listener *l = wl_resource_get_user_data(resource);

    wl_list_remove(&l->frame_listener.link);
    wl_list_remove(&l->output_destroyed.link);
    free(l);
}

static void
controller_screen_screenshot(struct wl_client *client,
                             struct wl_resource *resource,
                             uint32_t id)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct screenshot_frame_listener *l;
    (void)client;

    l = malloc(sizeof *l);
    if(l == NULL) {
        wl_resource_post_no_memory(resource);
        return;
    }

    l->screenshot =
        wl_resource_create(client, &ivi_screenshot_interface, 1, id);

    if (l->screenshot == NULL) {
        wl_resource_post_no_memory(resource);
        free(l);
        return;
    }

    if (!iviscrn) {
        ivi_screenshot_send_error(l->screenshot, IVI_SCREENSHOT_ERROR_NO_OUTPUT,
                                  "the output is already destroyed");
        wl_resource_destroy(l->screenshot);
        free(l);
        return;
    }

    l->output = iviscrn->output;

    wl_resource_set_implementation(l->screenshot, NULL, l,
                                   screenshot_frame_listener_destroy);
    l->output_destroyed.notify = screenshot_output_destroyed;
    wl_signal_add(&iviscrn->output->destroy_signal, &l->output_destroyed);
    l->frame_listener.notify = controller_screenshot_notify;
    wl_signal_add(&iviscrn->output->frame_signal, &l->frame_listener);
    iviscrn->output->disable_planes++;
    weston_output_damage(iviscrn->output);
}

static void
controller_screen_get(struct wl_client *client,
                       struct wl_resource *resource,
                       int32_t param)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt;
    (void)client;
    struct ivi_layout_layer **layer_list = NULL;
    int32_t layer_count, i;
    uint32_t id;

    lyt = iviscrn->shell->interface;

    if (!iviscrn) {
        ivi_wm_screen_send_error(resource, IVI_WM_SCREEN_ERROR_NO_SCREEN,
                                 "the output is already destroyed");
        return;
    }

    if (param & IVI_WM_PARAM_RENDER_ORDER) {
        lyt->get_layers_on_screen(iviscrn->output, &layer_count, &layer_list);

        for (i = 0; i < layer_count; i++) {
            id = lyt->get_id_of_layer(layer_list[i]);
            ivi_wm_screen_send_layer_added(resource, id);
	}

        free(layer_list);
    }
}

static const
struct ivi_wm_screen_interface controller_screen_implementation = {
    controller_screen_destroy,
    controller_screen_clear,
    controller_screen_add_layer,
    controller_screen_remove_layer,
    controller_screen_screenshot,
    controller_screen_get
};

static void
controller_commit_changes(struct wl_client *client,
                          struct wl_resource *resource)
{
    int32_t ans = 0;
    (void)client;
    struct ivicontroller *controller = wl_resource_get_user_data(resource);

    ans = controller->shell->interface->commit_changes();
    if (ans < 0) {
        weston_log("Failed to commit changes at controller_commit_changes\n");
    }
}

static void
controller_create_screen(struct wl_client *client,
                        struct wl_resource *resource,
                        struct wl_resource *output_resource,
                        uint32_t id)
{
    struct weston_head *weston_head =
        wl_resource_get_user_data(output_resource);
    struct wl_resource *screen_resource;
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct iviscreen* iviscrn = NULL;


    wl_list_for_each(iviscrn, &ctrl->shell->list_screen, link) {
        if (weston_head->output != iviscrn->output) {
            continue;
        }

        screen_resource = wl_resource_create(client, &ivi_wm_screen_interface, 1, id);
        if (screen_resource == NULL) {
            wl_resource_post_no_memory(resource);
            return;
        }

        wl_resource_set_implementation(screen_resource,
                                       &controller_screen_implementation,
                                       iviscrn, destroy_ivicontroller_screen);

        wl_list_insert(&iviscrn->resource_list, wl_resource_get_link(screen_resource));

        ivi_wm_screen_send_screen_id(screen_resource, iviscrn->id_screen);
        ivi_wm_screen_send_connector_name(screen_resource, iviscrn->output->name);
    }
}

static const struct ivi_wm_interface controller_implementation = {
    controller_commit_changes,
    controller_create_screen,
    controller_set_surface_visibility,
    controller_set_layer_visibility,
    controller_set_surface_opacity,
    controller_set_layer_opacity,
    controller_set_surface_source_rectangle,
    controller_set_layer_source_rectangle,
    controller_set_surface_destination_rectangle,
    controller_set_layer_destination_rectangle,
    controller_surface_sync,
    controller_layer_sync,
    controller_surface_get,
    controller_layer_get,
    controller_surface_screenshot,
    controller_set_surface_type,
    controller_layer_clear,
    controller_layer_add_surface,
    controller_layer_remove_surface,
    controller_create_layout_layer,
    controller_destroy_layout_layer
};

static void
bind_ivi_controller(struct wl_client *client, void *data,
                    uint32_t version, uint32_t id)
{
    struct ivishell *shell = data;
    struct ivicontroller *controller;
    (void)version;
    uint32_t surface_id, layer_id;
    struct ivisurface *ivisurf;
    struct ivilayer *ivilayer;

    controller = calloc(1, sizeof *controller);
    if (controller == NULL) {
        wl_client_post_no_memory(client);
        return;
    }

    controller->resource =
        wl_resource_create(client, &ivi_wm_interface, 1, id);
    if (controller->resource == NULL) {
        wl_client_post_no_memory(client);
        free(controller);
        return;
    }

    wl_resource_set_implementation(controller->resource,
                                   &controller_implementation,
                                   controller, unbind_resource_controller);

    controller->shell = shell;
    controller->client = client;
    controller->id = id;

    wl_list_insert(&shell->list_controller, &controller->link);
    wl_list_init(&controller->surface_notifications);
    wl_list_init(&controller->layer_notifications);

    wl_list_for_each_reverse(ivisurf, &shell->list_surface, link) {
        surface_id = shell->interface->get_id_of_surface(ivisurf->layout_surface);
        ivi_wm_send_surface_created(controller->resource, surface_id);
    }

    wl_list_for_each_reverse(ivilayer, &shell->list_layer, link) {
        layer_id = shell->interface->get_id_of_layer(ivilayer->layout_layer);
        ivi_wm_send_layer_created(controller->resource, layer_id);
    }
}

static void
create_screen(struct ivishell *shell, struct weston_output *output)
{
    struct iviscreen *iviscrn;
    struct screen_id_info *screen_info;
    uint32_t id;

    iviscrn = calloc(1, sizeof *iviscrn);
    if (iviscrn == NULL) {
        weston_log("no memory to allocate client screen\n");
        return;
    }

    id = output->id + shell->screen_id_offset;

    wl_array_for_each(screen_info, &shell->screen_ids) {
        if(!strcmp(screen_info->screen_name, output->name))
        {
            id = screen_info->screen_id;
            break;
        }
    }

    iviscrn->shell = shell;
    iviscrn->output = output;

    iviscrn->id_screen = id;

    wl_list_insert(&shell->list_screen, &iviscrn->link);
    wl_list_init(&iviscrn->resource_list);

    return;
}

static void
destroy_screen(struct iviscreen *iviscrn)
{
    struct wl_resource *resource, *next;

    wl_resource_for_each_safe(resource, next, &iviscrn->resource_list) {
        wl_resource_set_destructor(resource, NULL);
        wl_resource_destroy(resource);
    }

    wl_list_remove(&iviscrn->link);
    free(iviscrn);
}

static void
output_destroyed_event(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, output_destroyed);
    struct iviscreen *iviscrn = NULL;
    struct iviscreen *next = NULL;
    struct weston_output *destroyed_output = (struct weston_output*)data;

    wl_list_for_each_safe(iviscrn, next, &shell->list_screen, link) {
        if (iviscrn->output == destroyed_output)
            destroy_screen(iviscrn);
    }

    if (shell->bkgnd_view && shell->client)
        set_bkgnd_surface_prop(shell);
    else
        weston_compositor_schedule_repaint(shell->compositor);
}

static void
output_resized_event(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, output_destroyed);

    if (shell->bkgnd_view && shell->client)
        set_bkgnd_surface_prop(shell);
}

static void
output_created_event(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, output_created);
    struct weston_output *created_output = (struct weston_output*)data;

    create_screen(shell, created_output);

    if (shell->bkgnd_view && shell->client)
        set_bkgnd_surface_prop(shell);
    else
        weston_compositor_schedule_repaint(shell->compositor);
}

static struct ivilayer*
create_layer(struct ivishell *shell,
             struct ivi_layout_layer *layout_layer,
             uint32_t id_layer)
{
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivilayer *ivilayer = NULL;
    struct ivicontroller *controller = NULL;

    ivilayer = calloc(1, sizeof *ivilayer);
    if (NULL == ivilayer) {
        weston_log("no memory to allocate client layer\n");
        return NULL;
    }

    ivilayer->shell = shell;
    wl_list_insert(&shell->list_layer, &ivilayer->link);
    wl_list_init(&ivilayer->notification_list);
    ivilayer->layout_layer = layout_layer;
    ivilayer->prop = lyt->get_properties_of_layer(layout_layer);

    ivilayer->property_changed.notify = send_layer_prop;
    lyt->layer_add_listener(layout_layer, &ivilayer->property_changed);

    wl_list_for_each(controller, &shell->list_controller, link) {
        if (controller->resource)
            ivi_wm_send_layer_created(controller->resource, id_layer);
    }

    return ivilayer;
}

static void
surface_committed(struct wl_listener *listener, void *data)
{
    struct ivisurface *ivisurf = wl_container_of(listener, ivisurf, committed);
    (void)data;

    ivisurf->frame_count++;
}

static struct ivisurface*
create_surface(struct ivishell *shell,
               struct ivi_layout_surface *layout_surface,
               uint32_t id_surface)
{
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller *controller = NULL;
    struct weston_surface *surface;

    ivisurf = calloc(1, sizeof *ivisurf);
    if (ivisurf == NULL) {
        weston_log("no memory to allocate client surface\n");
        return NULL;
    }

    ivisurf->shell = shell;
    ivisurf->layout_surface = layout_surface;
    ivisurf->prop = lyt->get_properties_of_surface(layout_surface);
    wl_list_init(&ivisurf->notification_list);

    ivisurf->committed.notify = surface_committed;
    surface = lyt->surface_get_weston_surface(layout_surface);
    wl_signal_add(&surface->commit_signal, &ivisurf->committed);

    if (shell->bkgnd_surface_id != (int32_t)id_surface) {
        wl_list_insert(&shell->list_surface, &ivisurf->link);

        wl_list_for_each(controller, &shell->list_controller, link) {
            if (controller->resource)
                ivi_wm_send_surface_created(controller->resource, id_surface);
            }

        ivisurf->property_changed.notify = send_surface_prop;
        lyt->surface_add_listener(layout_surface, &ivisurf->property_changed);
    }
    else {
        shell->bkgnd_surface = ivisurf;
    }

    return ivisurf;
}

static void
layer_event_create(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, layer_created);
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivilayer *ivilayer = NULL;
    struct ivi_layout_layer *layout_layer =
           (struct ivi_layout_layer *) data;
    uint32_t id_layer = 0;

    id_layer = lyt->get_id_of_layer(layout_layer);

    ivilayer = create_layer(shell, layout_layer, id_layer);
    if (ivilayer == NULL) {
        weston_log("failed to create layer");
        return;
    }
}

static void
layer_event_remove(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, layer_removed);
    struct ivilayer *ivilayer = NULL;
    struct ivicontroller *controller = NULL;
    struct ivi_layout_layer *layout_layer =
           (struct ivi_layout_layer *) data;
    uint32_t id_layer = 0;
    struct notification *not, *next;

    ivilayer = get_layer(&shell->list_layer, layout_layer);
    if (ivilayer == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    wl_list_for_each_safe(not, next, &ivilayer->notification_list, layout_link)
    {
        wl_list_remove(&not->link);
        wl_list_remove(&not->layout_link);
        free(not);
    }

    wl_list_remove(&ivilayer->link);
    wl_list_remove(&ivilayer->property_changed.link);
    free(ivilayer);

    id_layer = shell->interface->get_id_of_layer(layout_layer);

    wl_list_for_each(controller, &shell->list_controller, link) {
        if (controller->resource)
            ivi_wm_send_layer_destroyed(controller->resource, id_layer);
    }
}


static void
surface_event_create(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, surface_created);
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivisurface *ivisurf = NULL;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    uint32_t id_surface = 0;

    id_surface = lyt->get_id_of_surface(layout_surface);

    ivisurf = create_surface(shell, layout_surface, id_surface);
    if (ivisurf == NULL) {
        weston_log("failed to create surface");
        return;
    }

    if (shell->bkgnd_surface_id != (int32_t)id_surface)
        wl_signal_emit(&shell->ivisurface_created_signal, ivisurf);
}

static void
surface_event_remove(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, surface_removed);
    struct ivicontroller *controller = NULL;
    struct ivisurface *ivisurf = NULL;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    uint32_t id_surface = 0;
    struct notification *not, *next;

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    wl_signal_emit(&shell->ivisurface_removed_signal, ivisurf);
    wl_list_for_each_safe(not, next, &ivisurf->notification_list, layout_link)
    {
        wl_list_remove(&not->link);
        wl_list_remove(&not->layout_link);
        free(not);
    }

    wl_list_remove(&ivisurf->link);
    wl_list_remove(&ivisurf->property_changed.link);
    wl_list_remove(&ivisurf->committed.link);
    free(ivisurf);

    id_surface = shell->interface->get_id_of_surface(layout_surface);

    if ((shell->bkgnd_surface_id == (int32_t)id_surface) &&
         shell->bkgnd_view) {
        weston_layer_entry_remove(&shell->bkgnd_view->layer_link);
        weston_view_destroy(shell->bkgnd_view);
    }

    wl_list_for_each(controller, &shell->list_controller, link) {
        if (controller->resource)
            ivi_wm_send_surface_destroyed(controller->resource, id_surface);
    }
}

static void
surface_event_configure(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, surface_configured);
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivisurface *ivisurf = NULL;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    struct ivicontroller *ctrl;
    struct notification *not;
    uint32_t surface_id;
    struct weston_surface *w_surface;

    surface_id = lyt->get_id_of_surface(layout_surface);
    if (shell->bkgnd_surface_id == (int32_t)surface_id) {
        float red, green, blue, alpha;

        if (!shell->bkgnd_view) {
            w_surface = lyt->surface_get_weston_surface(layout_surface);

            alpha = ((shell->bkgnd_color >> 24) & 0xFF) / 255.0F;
            red = ((shell->bkgnd_color >> 16) & 0xFF) / 255.0F;
            green = ((shell->bkgnd_color >> 8) & 0xFF) / 255.0F;
            blue = (shell->bkgnd_color & 0xFF) / 255.0F;

            weston_surface_set_color(w_surface, red, green, blue, alpha);

            wl_list_init(&shell->bkgnd_transform.link);
            shell->bkgnd_view = weston_view_create(w_surface);
            weston_layer_entry_insert(&shell->bkgnd_layer.view_list,
                                      &shell->bkgnd_view->layer_link);
        }

        set_bkgnd_surface_prop(shell);
        return;
    }

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    if (ivisurf->type == IVI_WM_SURFACE_TYPE_DESKTOP) {
        w_surface = lyt->surface_get_weston_surface(layout_surface);
        lyt->surface_set_destination_rectangle(layout_surface,
                                               ivisurf->prop->dest_x,
                                               ivisurf->prop->dest_y,
                                               w_surface->width,
                                               w_surface->height);
        lyt->surface_set_source_rectangle(layout_surface,
                                          0,
                                          0,
                                          w_surface->width,
                                          w_surface->height);
        lyt->commit_changes();
    }

    wl_list_for_each(not, &ivisurf->notification_list, layout_link) {
        ctrl = wl_resource_get_user_data(not->resource);
        send_surface_event(ctrl, ivisurf->layout_surface, surface_id, ivisurf->prop,
                           IVI_NOTIFICATION_CONFIGURE);
    }
}

static int32_t
check_layout_layers(struct ivishell *shell)
{
    struct ivi_layout_layer **pArray = NULL;
    struct ivilayer *ivilayer = NULL;
    const struct ivi_layout_interface *lyt = shell->interface;
    uint32_t id_layer = 0;
    int32_t length = 0;
    int32_t i = 0;
    int32_t ret = 0;

    ret = lyt->get_layers(&length, &pArray);
    if(ret != 0) {
        weston_log("failed to get layers at check_layout_layers\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_layer = lyt->get_id_of_layer(pArray[i]);
        ivilayer = create_layer(shell, pArray[i], id_layer);
        if (ivilayer == NULL) {
            weston_log("failed to create layer");
        }
    }

    free(pArray);
    pArray = NULL;

    return 0;
}

static int32_t
check_layout_surfaces(struct ivishell *shell)
{
    struct ivi_layout_surface **pArray = NULL;
    struct ivisurface *ivisurf = NULL;
    const struct ivi_layout_interface *lyt = shell->interface;
    uint32_t id_surface = 0;
    int32_t length = 0;
    int32_t i = 0;
    int32_t ret = 0;

    ret = lyt->get_surfaces(&length, &pArray);
    if(ret != 0) {
        weston_log("failed to get surfaces at check_layout_surfaces\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_surface = lyt->get_id_of_surface(pArray[i]);
        ivisurf = create_surface(shell, pArray[i], id_surface);
        if (ivisurf == NULL) {
            weston_log("failed to create surface");
        }
    }

    free(pArray);
    pArray = NULL;

    return 0;
}

static void
destroy_screen_ids(struct ivishell *shell)
{
	struct screen_id_info *screen_info = NULL;

	wl_array_for_each(screen_info, &shell->screen_ids) {
		free(screen_info->screen_name);
	}

	wl_array_release(&shell->screen_ids);
}

static void
get_config(struct weston_compositor *compositor, struct ivishell *shell)
{
	struct weston_config_section *section = NULL;
	struct weston_config *config = NULL;
	struct screen_id_info *screen_info = NULL;
	const char *name = NULL;

	config = wet_get_config(compositor);
	if (!config)
		return;

	section = weston_config_get_section(config, "ivi-shell", NULL, NULL);
	if (!section)
		return;

	weston_config_section_get_uint(section,
				       "screen-id-offset",
				       &shell->screen_id_offset, 0);

	weston_config_section_get_string(section,
                       "ivi-client-name",
                       &shell->ivi_client_name, NULL);

	weston_config_section_get_int(section,
                       "bkgnd-surface-id",
                       &shell->bkgnd_surface_id, -1);

	weston_config_section_get_string(section,
	                   "debug-scopes",
	                   &shell->debug_scopes, NULL);

	weston_config_section_get_color(section,
                       "bkgnd-color",
                       &shell->bkgnd_color, 0xFF000000);

	weston_config_section_get_bool(section,
	                   "enable-cursor",
	                   &shell->enable_cursor, 0);

	wl_array_init(&shell->screen_ids);

	while (weston_config_next_section(config, &section, &name)) {
		char *screen_name = NULL;
		uint32_t screen_id = 0;

		if (0 != strcmp(name, "ivi-screen"))
			continue;

		if (0 != weston_config_section_get_string(section, "screen-name",
							  &screen_name, NULL))
			continue;

		if (0 != weston_config_section_get_uint(section,
							"screen-id",
							&screen_id, 0))
		{
			free(screen_name);
			continue;
		}

		screen_info = wl_array_add(&shell->screen_ids,
					   sizeof(*screen_info));
		if(screen_info)
		{
			screen_info->screen_name = screen_name;
			screen_info->screen_id = screen_id;
		}
	}
}

static void
ivi_shell_destroy(struct wl_listener *listener, void *data)
{
	struct ivisurface *ivisurf;
	struct ivisurface *ivisurf_next;
	struct ivilayer *ivilayer;
	struct ivilayer *ivilayer_next;
	struct iviscreen *iviscrn;
	struct iviscreen *iviscrn_next;
	struct ivishell *shell =
		wl_container_of(listener, shell, destroy_listener);

        if (shell->client) {
            wl_list_remove(&shell->client_destroy_listener.link);
            wl_client_destroy(shell->client);
        }

	wl_list_remove(&shell->destroy_listener.link);

	wl_list_remove(&shell->output_created.link);
	wl_list_remove(&shell->output_destroyed.link);
	wl_list_remove(&shell->output_resized.link);

	wl_list_remove(&shell->surface_configured.link);
	wl_list_remove(&shell->surface_removed.link);
	wl_list_remove(&shell->surface_created.link);

	wl_list_remove(&shell->layer_removed.link);
	wl_list_remove(&shell->layer_created.link);

	wl_list_for_each_safe(ivisurf, ivisurf_next,
			      &shell->list_surface, link) {
		wl_list_remove(&ivisurf->link);
		free(ivisurf);
	}

	wl_list_for_each_safe(ivilayer, ivilayer_next,
			      &shell->list_layer, link) {
		wl_list_remove(&ivilayer->link);
		free(ivilayer);
	}

	wl_list_for_each_safe(iviscrn, iviscrn_next,
			      &shell->list_screen, link) {
		destroy_screen(iviscrn);
	}

	destroy_screen_ids(shell);
	free(shell);
}

void
init_ivi_shell(struct weston_compositor *ec, struct ivishell *shell)
{
    const struct ivi_layout_interface *lyt = shell->interface;
    struct weston_output *output = NULL;
    int32_t ret = 0;

    shell->compositor = ec;

    wl_list_init(&shell->list_surface);
    wl_list_init(&shell->list_layer);
    wl_list_init(&shell->list_screen);
    wl_list_init(&shell->list_controller);

    wl_list_for_each(output, &ec->output_list, link)
        create_screen(shell, output);

    ret = check_layout_layers(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_layers");
    }

    ret = check_layout_surfaces(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_surfaces");
    }

    shell->layer_created.notify = layer_event_create;
    shell->layer_removed.notify = layer_event_remove;

    lyt->add_listener_create_layer(&shell->layer_created);
    lyt->add_listener_remove_layer(&shell->layer_removed);

    shell->surface_created.notify = surface_event_create;
    shell->surface_removed.notify = surface_event_remove;
    shell->surface_configured.notify = surface_event_configure;

    lyt->add_listener_create_surface(&shell->surface_created);
    lyt->add_listener_remove_surface(&shell->surface_removed);
    lyt->add_listener_configure_surface(&shell->surface_configured);

    shell->output_created.notify = output_created_event;
    shell->output_destroyed.notify = output_destroyed_event;
    shell->output_resized.notify = output_resized_event;

    wl_signal_add(&ec->output_created_signal, &shell->output_created);
    wl_signal_add(&ec->output_destroyed_signal, &shell->output_destroyed);
    wl_signal_add(&ec->output_resized_signal, &shell->output_resized);

    wl_signal_init(&shell->ivisurface_created_signal);
    wl_signal_init(&shell->ivisurface_removed_signal);
}

int
setup_ivi_controller_server(struct weston_compositor *compositor,
                            struct ivishell *shell)
{
    if (wl_global_create(compositor->wl_display, &ivi_wm_interface, 1,
                         shell, bind_ivi_controller) == NULL) {
        return -1;
    }

    return 0;
}

static int
load_input_module(struct ivishell *shell)
{
    struct weston_config *config = wet_get_config(shell->compositor);
    struct weston_config_section *section;
    char *input_module = NULL;

    int (*input_module_init)(struct ivishell *shell);

    section = weston_config_get_section(config, "ivi-shell", NULL, NULL);

    if (weston_config_section_get_string(section, "ivi-input-module",
                                         &input_module, NULL) < 0) {
        /* input events are handled by weston's default grabs */
        weston_log("ivi-controller: No ivi-input-module set\n");
        return 0;
    }

    input_module_init = wet_load_module_entrypoint(input_module, "input_controller_module_init");
    if (!input_module_init)
        return -1;

    if (input_module_init(shell) != 0) {
        weston_log("ivi-controller: Initialization of input module fails");
        return -1;
    }

    free(input_module);

    return 0;
}

static void
ivi_shell_client_destroy(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell,
                                             client_destroy_listener);

    weston_log("ivi shell client %p destroyed \n", shell->client);

    wl_list_remove(&shell->client_destroy_listener.link);
    shell->client = NULL;
}

static void
launch_client_process(void *data)
{
    struct ivishell *shell =
            (struct ivishell *)data;
    char option[128] = {0};

    sprintf(option, "%d", shell->bkgnd_surface_id);
    setenv(IVI_CLIENT_SURFACE_ID_ENV_NAME, option, 0x1);
    if (shell->debug_scopes) {
        setenv(IVI_CLIENT_DEBUG_SCOPES_ENV_NAME, shell->debug_scopes, 0x1);
        free(shell->debug_scopes);
    }
    if(shell->enable_cursor) {
      sprintf(option, "%d", shell->enable_cursor);
      setenv(IVI_CLIENT_ENABLE_CURSOR_ENV_NAME, option, 0x1);
    }

    shell->client = weston_client_start(shell->compositor,
                                        shell->ivi_client_name);

    shell->client_destroy_listener.notify = ivi_shell_client_destroy;
    wl_client_add_destroy_listener(shell->client,
                                   &shell->client_destroy_listener);

    free(shell->ivi_client_name);
}

static int load_id_agent_module(struct ivishell *shell)
{
    struct weston_config *config = wet_get_config(shell->compositor);
    struct weston_config_section *section;
    char *id_agent_module = NULL;

    int (*id_agent_module_init)(struct weston_compositor *compositor,
            const struct ivi_layout_interface *interface);

    section = weston_config_get_section(config, "ivi-shell", NULL, NULL);

    if (weston_config_section_get_string(section, "ivi-id-agent-module",
                                         &id_agent_module, NULL) < 0) {
        /* input events are handled by weston's default grabs */
        weston_log("ivi-controller: No ivi-id-agent-module set\n");
        return 0;
    }

    id_agent_module_init = wet_load_module_entrypoint(id_agent_module, "id_agent_module_init");
    if (!id_agent_module_init)
        return -1;

    if (id_agent_module_init(shell->compositor, shell->interface) != 0) {
        weston_log("ivi-controller: Initialization of id-agent module failed\n");
        return -1;
    }

    free(id_agent_module);

    return 0;
}

WL_EXPORT int
wet_module_init(struct weston_compositor *compositor,
		       int *argc, char *argv[])
{
    struct ivishell *shell;
    struct wl_event_loop *loop = NULL;
    (void)argc;
    (void)argv;

    shell = malloc(sizeof *shell);
    if (shell == NULL)
        return -1;

    memset(shell, 0, sizeof *shell);

    shell->interface = ivi_layout_get_api(compositor);
    if (!shell->interface) {
        free(shell);
        weston_log("Cannot use ivi_layout_interface.\n");
        return -1;
    }

    get_config(compositor, shell);

    /* Add background layer*/
    if (shell->bkgnd_surface_id && shell->ivi_client_name) {
        weston_layer_init(&shell->bkgnd_layer, compositor);
        weston_layer_set_position(&shell->bkgnd_layer,
                                  WESTON_LAYER_POSITION_BACKGROUND);
    }

    init_ivi_shell(compositor, shell);

    if (setup_ivi_controller_server(compositor, shell)) {
        destroy_screen_ids(shell);
        free(shell);
        return -1;
    }

    if (load_input_module(shell) < 0) {
        destroy_screen_ids(shell);
        free(shell);
        return -1;
    }
    /* add compositor destroy signal after loading input
     * modules, to ensure input module is the first one to
     * de-initialize
     */
    shell->destroy_listener.notify = ivi_shell_destroy;
    wl_signal_add(&compositor->destroy_signal, &shell->destroy_listener);

    if (shell->bkgnd_surface_id && shell->ivi_client_name) {
        loop = wl_display_get_event_loop(compositor->wl_display);
        wl_event_loop_add_idle(loop, launch_client_process, shell);
    }

    if (load_id_agent_module(shell) < 0) {
        weston_log("ivi-controller: id-agent module not loaded\n");
    }

    return 0;
}
