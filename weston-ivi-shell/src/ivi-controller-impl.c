/*
 * Copyright (C) 2013 DENSO CORPORATION
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

#include <weston/compositor.h>
#include "ivi-controller-server-protocol.h"
#include "bitmap.h"

#include "ivi-layout-export.h"
#include "ivi-extension.h"
#include "ivi-controller-impl.h"
#include "wayland-util.h"

struct ivilayer;
struct iviscreen;

struct ivisurface {
    struct wl_list link;
    struct ivishell *shell;
    uint32_t update_count;
    struct ivi_layout_surface *layout_surface;
    struct wl_listener surface_destroy_listener;
    struct ivilayer *on_layer;
    struct wl_list resource_list;
};

struct ivilayer {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_layer *layout_layer;
    struct iviscreen *on_screen;
    struct wl_list resource_list;
};

struct iviscreen {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_screen *layout_screen;
    struct weston_output *output;
    struct wl_list resource_list;
};

struct ivicontroller {
    struct wl_resource *resource;
    uint32_t id;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
};

struct screenshot_frame_listener {
        struct wl_listener listener;
	char *filename;
};

static void surface_event_remove(struct ivi_layout_surface *, void *);

static void
destroy_ivicontroller_surface(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
destroy_ivicontroller_layer(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
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
send_surface_add_event(struct ivisurface *ivisurf,
                       struct wl_resource *resource,
                       enum ivi_layout_notification_mask mask)
{
    struct wl_resource *layer_resource;
    struct ivi_layout_layer **pArray = NULL;
    int32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct ivilayer *ivilayer = NULL;
    struct ivishell *shell = ivisurf->shell;
    struct wl_client *client = wl_resource_get_client(resource);

    ans = ivi_extension_get_layers_under_surface(shell, ivisurf->layout_surface,
                                                  &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get layers at send_surface_add_event\n");
        return;
    }

    /* Send Null to cancel added surface */
    if (mask & IVI_NOTIFICATION_REMOVE) {
        ivi_controller_surface_send_layer(resource, NULL);
    }
    else if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; i++) {
            /* Send new surface event */
            ivilayer = NULL;
            wl_list_for_each(ivilayer, &shell->list_layer, link) {
                if (ivilayer->layout_layer == pArray[i]) {
                    break;
                }
            }

            if (ivilayer == NULL) {
                continue;
            }

            layer_resource = wl_resource_find_for_client(&ivilayer->resource_list,
                                                         client);

            if (layer_resource != NULL) {
                ivi_controller_surface_send_layer(resource, layer_resource);
            }
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_surface_configure_event(struct ivisurface *ivisurf,
                       struct wl_resource *resource)
{
    struct weston_surface *surface;
    struct ivi_layout_surface* layout_surface;
    struct ivishell* shell;

    layout_surface = ivisurf->layout_surface;
    shell = ivisurf->shell;

    surface = ivi_extension_surface_get_weston_surface(shell, layout_surface);

    if (!surface)
        return;

    if ((surface->width == 0) || (surface->height == 0))
        return;

    ivi_controller_surface_send_configuration(resource,
                                              surface->width,
                                              surface->height);
}

static void
send_surface_event(struct wl_resource *resource,
                   struct ivisurface *ivisurf,
                   const struct ivi_layout_surface_properties *prop,
                   uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_controller_surface_send_opacity(resource,
                                            prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_controller_surface_send_source_rectangle(resource,
            prop->source_x, prop->source_y,
            prop->source_width, prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_controller_surface_send_destination_rectangle(resource,
            prop->dest_x, prop->dest_y,
            prop->dest_width, prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_ORIENTATION) {
        ivi_controller_surface_send_orientation(resource,
                                                prop->orientation);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_controller_surface_send_visibility(resource,
                                               prop->visibility);
    }
    if (mask & IVI_NOTIFICATION_REMOVE) {
        send_surface_add_event(ivisurf, resource, IVI_NOTIFICATION_REMOVE);
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        send_surface_add_event(ivisurf, resource, IVI_NOTIFICATION_ADD);
    }
    if (mask & IVI_NOTIFICATION_CONFIGURE) {
        send_surface_configure_event(ivisurf, resource);
    }
}

static void
update_surface_prop(struct ivisurface *ivisurf,
                    uint32_t mask)
{
    struct ivi_layout_layer **pArray = NULL;
    int32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct ivishell *shell = ivisurf->shell;

    ans = ivi_extension_get_layers_under_surface(shell, ivisurf->layout_surface,
                                                  &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get layers at send_surface_add_event\n");
        return;
    }

    if (mask & IVI_NOTIFICATION_REMOVE) {
        ivisurf->on_layer = NULL;
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; ++i) {
            /* Create list_layer */
            struct ivilayer *ivilayer = NULL;

            wl_list_for_each(ivilayer, &shell->list_layer, link) {
                if (ivilayer->layout_layer == pArray[i]) {
                    break;
                }
            }

            ivisurf->on_layer = ivilayer;
        }
    }
}

static void
send_surface_prop(struct ivi_layout_surface *layout_surface,
                  const struct ivi_layout_surface_properties *prop,
                  enum ivi_layout_notification_mask mask,
                  void *userdata)
{
    struct ivisurface *ivisurf = userdata;
    struct wl_resource *resource;

    wl_resource_for_each(resource, &ivisurf->resource_list) {
        send_surface_event(resource, ivisurf, prop, mask);
    }

    update_surface_prop(ivisurf, mask);
}

static void
send_layer_add_event(struct ivilayer *ivilayer,
                     struct wl_resource *resource,
                     enum ivi_layout_notification_mask mask)
{
    struct ivi_layout_screen **pArray = NULL;
    int32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct iviscreen *iviscrn = NULL;
    struct ivishell *shell = ivilayer->shell;
    struct wl_client *client = wl_resource_get_client(resource);
    struct wl_resource *resource_output = NULL;

    ans = ivi_extension_get_screens_under_layer(shell, ivilayer->layout_layer,
                                                 &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get screens at send_layer_add_event\n");
        return;
    }

    /* Send Null to cancel added layer */
    if (mask & IVI_NOTIFICATION_REMOVE) {
            ivi_controller_layer_send_screen(resource, NULL);
    }
    else if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; i++) {
            /* Send new layer event */
            iviscrn = NULL;
            wl_list_for_each(iviscrn, &shell->list_screen, link) {
                if (iviscrn->layout_screen == pArray[i]) {
                    break;
                }
            }

            if (iviscrn == NULL) {
                continue;
            }

            resource_output =
                wl_resource_find_for_client(&iviscrn->output->resource_list,
                                     client);
            if (resource_output != NULL) {
                ivi_controller_layer_send_screen(resource, resource_output);
            }
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_layer_event(struct wl_resource *resource,
                 struct ivilayer *ivilayer,
                 const struct ivi_layout_layer_properties *prop,
                 uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_controller_layer_send_opacity(resource,
                                          prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_controller_layer_send_source_rectangle(resource,
                                          prop->source_x,
                                          prop->source_y,
                                          prop->source_width,
                                          prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_controller_layer_send_destination_rectangle(resource,
                                          prop->dest_x,
                                          prop->dest_y,
                                          prop->dest_width,
                                          prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_ORIENTATION) {
        ivi_controller_layer_send_orientation(resource,
                                          prop->orientation);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_controller_layer_send_visibility(resource,
                                          prop->visibility);
    }
    if (mask & IVI_NOTIFICATION_REMOVE) {
        send_layer_add_event(ivilayer, resource, IVI_NOTIFICATION_REMOVE);
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        send_layer_add_event(ivilayer, resource, IVI_NOTIFICATION_ADD);
    }
}

static void
update_layer_prop(struct ivilayer *ivilayer,
                  enum ivi_layout_notification_mask mask)
{
    struct ivi_layout_screen **pArray = NULL;
    int32_t length = 0;
    int32_t ans = 0;
    struct ivishell *shell = ivilayer->shell;

    ans = ivi_extension_get_screens_under_layer(shell, ivilayer->layout_layer,
                                                 &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get screens at send_layer_add_event\n");
        return;
    }

    /* Send Null to cancel added layer */
    if (mask & IVI_NOTIFICATION_REMOVE) {
        ivilayer->on_screen = NULL;
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        int i = 0;
        for (i = 0; i < (int)length; i++) {
            struct ivishell *shell = ivilayer->shell;
            struct iviscreen *iviscrn = NULL;

            wl_list_for_each(iviscrn, &shell->list_screen, link) {
                if (iviscrn->layout_screen == pArray[i]) {
                    ivilayer->on_screen = iviscrn;
                    break;
                }
            }
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_layer_prop(struct ivi_layout_layer *layer,
                const struct ivi_layout_layer_properties *prop,
                enum ivi_layout_notification_mask mask,
                void *userdata)
{
    struct ivilayer *ivilayer = userdata;
    struct wl_resource *resource;

    wl_resource_for_each(resource, &ivilayer->resource_list) {
        send_layer_event(resource, ivilayer, prop, mask);
    }

    update_layer_prop(ivilayer, mask);
}

static void
controller_surface_set_opacity(struct wl_client *client,
                   struct wl_resource *resource,
                   wl_fixed_t opacity)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_opacity(ivisurf->shell, ivisurf->layout_surface, opacity);
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
    (void)client;
    ivi_extension_surface_set_source_rectangle(ivisurf->shell, ivisurf->layout_surface,
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
    (void)client;

    // TODO: create set transition type protocol
    ivi_extension_surface_set_transition(ivisurf->shell, ivisurf->layout_surface,
                                     IVI_LAYOUT_TRANSITION_NONE,
                                     300); // ms

    ivi_extension_surface_set_destination_rectangle(ivisurf->shell, ivisurf->layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_surface_set_visibility(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t visibility)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_visibility(ivisurf->shell, ivisurf->layout_surface, visibility);
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
    (void)client;
    ivi_extension_surface_set_orientation(ivisurf->shell, ivisurf->layout_surface, (uint32_t)orientation);
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
    struct ivishell *shell = ivisurf->shell;
    char *buffer = NULL;
    int32_t image_stride = 0;
    int32_t image_size = 0;
    char *image_buffer = NULL;
    int32_t row = 0;
    int32_t col = 0;
    int32_t offset = 0;
    int32_t image_offset = 0;

    result = ivi_extension_surface_get_size(
        shell, ivisurf->layout_surface, &width, &height, &stride);
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

    weston_surface = ivi_extension_surface_get_weston_surface(
        shell, ivisurf->layout_surface);

    result = ivi_extension_surface_dump(shell, weston_surface,
        buffer, size, 0, 0, width, height);

    if (result != IVI_SUCCEEDED) {
        free(image_buffer);
        free(buffer);
        weston_log("failed to dump surface\n");
        return;
    }

    for (row = 0; row < height; ++row) {
        for (col = 0; col < width; ++col) {
            offset = row * width + col;
            image_offset = (height - row - 1) * width + col;

            image_buffer[image_offset * 3] = buffer[offset * 4 + 2];
            image_buffer[image_offset * 3 + 1] = buffer[offset * 4 + 1];
            image_buffer[image_offset * 3 + 2] = buffer[offset * 4];
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
    pid_t pid;
    uid_t uid;
    gid_t gid;
    wl_client_get_credentials(client, &pid, &uid, &gid);

    ivi_controller_surface_send_stats(resource, 0, 0,
                                      ivisurf->update_count, pid, "");
}

static void
controller_surface_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    (void)client;
    (void)destroy_scene_object;
    wl_resource_destroy(resource);
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
    (void)client;
    ivi_extension_layer_set_source_rectangle(ivilayer->shell, ivilayer->layout_layer,
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
    (void)client;
    ivi_extension_layer_set_destination_rectangle(ivilayer->shell, ivilayer->layout_layer,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_layer_set_visibility(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t visibility)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_visibility(ivilayer->shell, ivilayer->layout_layer, visibility);
}

static void
controller_layer_set_opacity(struct wl_client *client,
                 struct wl_resource *resource,
                 wl_fixed_t opacity)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_opacity(ivilayer->shell, ivilayer->layout_layer, opacity);
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
    (void)client;
    ivi_extension_layer_set_orientation(ivilayer->shell, ivilayer->layout_layer, (uint32_t)orientation);
}

static void
controller_layer_clear_surfaces(struct wl_client *client,
                    struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_render_order(ivilayer->shell, ivilayer->layout_layer, NULL, 0);
}

static void
controller_layer_add_surface(struct wl_client *client,
                 struct wl_resource *resource,
                 struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    (void)client;
    ivi_extension_layer_add_surface(ivilayer->shell, ivilayer->layout_layer, ivisurf->layout_surface);
}

static void
controller_layer_remove_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    (void)client;
    ivi_extension_layer_remove_surface(ivilayer->shell, ivilayer->layout_layer, ivisurf->layout_surface);
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
    struct ivi_layout_surface **layoutsurf_array = NULL;
    struct ivisurface *ivisurf = NULL;
    uint32_t *id_surface = NULL;
    uint32_t id_layout_surface = 0;
    int i = 0;
    (void)client;
    struct ivishell *shell = ivilayer->shell;

    layoutsurf_array = (struct ivi_layout_surface**)calloc(
                           id_surfaces->size, sizeof(void*));

    wl_array_for_each(id_surface, id_surfaces) {
        wl_list_for_each(ivisurf, &ivilayer->shell->list_surface, link) {
            id_layout_surface = ivi_extension_get_id_of_surface(shell, ivisurf->layout_surface);
            if (*id_surface == id_layout_surface) {
                layoutsurf_array[i] = ivisurf->layout_surface;
                i++;
                break;
            }
        }
    }

    ivi_extension_layer_set_render_order(shell, ivilayer->layout_layer,
                                   layoutsurf_array, i);
    free(layoutsurf_array);
}

static void
controller_layer_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    (void)destroy_scene_object;

    if (ivilayer->layout_layer != NULL) {
        ivi_extension_layer_remove(ivilayer->shell, ivilayer->layout_layer);
        ivilayer->layout_layer = NULL;
    }

    wl_resource_destroy(resource);

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
    (void)client;
    ivi_extension_screen_set_render_order(iviscrn->shell, iviscrn->layout_screen, NULL, 0);
}

static void
controller_screen_add_layer(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_resource *layer)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivilayer *ivilayer = wl_resource_get_user_data(layer);
    (void)client;
    ivi_extension_screen_add_layer(iviscrn->shell, iviscrn->layout_screen, ivilayer->layout_layer);
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

    struct weston_output *output = NULL;
    l = malloc(sizeof *l);
    if(l == NULL) {
        fprintf(stderr, "fails to allocate memory\n");
        return;
    }

    l->filename = malloc(strlen(filename));
    if(l->filename == NULL) {
        fprintf(stderr, "fails to allocate memory\n");
        free(l);
        return;
    }

    output = ivi_extension_screen_get_output(iviscrn->shell, iviscrn->layout_screen);
    strcpy(l->filename, filename);
    l->listener.notify = controller_screenshot_notify;
    wl_signal_add(&output->frame_signal, &l->listener);
    output->disable_planes++;
    weston_output_schedule_repaint(output);
    return;
}

static void
controller_screen_set_render_order(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_array *id_layers)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivi_layout_layer **layoutlayer_array = NULL;
    struct ivilayer *ivilayer = NULL;
    uint32_t *id_layer = NULL;
    uint32_t id_layout_layer = 0;
    int i = 0;
    (void)client;
    struct ivishell *shell = iviscrn->shell;

    layoutlayer_array = (struct ivi_layout_layer**)calloc(
                           id_layers->size, sizeof(void*));

    wl_array_for_each(id_layer, id_layers) {
        wl_list_for_each(ivilayer, &iviscrn->shell->list_layer, link) {
            id_layout_layer = ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);
            if (*id_layer == id_layout_layer) {
                layoutlayer_array[i] = ivilayer->layout_layer;
                i++;
                break;
            }
        }
    }

    ivi_extension_screen_set_render_order(shell, iviscrn->layout_screen,
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

    ans = ivi_extension_commit_changes(controller->shell);
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
    struct ivi_layout_layer *layout_layer = NULL;
    struct ivilayer *ivilayer = NULL;
    const struct ivi_layout_layer_properties *prop;

    layout_layer = ivi_extension_get_layer_from_id(shell, id_layer);
    if (layout_layer == NULL) {
        layout_layer = ivi_extension_layer_create_with_dimension(shell, id_layer,
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

    prop = ivi_extension_get_properties_of_layer(shell, ivilayer->layout_layer);
    send_layer_event(layer_resource, ivilayer,
                     prop, IVI_NOTIFICATION_ALL);
}

static void
surface_event_content(struct ivi_layout_surface *layout_surface, int32_t content, void *userdata)
{
    if (content == 0) {
        surface_event_remove(layout_surface, userdata);
    }
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
    const struct ivi_layout_surface_properties *prop;
    struct ivi_layout_surface *layout_surface = NULL;
    struct ivisurface *ivisurf = NULL;

    layout_surface = ivi_extension_get_surface_from_id(shell, id_surface);
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

    prop = ivi_extension_get_properties_of_surface(shell, ivisurf->layout_surface);
    ivi_extension_surface_set_content_observer(shell, ivisurf->layout_surface, surface_event_content, shell);

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
        id_layout_layer =
            ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);

        ivi_controller_send_layer(controller->resource,
                                  id_layout_layer);
    }
    wl_list_for_each_reverse(ivisurf, &shell->list_surface, link) {
        id_layout_surface =
            ivi_extension_get_id_of_surface(shell, ivisurf->layout_surface);

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

    controller = calloc(1, sizeof *controller);
    if (controller == NULL) {
        weston_log("no memory to allocate controller\n");
        return;
    }

    controller->resource =
        wl_resource_create(client, &ivi_controller_interface, 1, id);
    wl_resource_set_implementation(controller->resource,
                                   &controller_implementation,
                                   controller, unbind_resource_controller);

    controller->shell = shell;
    controller->client = client;
    controller->id = id;

    wl_list_insert(&shell->list_controller, &controller->link);

    add_client_to_resources(shell, client, controller);
}

static struct iviscreen*
create_screen(struct ivishell *shell, struct weston_output *output)
{
    struct iviscreen *iviscrn;
    static int id_counter = 0;
    iviscrn = calloc(1, sizeof *iviscrn);
    if (iviscrn == NULL) {
        weston_log("no memory to allocate client screen\n");
        return NULL;
    }

    iviscrn->shell = shell;
    iviscrn->output = output;

    iviscrn->layout_screen = ivi_extension_get_screen_from_id(shell, id_counter++);

    wl_list_init(&iviscrn->link);
    wl_list_init(&iviscrn->resource_list);

    return iviscrn;
}

static struct ivilayer*
create_layer(struct ivishell *shell,
             struct ivi_layout_layer *layout_layer,
             uint32_t id_layer)
{
    struct ivilayer *ivilayer = NULL;
    struct ivicontroller *controller = NULL;

    ivilayer = calloc(1, sizeof *ivilayer);
    if (NULL == ivilayer) {
        weston_log("no memory to allocate client layer\n");
        return NULL;
    }

    ivilayer->shell = shell;
    wl_list_insert(&shell->list_layer, &ivilayer->link);
    wl_list_init(&ivilayer->resource_list);
    ivilayer->layout_layer = layout_layer;

    ivi_extension_layer_add_notification(shell, layout_layer, send_layer_prop, ivilayer);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_layer(controller->resource, id_layer);
    }

    return ivilayer;
}

static struct ivisurface*
create_surface(struct ivishell *shell,
               struct ivi_layout_surface *layout_surface,
               uint32_t id_surface)
{
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller *controller = NULL;

    ivisurf = calloc(1, sizeof *ivisurf);
    if (ivisurf == NULL) {
        weston_log("no memory to allocate client surface\n");
        return NULL;
    }

    ivisurf->shell = shell;
    ivisurf->layout_surface = layout_surface;
    wl_list_insert(&shell->list_surface, &ivisurf->link);
    wl_list_init(&ivisurf->resource_list);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_surface(controller->resource,
                                    id_surface);
    }

    ivi_extension_surface_add_notification(shell, layout_surface,
                                    send_surface_prop, ivisurf);

    return ivisurf;
}

static void
layer_event_create(struct ivi_layout_layer *layout_layer,
                     void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivilayer *ivilayer = NULL;
    uint32_t id_layer = 0;

    id_layer = ivi_extension_get_id_of_layer(shell, layout_layer);

    ivilayer = create_layer(shell, layout_layer, id_layer);
    if (ivilayer == NULL) {
        weston_log("failed to create layer");
        return;
    }
}

static void
layer_event_remove(struct ivi_layout_layer *layout_layer,
                     void *userdata)
{
    struct wl_resource *resource;
    struct ivishell *shell = userdata;
    struct ivilayer *ivilayer = NULL;

    ivilayer = get_layer(&shell->list_layer, layout_layer);
    if (ivilayer == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    wl_resource_for_each(resource, &ivilayer->resource_list) {
            ivi_controller_layer_send_destroyed(resource);
    }

    wl_list_remove(&ivilayer->link);
    free(ivilayer);
}


static void
surface_event_create(struct ivi_layout_surface *layout_surface,
                     void *userdata)
{
    struct wl_resource *resource;
    struct ivishell *shell = userdata;
    struct ivisurface *ivisurf = NULL;
    uint32_t id_surface = 0;

    id_surface = ivi_extension_get_id_of_surface(shell, layout_surface);

    ivisurf = create_surface(shell, layout_surface, id_surface);
    if (ivisurf == NULL) {
        weston_log("failed to create surface");
        return;
    }

    wl_resource_for_each(resource, &ivisurf->resource_list) {
        ivi_controller_surface_send_content(resource, IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_AVAILABLE);
    }
}

static void
surface_event_remove(struct ivi_layout_surface *layout_surface,
                     void *userdata)
{
    struct wl_resource *resource;
    struct ivishell *shell = userdata;
    struct ivisurface *ivisurf = NULL;

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    wl_resource_for_each(resource, &ivisurf->resource_list) {
            ivi_controller_surface_send_content(resource, IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED);
            ivi_controller_surface_send_destroyed(resource);
    }

    wl_list_remove(&ivisurf->link);
    free(ivisurf);
}

static void
surface_event_configure(struct ivi_layout_surface *layout_surface,
                        void *userdata)
{
    struct wl_resource *resource;
    struct ivishell *shell = userdata;
    struct ivisurface *ivisurf = NULL;
    const struct ivi_layout_surface_properties *prop;

    ivisurf = get_surface(&shell->list_surface, layout_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    prop = ivi_extension_get_properties_of_surface(shell, layout_surface);

    wl_resource_for_each(resource, &ivisurf->resource_list) {
        send_surface_event(resource, ivisurf,
                           prop, IVI_NOTIFICATION_CONFIGURE);
    }
}

static int32_t
check_layout_layers(struct ivishell *shell)
{
    struct ivi_layout_layer **pArray = NULL;
    struct ivilayer *ivilayer = NULL;
    uint32_t id_layer = 0;
    int32_t length = 0;
    uint32_t i = 0;
    int32_t ret = 0;

    ret = ivi_extension_get_layers(shell, &length, &pArray);
    if(ret != 0) {
        weston_log("failed to get layers at check_layout_layers\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_layer = ivi_extension_get_id_of_layer(shell, pArray[i]);
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
    uint32_t id_surface = 0;
    int32_t length = 0;
    uint32_t i = 0;
    int32_t ret = 0;

    ret = ivi_extension_get_surfaces(shell, &length, &pArray);
    if(ret != 0) {
        weston_log("failed to get surfaces at check_layout_surfaces\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_surface = ivi_extension_get_id_of_surface(shell, pArray[i]);
        ivisurf = create_surface(shell, pArray[i], id_surface);
        if (ivisurf == NULL) {
            weston_log("failed to create surface");
        }
    }

    free(pArray);
    pArray = NULL;

    return 0;
}

void
init_ivi_shell(struct weston_compositor *ec, struct ivishell *shell)
{
    struct weston_output *output = NULL;
    struct iviscreen *iviscrn = NULL;
    int32_t ret = 0;

    shell->compositor = ec;

    wl_list_init(&shell->list_surface);
    wl_list_init(&shell->list_layer);
    wl_list_init(&shell->list_screen);
    wl_list_init(&shell->list_controller);

    wl_list_for_each(output, &ec->output_list, link) {
        iviscrn = create_screen(shell, output);
        if (iviscrn != NULL) {
            wl_list_insert(&shell->list_screen, &iviscrn->link);
        }
    }

    ret = check_layout_layers(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_layers");
    }

    ret = check_layout_surfaces(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_surfaces");
    }

    ivi_extension_add_notification_create_layer(shell, layer_event_create, shell);
    ivi_extension_add_notification_remove_layer(shell, layer_event_remove, shell);

    ivi_extension_add_notification_create_surface(shell, surface_event_create, shell);
    ivi_extension_add_notification_remove_surface(shell, surface_event_remove, shell);

    ivi_extension_add_notification_configure_surface(shell, surface_event_configure, shell);
}

int
setup_ivi_controller_server(struct weston_compositor *compositor,
                            struct ivishell *shell)
{
    if (wl_global_create(compositor->wl_display, &ivi_controller_interface, 1,
                         shell, bind_ivi_controller) == NULL) {
        return -1;
    }

    return 0;
}
