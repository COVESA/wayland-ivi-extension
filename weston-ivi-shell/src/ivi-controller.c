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

#include <stdlib.h>
#include <string.h>

#include <weston.h>
#include <weston/ivi-layout-export.h>
#include "ivi-wm-server-protocol.h"
#include "bitmap.h"

#include "wayland-util.h"
#ifdef IVI_SHARE_ENABLE
#  include "ivi-share.h"
#endif

struct ivilayer;
struct iviscreen;

struct notification {
    struct wl_list link;
    struct wl_resource *resource;
    struct wl_list layout_link;
};

struct ivisurface {
    struct wl_list link;
    struct ivishell *shell;
    uint32_t update_count;
    struct ivi_layout_surface *layout_surface;
    const struct ivi_layout_surface_properties *prop;
    struct wl_listener property_changed;
    struct wl_listener surface_destroy_listener;
    struct wl_list notification_list;
    enum ivi_wm_surface_type type;
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

struct ivishell {
    struct weston_compositor *compositor;
    const struct ivi_layout_interface *interface;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;

    struct wl_list list_controller;

    struct wl_listener surface_created;
    struct wl_listener surface_removed;
    struct wl_listener surface_configured;

    struct wl_listener layer_created;
    struct wl_listener layer_removed;

    struct wl_listener output_created;
    struct wl_listener output_destroyed;

    struct wl_listener destroy_listener;

    struct wl_array screen_ids;
    uint32_t screen_id_offset;
};

struct screenshot_frame_listener {
        struct wl_listener listener;
	char *filename;
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
controller_surface_set_opacity(struct wl_client *client,
                   struct wl_resource *resource,
                   wl_fixed_t opacity)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    (void)client;
    lyt->surface_set_opacity(ivisurf->layout_surface, opacity);
}

static void
controller_surface_set_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    (void)client;
    lyt->surface_set_source_rectangle(ivisurf->layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_surface_set_destination_rectangle(struct wl_client *client,
                     struct wl_resource *resource,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    (void)client;

    // TODO: create set transition type protocol
    lyt->surface_set_transition(ivisurf->layout_surface,
                                     IVI_LAYOUT_TRANSITION_NONE,
                                     300); // ms

    lyt->surface_set_destination_rectangle(ivisurf->layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_surface_set_visibility(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t visibility)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    (void)client;
    lyt->surface_set_visibility(ivisurf->layout_surface, visibility);
}

static void
controller_surface_set_configuration(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t width, int32_t height)
{
    /* This interface has been supported yet. */
    (void)client;
    (void)resource;
    (void)width;
    (void)height;
}

static void
controller_surface_set_orientation(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t orientation)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    (void)client;
    lyt->surface_set_orientation(ivisurf->layout_surface, (uint32_t)orientation);
}

static void
controller_surface_screenshot(struct wl_client *client,
                  struct wl_resource *resource,
                  const char *filename)
{
    int32_t result = IVI_FAILED;
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct weston_surface *weston_surface = NULL;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    int32_t size = 0;
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    char *buffer = NULL;
    int32_t image_stride = 0;
    int32_t image_size = 0;
    char *image_buffer = NULL;
    int32_t row = 0;
    int32_t col = 0;
    int32_t offset = 0;
    int32_t image_offset = 0;
    int32_t i = 0;
    int32_t padding = 0;
    int32_t sum_padding = 0;

    result = lyt->surface_get_size(ivisurf->layout_surface, &width,
                                   &height, &stride);
    if (result != IVI_SUCCEEDED) {
        weston_log("failed to get surface size\n");
        return;
    }

    size = stride * height;
    image_stride = (((width * 3) + 31) & ~31);
    image_size = image_stride * height;

    buffer = malloc(size);
    image_buffer = malloc(image_size);
    if (buffer == NULL || image_buffer == NULL) {
        free(image_buffer);
        free(buffer);
        weston_log("failed to allocate memory\n");
        return;
    }

    weston_surface = lyt->surface_get_weston_surface(ivisurf->layout_surface);

    result = lyt->surface_dump(weston_surface, buffer, size, 0, 0,
                               width, height);

    if (result != IVI_SUCCEEDED) {
        free(image_buffer);
        free(buffer);
        weston_log("failed to dump surface\n");
        return;
    }

    /* When width is not multiple of 4, calculate padding. */
    if (width % 4 != 0)
        padding = (4 - ((width * 3) % 4));

    for (row = 0; row < height; ++row) {
        for (col = 0; col < width; ++col) {
            offset = (height - row - 1) * width + col;
            image_offset = (row * width + col) * 3 + sum_padding;

            image_buffer[image_offset] = buffer[offset * 4 + 2];
            image_buffer[image_offset + 1] = buffer[offset * 4 + 1];
            image_buffer[image_offset + 2] = buffer[offset * 4];
        }
        for (i = 1; i <= padding; ++i) {
            image_buffer[image_offset + 2 + i] = 0;
            sum_padding++;
         }

    }

    free(buffer);

    if (save_as_bitmap(filename, (const char *)image_buffer,
                       image_size, width, height, 24) != 0) {
        weston_log("failed to take screenshot\n");
    }

    free(image_buffer);
}


static void
controller_surface_send_stats(struct wl_client *client,
                              struct wl_resource *resource)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivisurf->shell->interface;
    struct weston_surface *surface;
    struct wl_client* target_client;
    pid_t pid;
    uid_t uid;
    gid_t gid;

    /* Get pid that creates surface */
    surface = lyt->surface_get_weston_surface(ivisurf->layout_surface);
    target_client = wl_resource_get_client(surface->resource);

    wl_client_get_credentials(target_client, &pid, &uid, &gid);

    ivi_controller_surface_send_stats(resource, 0, 0,
                                      ivisurf->update_count, pid, "");
}

static void
controller_surface_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    (void)client;
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);

    wl_resource_destroy(resource);

    if (wl_list_empty(&ivisurf->resource_list) && destroy_scene_object) {
        wl_list_remove(&ivisurf->link);
        free(ivisurf);
    }
}

static const
struct ivi_controller_surface_interface controller_surface_implementation = {
    controller_surface_set_visibility,
    controller_surface_set_opacity,
    controller_surface_set_source_rectangle,
    controller_surface_set_destination_rectangle,
    controller_surface_set_configuration,
    controller_surface_set_orientation,
    controller_surface_screenshot,
    controller_surface_send_stats,
    controller_surface_destroy
};

static void
controller_layer_set_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_source_rectangle(ivilayer->layout_layer,
           (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_layer_set_destination_rectangle(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t x,
                 int32_t y,
                 int32_t width,
                 int32_t height)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_destination_rectangle(ivilayer->layout_layer,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_layer_set_visibility(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t visibility)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_visibility(ivilayer->layout_layer, visibility);
}

static void
controller_layer_set_opacity(struct wl_client *client,
                 struct wl_resource *resource,
                 wl_fixed_t opacity)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_opacity(ivilayer->layout_layer, opacity);
}

static void
controller_layer_set_configuration(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t width,
                 int32_t height)
{
    /* This interface has been supported yet. */
    (void)client;
    (void)resource;
    (void)width;
    (void)height;
}

static void
controller_layer_set_orientation(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t orientation)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_orientation(ivilayer->layout_layer, (uint32_t)orientation);
}

static void
controller_layer_clear_surfaces(struct wl_client *client,
                    struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_set_render_order(ivilayer->layout_layer, NULL, 0);
}

static void
controller_layer_add_surface(struct wl_client *client,
                 struct wl_resource *resource,
                 struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_add_surface(ivilayer->layout_layer, ivisurf->layout_surface);
}

static void
controller_layer_remove_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;
    lyt->layer_remove_surface(ivilayer->layout_layer, ivisurf->layout_surface);
}

static void
controller_layer_screenshot(struct wl_client *client,
                struct wl_resource *resource,
                const char *filename)
{
    (void)client;
    (void)resource;
    (void)filename;
}

static void
controller_layer_set_render_order(struct wl_client *client,
                                  struct wl_resource *resource,
                                  struct wl_array *id_surfaces)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    struct ivi_layout_surface **layoutsurf_array = NULL;
    uint32_t *id_surface = NULL;
    int i = 0;
    (void)client;

    layoutsurf_array = (struct ivi_layout_surface**)calloc(
                           id_surfaces->size, sizeof(void*));

    wl_array_for_each(id_surface, id_surfaces) {
        layoutsurf_array[i] = lyt->get_surface_from_id(*id_surface);

        if (layoutsurf_array[i])
            i++;
    }

    lyt->layer_set_render_order(ivilayer->layout_layer,
                                   layoutsurf_array, i);
    free(layoutsurf_array);
}

static void
controller_layer_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = ivilayer->shell->interface;
    (void)client;

    if (destroy_scene_object) {
        if (ivilayer->layout_layer != NULL) {
            lyt->layer_destroy(ivilayer->layout_layer);
            ivilayer->layout_layer = NULL;
        }

        wl_resource_destroy(resource);

        if (wl_list_empty(&ivilayer->resource_list)) {
            wl_list_remove(&ivilayer->link);
            free(ivilayer);
        }
    } else {
        wl_resource_destroy(resource);
    }
}

static const
struct ivi_controller_layer_interface controller_layer_implementation = {
    controller_layer_set_visibility,
    controller_layer_set_opacity,
    controller_layer_set_source_rectangle,
    controller_layer_set_destination_rectangle,
    controller_layer_set_configuration,
    controller_layer_set_orientation,
    controller_layer_screenshot,
    controller_layer_clear_surfaces,
    controller_layer_add_surface,
    controller_layer_remove_surface,
    controller_layer_set_render_order,
    controller_layer_destroy
};

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
    const struct ivi_layout_interface *lyt = iviscrn->shell->interface;
    (void)client;
    lyt->screen_set_render_order(iviscrn->output, NULL, 0);
}

static void
controller_screen_add_layer(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_resource *layer)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = iviscrn->shell->interface;
    struct ivilayer *ivilayer = wl_resource_get_user_data(layer);
    (void)client;
    lyt->screen_add_layer(iviscrn->output, ivilayer->layout_layer);
}

static void
controller_screenshot_notify(struct wl_listener *listener, void *data)
{
    struct screenshot_frame_listener *l =
        wl_container_of(listener, l, listener);
    char *filename = l->filename;

    struct weston_output *output = data;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    uint8_t *readpixs = NULL;

    --output->disable_planes;
    wl_list_remove(&listener->link);

    width = output->current_mode->width;
    height = output->current_mode->height;
    stride = width * (PIXMAN_FORMAT_BPP(output->compositor->read_format) / 8);

    readpixs = malloc(stride * height);
    if (readpixs == NULL) {
        weston_log("fails to allocate memory\n");
        free(l->filename);
        free(l);
        return;
    }

    output->compositor->renderer->read_pixels(
            output,
            output->compositor->read_format,
            readpixs,
            0,
            0,
            width,
            height);

    save_as_bitmap(filename, (const char*)readpixs, stride * height, width, height,
                   PIXMAN_FORMAT_BPP(output->compositor->read_format));
    free(readpixs);
    free(l->filename);
    free(l);
}

static void
controller_screen_screenshot(struct wl_client *client,
                struct wl_resource *resource,
                const char *filename)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct screenshot_frame_listener *l;
    (void)client;

    l = malloc(sizeof *l);
    if(l == NULL) {
        fprintf(stderr, "fails to allocate memory\n");
        return;
    }

    l->filename = strdup(filename);
    if(l->filename == NULL) {
        fprintf(stderr, "fails to allocate memory\n");
        free(l);
        return;
    }

    l->listener.notify = controller_screenshot_notify;
    wl_signal_add(&iviscrn->output->frame_signal, &l->listener);
    iviscrn->output->disable_planes++;
    weston_output_schedule_repaint(iviscrn->output);
    return;
}

static void
controller_screen_set_render_order(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_array *id_layers)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    const struct ivi_layout_interface *lyt = iviscrn->shell->interface;
    struct ivi_layout_layer **layoutlayer_array = NULL;
    struct ivilayer *ivilayer = NULL;
    uint32_t *id_layer = NULL;
    uint32_t id_layout_layer = 0;
    int i = 0;
    (void)client;

    layoutlayer_array = (struct ivi_layout_layer**)calloc(
                           id_layers->size, sizeof(void*));

    wl_array_for_each(id_layer, id_layers) {
        layoutlayer_array[i] = lyt->get_layer_from_id(*id_layer);

        if (layoutlayer_array[i])
            i++;
    }

    lyt->screen_set_render_order(iviscrn->output,
                                    layoutlayer_array, i);
    free(layoutlayer_array);
}

static const
struct ivi_controller_screen_interface controller_screen_implementation = {
    controller_screen_destroy,
    controller_screen_clear,
    controller_screen_add_layer,
    controller_screen_screenshot,
    controller_screen_set_render_order
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
controller_layer_create(struct wl_client *client,
                        struct wl_resource *resource,
                        uint32_t id_layer,
                        int32_t width,
                        int32_t height,
                        uint32_t id)
{
    struct wl_resource *layer_resource;
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct ivishell *shell = ctrl->shell;
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivi_layout_layer *layout_layer = NULL;
    struct ivilayer *ivilayer = NULL;
    const struct ivi_layout_layer_properties *prop;

    layout_layer = lyt->get_layer_from_id(id_layer);
    if (layout_layer == NULL) {
        layout_layer = lyt->layer_create_with_dimension(id_layer,
                           (uint32_t)width, (uint32_t)height);
        if (layout_layer == NULL) {
            weston_log("id_layer is already created\n");
            return;
        }
    }

    /* ivilayer will be created by layer_event_create */
    ivilayer = get_layer(&shell->list_layer, layout_layer);
    if (ivilayer == NULL) {
        weston_log("couldn't get layer object\n");
        return;
    }

    layer_resource = wl_resource_create(client,
                               &ivi_controller_layer_interface, 1, id);
    if (layer_resource == NULL) {
        weston_log("couldn't get layer object\n");
        return;
    }

    wl_list_insert(&ivilayer->resource_list, wl_resource_get_link(layer_resource));
    wl_resource_set_implementation(layer_resource,
                                   &controller_layer_implementation,
                                   ivilayer, destroy_ivicontroller_layer);

    prop = lyt->get_properties_of_layer(ivilayer->layout_layer);
    send_layer_event(layer_resource, ivilayer,
                     prop, IVI_NOTIFICATION_ALL);
}

static void
controller_surface_create(struct wl_client *client,
                          struct wl_resource *resource,
                          uint32_t id_surface,
                          uint32_t id)
{
    struct wl_resource *surf_resource;
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct ivishell *shell = ctrl->shell;
    const struct ivi_layout_interface *lyt = shell->interface;
    const struct ivi_layout_surface_properties *prop;
    struct ivi_layout_surface *layout_surface = NULL;
    struct ivisurface *ivisurf = NULL;

    layout_surface = lyt->get_surface_from_id(id_surface);
    if (layout_surface == NULL) {
        return;
    }

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        return;
    }

    surf_resource = wl_resource_create(client,
                               &ivi_controller_surface_interface, 1, id);
    if (surf_resource == NULL) {
        weston_log("couldn't surface object");
        return;
    }

    wl_list_insert(&ivisurf->resource_list, wl_resource_get_link(surf_resource));
    wl_resource_set_implementation(surf_resource,
                                   &controller_surface_implementation,
                                   ivisurf, destroy_ivicontroller_surface);

    prop = lyt->get_properties_of_surface(ivisurf->layout_surface);

    send_surface_event(surf_resource, ivisurf, prop, IVI_NOTIFICATION_ALL);
}

static const struct ivi_controller_interface controller_implementation = {
    controller_commit_changes,
    controller_layer_create,
    controller_surface_create
};

static void
add_client_to_resources(struct ivishell *shell,
                        struct wl_client *client,
                        struct ivicontroller *controller)
{
    const struct ivi_layout_interface *lyt = shell->interface;
    struct wl_resource *screen_resource;
    struct ivisurface* ivisurf = NULL;
    struct ivilayer* ivilayer = NULL;
    struct iviscreen* iviscrn = NULL;
    struct wl_resource *resource_output = NULL;
    uint32_t id_layout_surface = 0;
    uint32_t id_layout_layer = 0;

    wl_list_for_each(iviscrn, &shell->list_screen, link) {
        resource_output = wl_resource_find_for_client(
                &iviscrn->output->resource_list, client);
        if (resource_output == NULL) {
            continue;
        }

        screen_resource = wl_resource_create(client, &ivi_controller_screen_interface, 1, 0);
        if (screen_resource == NULL) {
            weston_log("couldn't new screen controller object");
            return;
        }

        wl_resource_set_implementation(screen_resource,
                                       &controller_screen_implementation,
                                       iviscrn, destroy_ivicontroller_screen);

        wl_list_insert(&iviscrn->resource_list, wl_resource_get_link(screen_resource));

        ivi_controller_send_screen(controller->resource,
                                   wl_resource_get_id(resource_output),
                                   screen_resource);
    }
    wl_list_for_each_reverse(ivilayer, &shell->list_layer, link) {
        id_layout_layer = lyt->get_id_of_layer(ivilayer->layout_layer);

        ivi_controller_send_layer(controller->resource,
                                  id_layout_layer);
    }
    wl_list_for_each_reverse(ivisurf, &shell->list_surface, link) {
        id_layout_surface = lyt->get_id_of_surface(ivisurf->layout_surface);

        ivi_controller_send_surface(controller->resource,
                                    id_layout_surface);
    }
}

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

static struct iviscreen*
create_screen(struct ivishell *shell, struct weston_output *output)
{
    struct iviscreen *iviscrn;
    struct screen_id_info *screen_info;
    uint32_t id;

    iviscrn = calloc(1, sizeof *iviscrn);
    if (iviscrn == NULL) {
        weston_log("no memory to allocate client screen\n");
        return NULL;
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

    return iviscrn;
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
}

static void
output_created_event(struct wl_listener *listener, void *data)
{
    struct ivishell *shell = wl_container_of(listener, shell, output_created);
    struct iviscreen *iviscrn = NULL;
    struct weston_output *created_output = (struct weston_output*)data;

    iviscrn = create_screen(shell, created_output);
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

static struct ivisurface*
create_surface(struct ivishell *shell,
               struct ivi_layout_surface *layout_surface,
               uint32_t id_surface)
{
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller *controller = NULL;

    ivisurf = calloc(1, sizeof *ivisurf);
    if (ivisurf == NULL) {
        weston_log("no memory to allocate client surface\n");
        return NULL;
    }

    ivisurf->shell = shell;
    ivisurf->layout_surface = layout_surface;
    ivisurf->prop = lyt->get_properties_of_surface(layout_surface);
    wl_list_insert(&shell->list_surface, &ivisurf->link);
    wl_list_init(&ivisurf->notification_list);

    wl_list_for_each(controller, &shell->list_controller, link) {
        if (controller->resource)
            ivi_wm_send_surface_created(controller->resource, id_surface);
    }

     ivisurf->property_changed.notify = send_surface_prop;
     lyt->surface_add_listener(layout_surface, &ivisurf->property_changed);

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
    struct wl_resource *resource;
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
    struct wl_resource *resource;
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
}

static void
surface_event_remove(struct wl_listener *listener, void *data)
{
    struct wl_resource *resource;
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

    wl_list_for_each_safe(not, next, &ivisurf->notification_list, layout_link)
    {
        wl_list_remove(&not->link);
        wl_list_remove(&not->layout_link);
        free(not);
    }

    wl_list_remove(&ivisurf->link);
    wl_list_remove(&ivisurf->property_changed.link);
    free(ivisurf);

    id_surface = shell->interface->get_id_of_surface(layout_surface);

    wl_list_for_each(controller, &shell->list_controller, link) {
        if (controller->resource)
            ivi_wm_send_surface_destroyed(controller->resource, id_surface);
    }
}

static void
surface_event_configure(struct wl_listener *listener, void *data)
{
    struct wl_resource *resource;
    struct ivishell *shell = wl_container_of(listener, shell, surface_configured);
    const struct ivi_layout_interface *lyt = shell->interface;
    struct ivisurface *ivisurf = NULL;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    struct ivicontroller *ctrl;
    struct notification *not;
    uint32_t surface_id;
    struct weston_surface *w_surface;

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    surface_id = lyt->get_id_of_surface(layout_surface);

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
    uint32_t i = 0;
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
    uint32_t i = 0;
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
get_screen_ids(struct weston_compositor *compositor, struct ivishell *shell)
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

	wl_list_remove(&shell->output_destroyed.link);

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
    struct iviscreen *iviscrn = NULL;
    int32_t ret = 0;

    shell->compositor = ec;

    wl_list_init(&shell->list_surface);
    wl_list_init(&shell->list_layer);
    wl_list_init(&shell->list_screen);
    wl_list_init(&shell->list_controller);

    wl_list_for_each(output, &ec->output_list, link)
        iviscrn = create_screen(shell, output);

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

    wl_signal_add(&ec->output_created_signal, &shell->output_created);
    wl_signal_add(&ec->output_destroyed_signal, &shell->output_destroyed);

    shell->destroy_listener.notify = ivi_shell_destroy;
    wl_signal_add(&ec->destroy_signal, &shell->destroy_listener);
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
load_input_module(struct weston_compositor *ec,
                  const struct ivi_layout_interface *interface,
                  size_t interface_version)
{
    struct weston_config *config = wet_get_config(ec);
    struct weston_config_section *section;
    char *input_module = NULL;

    int (*input_module_init)(struct weston_compositor *ec,
                             const struct ivi_layout_interface *interface,
                             size_t interface_version);

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

    if (input_module_init(ec, interface,
                          sizeof(struct ivi_layout_interface)) != 0) {
        weston_log("ivi-controller: Initialization of input module failes");
        return -1;
    }

    free(input_module);

    return 0;
}

WL_EXPORT int
controller_module_init(struct weston_compositor *compositor,
		       int *argc, char *argv[],
		       const struct ivi_layout_interface *interface,
		       size_t interface_version)
{
    struct ivishell *shell;
    (void)argc;
    (void)argv;

    shell = malloc(sizeof *shell);
    if (shell == NULL)
        return -1;

    memset(shell, 0, sizeof *shell);

    shell->interface = interface;

    get_screen_ids(compositor, shell);

    init_ivi_shell(compositor, shell);

#ifdef IVI_SHARE_ENABLE
    if (setup_buffer_sharing(compositor, interface) < 0) {
        free(shell);
        return -1;
    }
#endif

    if (setup_ivi_controller_server(compositor, shell)) {
        destroy_screen_ids(shell);
        free(shell);
        return -1;
    }

    if (load_input_module(compositor, interface, interface_version) < 0) {
        destroy_screen_ids(shell);
        free(shell);
        return -1;
    }

    return 0;
}
