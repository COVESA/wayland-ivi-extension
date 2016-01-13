/**************************************************************************
 *
 * Copyright (C) 2013 DENSO CORPORATION
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
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include <unistd.h>
#include <poll.h>

#include <sys/eventfd.h>

#include "ilm_common.h"
#include "ilm_control_platform.h"
#include "wayland-util.h"
#include "ivi-controller-client-protocol.h"
#include "ivi-input-client-protocol.h"

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

struct layer_context {
    struct wl_list link;

    struct ivi_controller_layer *controller;
    t_ilm_uint id_layer;

    struct ilmLayerProperties prop;
    layerNotificationFunc notification;

    struct {
        struct wl_list list_surface;
        struct wl_list link;
    } order;

    struct wayland_context *ctx;
};

struct screen_context {
    struct wl_list link;

    struct wl_output *output;
    struct ivi_controller_screen *controller;
    t_ilm_uint id_from_server;
    t_ilm_uint id_screen;
    int32_t transform;

    struct ilmScreenProperties prop;

    struct {
        struct wl_list list_layer;
    } order;

    struct ilm_control_context *ctx;
};

static inline void lock_context(struct ilm_control_context *ctx)
{
   pthread_mutex_lock(&ctx->mutex);
}

static inline void unlock_context(struct ilm_control_context *ctx)
{
   pthread_mutex_unlock(&ctx->mutex);
}

static int init_control(void);

static struct surface_context* get_surface_context(struct wayland_context *, uint32_t);

void release_instance(void);

static int create_controller_layer(struct wayland_context *ctx, t_ilm_uint width, t_ilm_uint height, t_ilm_layer layerid);

static int32_t
wayland_controller_is_inside_layer_list(struct wl_list *list,
                                        uint32_t id_layer)
{
    struct layer_context *ctx_layer = NULL;
    wl_list_for_each(ctx_layer, list, link) {
        if (ctx_layer->id_layer == id_layer) {
            return 1;
        }
    }

    return 0;
}

static struct layer_context*
wayland_controller_get_layer_context(struct wayland_context *ctx,
                                     uint32_t id_layer)
{
    struct layer_context *ctx_layer = NULL;

    if (ctx->controller == NULL) {
        fprintf(stderr, "controller is not initialized in ilmControl\n");
        return NULL;
    }

    wl_list_for_each(ctx_layer, &ctx->list_layer, link) {
        if (ctx_layer->id_layer == id_layer) {
            return ctx_layer;
        }
    }

    return NULL;
}

static void
output_listener_geometry(void *data,
                         struct wl_output *output,
                         int32_t x,
                         int32_t y,
                         int32_t physical_width,
                         int32_t physical_height,
                         int32_t subpixel,
                         const char *make,
                         const char *model,
                         int32_t transform)
{
    (void)output;
    (void)x;
    (void)y;
    (void)subpixel;
    (void)make;
    (void)model;

    struct screen_context *ctx_scrn = data;
    ctx_scrn->transform = transform;
}

static void
output_listener_mode(void *data,
                     struct wl_output *output,
                     uint32_t flags,
                     int32_t width,
                     int32_t height,
                     int32_t refresh)
{
    (void)output;
    (void)refresh;

    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        struct screen_context *ctx_scrn = data;
        if (ctx_scrn->transform == WL_OUTPUT_TRANSFORM_90 ||
            ctx_scrn->transform == WL_OUTPUT_TRANSFORM_270 ||
            ctx_scrn->transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
            ctx_scrn->transform == WL_OUTPUT_TRANSFORM_FLIPPED_270) {
            ctx_scrn->prop.screenWidth = height;
            ctx_scrn->prop.screenHeight = width;
        } else {
            ctx_scrn->prop.screenWidth = width;
            ctx_scrn->prop.screenHeight = height;
        }
    }
}

static void
output_listener_done(void *data,
                     struct wl_output *output)
{
    (void)data;
    (void)output;
}

static void
output_listener_scale(void *data,
                      struct wl_output *output,
                      int32_t factor)
{
    (void)data;
    (void)output;
    (void)factor;
}

static struct wl_output_listener output_listener = {
    output_listener_geometry,
    output_listener_mode,
    output_listener_done,
    output_listener_scale
};

static struct screen_context*
get_screen_context_by_serverid(struct wayland_context *ctx,
                               uint32_t id_screen)
{
    struct screen_context *ctx_scrn = NULL;

    wl_list_for_each(ctx_scrn, &ctx->list_screen, link) {
        if (ctx_scrn->id_from_server == id_screen) {
            return ctx_scrn;
        }
    }
    return NULL;
}

static void
add_orderlayer_to_screen(struct layer_context *ctx_layer,
                         struct wl_output* output)
{
    struct screen_context *ctx_scrn = wl_output_get_user_data(output);

    int found = 0;
    struct layer_context *layer_link;
    wl_list_for_each(layer_link, &ctx_scrn->order.list_layer, order.link) {
        if (layer_link == ctx_layer) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        wl_list_insert(&ctx_scrn->order.list_layer, &ctx_layer->order.link);
    }
}

static void
remove_orderlayer_from_screen(struct layer_context *ctx_layer)
{
    wl_list_remove(&ctx_layer->order.link);
    wl_list_init(&ctx_layer->order.link);
}

static void
controller_layer_listener_visibility(void *data,
                            struct ivi_controller_layer *controller,
                            int32_t visibility)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.visibility = (t_ilm_bool)visibility;

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_VISIBILITY);
    }
}

static void
controller_layer_listener_opacity(void *data,
                       struct ivi_controller_layer *controller,
                       wl_fixed_t opacity)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.opacity = (t_ilm_float)wl_fixed_to_double(opacity);

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_OPACITY);
    }
}

static void
controller_layer_listener_source_rectangle(void *data,
                                struct ivi_controller_layer *controller,
                                int32_t x,
                                int32_t y,
                                int32_t width,
                                int32_t height)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.sourceX = (t_ilm_uint)x;
    ctx_layer->prop.sourceY = (t_ilm_uint)y;
    ctx_layer->prop.sourceWidth = (t_ilm_uint)width;
    ctx_layer->prop.sourceHeight = (t_ilm_uint)height;
    if (ctx_layer->prop.origSourceWidth == 0) {
        ctx_layer->prop.origSourceWidth = (t_ilm_uint)width;
    }
    if (ctx_layer->prop.origSourceHeight == 0) {
        ctx_layer->prop.origSourceHeight = (t_ilm_uint)height;
    }

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_SOURCE_RECT);
    }
}

static void
controller_layer_listener_destination_rectangle(void *data,
                                     struct ivi_controller_layer *controller,
                                     int32_t x,
                                     int32_t y,
                                     int32_t width,
                                     int32_t height)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.destX = (t_ilm_uint)x;
    ctx_layer->prop.destY = (t_ilm_uint)y;
    ctx_layer->prop.destWidth = (t_ilm_uint)width;
    ctx_layer->prop.destHeight = (t_ilm_uint)height;

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_DEST_RECT);
    }
}

static void
controller_layer_listener_configuration(void *data,
                          struct ivi_controller_layer *controller,
                          int32_t width,
                          int32_t height)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.sourceWidth = (t_ilm_uint)width;
    ctx_layer->prop.sourceHeight = (t_ilm_uint)height;
}

static void
controller_layer_listener_orientation(void *data,
                             struct ivi_controller_layer *controller,
                             int32_t orientation)
{
    ilmOrientation ilmorientation = ILM_ZERO;
    struct layer_context *ctx_layer = data;

    switch(orientation) {
    case IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES:
        ilmorientation = ILM_ZERO;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES:
        ilmorientation = ILM_NINETY;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES:
        ilmorientation = ILM_ONEHUNDREDEIGHTY;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES:
        ilmorientation = ILM_TWOHUNDREDSEVENTY;
        break;
    default:
        break;
    }

    ctx_layer->prop.orientation = ilmorientation;

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_ORIENTATION);
    }
}

static void
controller_layer_listener_screen(void *data,
                                 struct ivi_controller_layer *controller,
                                 struct wl_output *output)
{
    struct layer_context *ctx_layer = data;

    if (output == NULL) {
        remove_orderlayer_from_screen(ctx_layer);
    } else {
        add_orderlayer_to_screen(ctx_layer, output);
    }
}

static void
remove_ordersurface_from_layer(struct surface_context *ctx_surf);

static void
controller_layer_listener_destroyed(void *data,
                                    struct ivi_controller_layer *controller)
{
    struct layer_context *ctx_layer = data;
    struct surface_context *ctx_surf = NULL;
    struct surface_context *ctx_surf_next = NULL;

    wl_list_remove(&ctx_layer->order.link);
    wl_list_remove(&ctx_layer->link);

    wl_list_for_each_safe(ctx_surf, ctx_surf_next,
                          &ctx_layer->order.list_surface, order.link) {
        remove_ordersurface_from_layer(ctx_surf);
    }

    if (ctx_layer->ctx->notification != NULL) {
        ilmObjectType layer = ILM_LAYER;
        ctx_layer->ctx->notification(layer, ctx_layer->id_layer, ILM_FALSE,
                                     ctx_layer->ctx->notification_user_data);
    }

    free(ctx_layer);
}

static struct ivi_controller_layer_listener controller_layer_listener =
{
    controller_layer_listener_visibility,
    controller_layer_listener_opacity,
    controller_layer_listener_source_rectangle,
    controller_layer_listener_destination_rectangle,
    controller_layer_listener_configuration,
    controller_layer_listener_orientation,
    controller_layer_listener_screen,
    controller_layer_listener_destroyed
};

static void
add_ordersurface_to_layer(struct surface_context *ctx_surf,
                          struct ivi_controller_layer *layer)
{
    struct layer_context *ctx_layer = NULL;
    struct surface_context *link = NULL;
    int found = 0;

    ctx_layer = ivi_controller_layer_get_user_data(layer);

    wl_list_for_each(link, &ctx_layer->order.list_surface, order.link) {
        if (link == ctx_surf) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        wl_list_insert(&ctx_layer->order.list_surface, &ctx_surf->order.link);
    }
}

static void
remove_ordersurface_from_layer(struct surface_context *ctx_surf)
{
    wl_list_remove(&ctx_surf->order.link);
    wl_list_init(&ctx_surf->order.link);
}

static void
controller_surface_listener_visibility(void *data,
                            struct ivi_controller_surface *controller,
                            int32_t visibility)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.visibility = (t_ilm_bool)visibility;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_VISIBILITY);
    }
}

static void
controller_surface_listener_opacity(void *data,
                         struct ivi_controller_surface *controller,
                         wl_fixed_t opacity)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.opacity = (t_ilm_float)wl_fixed_to_double(opacity);

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_OPACITY);
    }
}

static void
controller_surface_listener_configuration(void *data,
                           struct ivi_controller_surface *controller,
                           int32_t width,
                           int32_t height)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.origSourceWidth = (t_ilm_uint)width;
    ctx_surf->prop.origSourceHeight = (t_ilm_uint)height;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_CONFIGURED);
    }
}

static void
controller_surface_listener_source_rectangle(void *data,
                                  struct ivi_controller_surface *controller,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.sourceX = (t_ilm_uint)x;
    ctx_surf->prop.sourceY = (t_ilm_uint)y;
    ctx_surf->prop.sourceWidth = (t_ilm_uint)width;
    ctx_surf->prop.sourceHeight = (t_ilm_uint)height;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_SOURCE_RECT);
    }
}

static void
controller_surface_listener_destination_rectangle(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.destX = (t_ilm_uint)x;
    ctx_surf->prop.destY = (t_ilm_uint)y;
    ctx_surf->prop.destWidth = (t_ilm_uint)width;
    ctx_surf->prop.destHeight = (t_ilm_uint)height;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_DEST_RECT);
    }
}

static void
controller_surface_listener_orientation(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t orientation)
{
    struct surface_context *ctx_surf = data;
    ilmOrientation ilmorientation = ILM_ZERO;

    switch (orientation) {
    case IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES:
        ilmorientation = ILM_ZERO;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES:
        ilmorientation = ILM_NINETY;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES:
        ilmorientation = ILM_ONEHUNDREDEIGHTY;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES:
        ilmorientation = ILM_TWOHUNDREDSEVENTY;
        break;
    default:
        break;
    }

    ctx_surf->prop.orientation = ilmorientation;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_ORIENTATION);
    }
}

static void
controller_surface_listener_pixelformat(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t pixelformat)
{
    struct surface_context *ctx_surf = data;

    ctx_surf->prop.pixelformat = (t_ilm_uint)pixelformat;
}

static void
controller_surface_listener_layer(void *data,
                                  struct ivi_controller_surface *controller,
                                  struct ivi_controller_layer *layer)
{
    struct surface_context *ctx_surf = data;

    if (layer == NULL) {
        remove_ordersurface_from_layer(ctx_surf);
    } else {
        add_ordersurface_to_layer(ctx_surf, layer);
    }
}

static void
controller_surface_listener_stats(void *data,
                                  struct ivi_controller_surface *controller,
                                  uint32_t redraw_count,
                                  uint32_t frame_count,
                                  uint32_t update_count,
                                  uint32_t pid,
                                  const char *process_name)
{
    struct surface_context *ctx_surf = data;
    (void)process_name;

    ctx_surf->prop.drawCounter = (t_ilm_uint)redraw_count;
    ctx_surf->prop.frameCounter = (t_ilm_uint)frame_count;
    ctx_surf->prop.updateCounter = (t_ilm_uint)update_count;
    ctx_surf->prop.creatorPid = (t_ilm_uint)pid;
}

static void
controller_surface_listener_destroyed(void *data,
                  struct ivi_controller_surface *controller)
{
    struct surface_context *ctx_surf = data;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                               &ctx_surf->prop,
                               ILM_NOTIFICATION_CONTENT_REMOVED);
    }

    wl_list_remove(&ctx_surf->order.link);
    wl_list_remove(&ctx_surf->link);
    free(ctx_surf);
}

static void
controller_surface_listener_content(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t content_state)
{
    struct surface_context *ctx_surf = data;

    // if client surface (=content) was removed with ilm_surfaceDestroy()
    // the expected behavior within ILM API mandates a full removal
    // of the surface from the scene. We must remove the controller
    // from scene, too.
    if (IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED == content_state)
    {
        if (ctx_surf->notification != NULL) {
            ctx_surf->notification(ctx_surf->id_surface,
                                   &ctx_surf->prop,
                                   ILM_NOTIFICATION_CONTENT_REMOVED);
        }

        if (ctx_surf->ctx->notification != NULL) {
            ilmObjectType surface = ILM_SURFACE;
            ctx_surf->ctx->notification(surface, ctx_surf->id_surface, ILM_FALSE,
                                        ctx_surf->ctx->notification_user_data);
        }

        ivi_controller_surface_destroy(controller, 1);

        wl_list_remove(&ctx_surf->order.link);
        wl_list_remove(&ctx_surf->link);
        free(ctx_surf);
    }
    else if (IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_AVAILABLE == content_state)
    {
        if (ctx_surf->notification != NULL) {
            ctx_surf->notification(ctx_surf->id_surface,
                                    &ctx_surf->prop,
                                    ILM_NOTIFICATION_CONTENT_AVAILABLE);
        }
    }
}

static struct ivi_controller_surface_listener controller_surface_listener=
{
    controller_surface_listener_visibility,
    controller_surface_listener_opacity,
    controller_surface_listener_source_rectangle,
    controller_surface_listener_destination_rectangle,
    controller_surface_listener_configuration,
    controller_surface_listener_orientation,
    controller_surface_listener_pixelformat,
    controller_surface_listener_layer,
    controller_surface_listener_stats,
    controller_surface_listener_destroyed,
    controller_surface_listener_content
};

static void
controller_listener_layer(void *data,
                          struct ivi_controller *controller,
                          uint32_t id_layer)
{
   struct wayland_context *ctx = data;

   if (wayland_controller_is_inside_layer_list(&ctx->list_layer, id_layer))
   {
      return;
   }

   (void) create_controller_layer(ctx, 0, 0, id_layer);
}

static void
controller_listener_surface(void *data,
                            struct ivi_controller *controller,
                            uint32_t id_surface)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(ctx, id_surface);
    if (ctx_surf != NULL) {
        if (!ctx_surf->is_surface_creation_noticed) {
            ctx_surf->controller = ivi_controller_surface_create(
                                       controller, id_surface);
            ivi_controller_surface_add_listener(ctx_surf->controller,
                                       &controller_surface_listener, ctx_surf);
            if (ctx_surf->notification != NULL) {
                ctx_surf->notification(ctx_surf->id_surface,
                                       &ctx_surf->prop,
                                       ILM_NOTIFICATION_CONTENT_AVAILABLE);
                ctx_surf->is_surface_creation_noticed = true;
            }
        }
        else {
            fprintf(stderr, "invalid id_surface in controller_listener_surface\n");
        }
        return;
    }

    ctx_surf = calloc(1, sizeof *ctx_surf);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface_context\n");
        return;
    }

    ctx_surf->controller = ivi_controller_surface_create(
                               controller, id_surface);
    if (ctx_surf->controller == NULL) {
        free(ctx_surf);
        fprintf(stderr, "Failed to create controller surface\n");
        return;
    }
    ctx_surf->id_surface = id_surface;
    ctx_surf->ctx = ctx;
    ctx_surf->is_surface_creation_noticed = true;

    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
    wl_list_init(&ctx_surf->order.link);
    wl_list_init(&ctx_surf->list_accepted_seats);
    ivi_controller_surface_add_listener(ctx_surf->controller,
                                        &controller_surface_listener, ctx_surf);

    if (ctx->notification != NULL) {
        ilmObjectType surface = ILM_SURFACE;
        ctx->notification(surface, ctx_surf->id_surface, ILM_TRUE,
                          ctx->notification_user_data);
    }
}

static void
controller_listener_error(void *data,
                          struct ivi_controller *ivi_controller,
	                  int32_t object_id,
	                  int32_t object_type,
	                  int32_t error_code,
	                  const char *error_text)
{
    (void)data;
    (void)ivi_controller;
    (void)object_id;
    (void)object_type;
    (void)error_code;
    (void)error_text;
}

static void
controller_listener_screen(void *data,
                           struct ivi_controller *ivi_controller,
                           uint32_t id_screen,
                           struct ivi_controller_screen *controller_screen)
{
    struct wayland_context *ctx = data;
    struct screen_context *ctx_screen;
    (void)ivi_controller;

    ctx_screen = get_screen_context_by_serverid(ctx, id_screen);
    if (ctx_screen == NULL) {
        fprintf(stderr, "Failed to allocate memory for screen_context\n");
        return;
    }
    ctx_screen->controller = controller_screen;
}

static struct ivi_controller_listener controller_listener= {
    controller_listener_screen,
    controller_listener_layer,
    controller_listener_surface,
    controller_listener_error
};

static struct seat_context *
find_seat(struct wl_list *list, const char *name)
{
    struct seat_context *seat;
    wl_list_for_each(seat, list, link) {
        if (strcmp(name, seat->seat_name) == 0)
            return seat;
    }
    return NULL;
}

static void
input_listener_seat_created(void *data,
                            struct ivi_input *ivi_input,
                            const char *name,
                            uint32_t capabilities)
{
    struct wayland_context *ctx = data;
    struct seat_context *seat;
    seat = find_seat(&ctx->list_seat, name);
    if (seat) {
        fprintf(stderr, "Warning: seat context was created twice!\n");
        seat->capabilities = capabilities;
        return;
    }
    seat = calloc(1, sizeof *seat);
    if (seat == NULL) {
        fprintf(stderr, "Failed to allocate memory for seat context\n");
        return;
    }
    seat->seat_name = strdup(name);
    seat->capabilities = capabilities;
    wl_list_insert(&ctx->list_seat, &seat->link);
}

static void
input_listener_seat_capabilities(void *data,
                                 struct ivi_input *ivi_input,
                                 const char *name,
                                 uint32_t capabilities)
{
    struct wayland_context *ctx = data;
    struct seat_context *seat = find_seat(&ctx->list_seat, name);
    if (seat == NULL) {
        fprintf(stderr, "Warning: Cannot find seat for name %s\n", name);
        return;
    }
    seat->capabilities = capabilities;
}

static void
input_listener_seat_destroyed(void *data,
                              struct ivi_input *ivi_input,
                              const char *name)
{
    struct wayland_context *ctx = data;
    struct seat_context *seat = find_seat(&ctx->list_seat, name);
    if (seat == NULL) {
        fprintf(stderr, "Warning: Cannot find seat %s to delete it\n", name);
        return;
    }
    free(seat->seat_name);
    wl_list_remove(&seat->link);
    free(seat);
}

static void
input_listener_input_focus(void *data,
                           struct ivi_input *ivi_input,
                           uint32_t surface, uint32_t device, int32_t enabled)
{
    struct wayland_context *ctx = data;
    struct surface_context *surf_ctx;
    wl_list_for_each(surf_ctx, &ctx->list_surface, link) {
        if (surface != surf_ctx->id_surface)
            continue;

        if (enabled == ILM_TRUE)
            surf_ctx->prop.focus |= device;
        else
            surf_ctx->prop.focus &= ~device;
    }
}

static void
input_listener_input_acceptance(void *data,
                                struct ivi_input *ivi_input,
                                uint32_t surface,
                                const char *seat,
                                int32_t accepted)
{
    struct accepted_seat *accepted_seat, *next;
    struct wayland_context *ctx = data;
    struct surface_context *surface_ctx = NULL;
    int surface_found = 1;
    int accepted_seat_found = 0;

    wl_list_for_each(surface_ctx, &ctx->list_surface, link) {
        if (surface_ctx->id_surface == surface) {
            surface_found = 1;
            break;
        }
    }

    if (!surface_found) {
        fprintf(stderr, "Warning: input acceptance event received for "
                "nonexistent surface %d\n", surface);
        return;
    }

    wl_list_for_each_safe(accepted_seat, next,
                          &surface_ctx->list_accepted_seats, link) {
        if (strcmp(accepted_seat->seat_name, seat) != 0)
            continue;

        if (accepted != ILM_TRUE) {
            /* Remove this from the accepted seats */
            free(accepted_seat->seat_name);
            wl_list_remove(&accepted_seat->link);
            free(accepted_seat);
            return;
        }
        accepted_seat_found = 1;
    }

    if (accepted_seat_found && accepted == ILM_TRUE) {
        fprintf(stderr, "Warning: input acceptance event trying to add seat "
                "%s, that is already in surface %d\n", seat, surface);
        return;
    }
    if (!accepted_seat_found && accepted != ILM_TRUE) {
        fprintf(stderr, "Warning: input acceptance event trying to remove "
                "seat %s, that is not in surface %d\n", seat, surface);
        return;
    }

    accepted_seat = calloc(1, sizeof(accepted_seat));
    if (accepted_seat == NULL) {
        fprintf(stderr, "Failed to allocate memory for accepted seat\n");
        return;
    }
    accepted_seat->seat_name = strdup(seat);
    wl_list_insert(&surface_ctx->list_accepted_seats, &accepted_seat->link);
}

static struct ivi_input_listener input_listener = {
    input_listener_seat_created,
    input_listener_seat_capabilities,
    input_listener_seat_destroyed,
    input_listener_input_focus,
    input_listener_input_acceptance
};

static void
registry_handle_control(void *data,
                       struct wl_registry *registry,
                       uint32_t name, const char *interface,
                       uint32_t version)
{
    struct wayland_context *ctx = data;
    (void)version;

    if (strcmp(interface, "ivi_controller") == 0) {
        ctx->controller = wl_registry_bind(registry, name,
                                           &ivi_controller_interface, 1);
        if (ctx->controller == NULL) {
            fprintf(stderr, "Failed to registry bind ivi_controller\n");
            return;
        }
        if (ivi_controller_add_listener(ctx->controller,
                                       &controller_listener,
                                       ctx)) {
            fprintf(stderr, "Failed to add ivi_controller listener\n");
            return;
        }
    } else if (strcmp(interface, "ivi_input") == 0) {
        ctx->input_controller =
            wl_registry_bind(registry, name, &ivi_input_interface, 1);

        if (ctx->input_controller == NULL) {
            fprintf(stderr, "Failed to registry bind input controller\n");
            return;
        }
        if (ivi_input_add_listener(ctx->input_controller,
                                              &input_listener, ctx)) {
            fprintf(stderr, "Failed to add ivi_input listener\n");
            return;
        }
    } else if (strcmp(interface, "wl_output") == 0) {
        struct screen_context *ctx_scrn = calloc(1, sizeof *ctx_scrn);
        struct wl_proxy *pxy = NULL;

        if (ctx_scrn == NULL) {
            fprintf(stderr, "Failed to allocate memory for screen_context\n");
            return;
        }
        ctx_scrn->output = wl_registry_bind(registry, name,
                                           &wl_output_interface, 1);
        if (ctx_scrn->output == NULL) {
            free(ctx_scrn);
            fprintf(stderr, "Failed to registry bind wl_output\n");
            return;
        }

        if (wl_output_add_listener(ctx_scrn->output,
                                   &output_listener,
                                   ctx_scrn)) {
            free(ctx_scrn);
            fprintf(stderr, "Failed to add wl_output listener\n");
            return;
        }

        pxy = (struct wl_proxy*)ctx_scrn->output;
        ctx_scrn->id_from_server = wl_proxy_get_id(pxy);
        ctx_scrn->id_screen = ctx->num_screen;
        ctx->num_screen++;
        wl_list_init(&ctx_scrn->order.list_layer);
        wl_list_insert(&ctx->list_screen, &ctx_scrn->link);
    }
}

static const struct wl_registry_listener
registry_control_listener= {
    registry_handle_control,
    NULL
};

struct ilm_control_context ilm_context;

static void destroy_control_resources(void)
{
    struct ilm_control_context *ctx = &ilm_context;

    // free resources of output objects
    if (! ctx->wl.controller) {
        struct screen_context *ctx_scrn;
        struct screen_context *next;

        wl_list_for_each_safe(ctx_scrn, next, &ctx->wl.list_screen, link) {
            if (ctx_scrn->output != NULL) {
                wl_output_destroy(ctx_scrn->output);
            }

            wl_list_remove(&ctx_scrn->link);
            free(ctx_scrn);
        }
    }

    if (ctx->wl.controller != NULL) {
        {
            struct surface_context *l;
            struct surface_context *n;
            wl_list_for_each_safe(l, n, &ctx->wl.list_surface, link) {
                wl_list_remove(&l->link);
                wl_list_remove(&l->order.link);
                ivi_controller_surface_destroy(l->controller, 0);
                free(l);
            }
        }

        {
            struct layer_context *l;
            struct layer_context *n;
            wl_list_for_each_safe(l, n, &ctx->wl.list_layer, link) {
                wl_list_remove(&l->link);
                wl_list_remove(&l->order.link);
                free(l);
            }
        }

        {
            struct screen_context *ctx_scrn;
            struct screen_context *next;

            wl_list_for_each_safe(ctx_scrn, next, &ctx->wl.list_screen, link) {
                if (ctx_scrn->output != NULL) {
                    wl_output_destroy(ctx_scrn->output);
                }

                wl_list_remove(&ctx_scrn->link);
                ivi_controller_screen_destroy(ctx_scrn->controller);
                free(ctx_scrn);
            }
        }

        ivi_controller_destroy(ctx->wl.controller);
        ctx->wl.controller = NULL;
    }

    if (ctx->wl.display) {
        wl_display_flush(ctx->wl.display);
    }

    if (ctx->wl.registry) {
        wl_registry_destroy(ctx->wl.registry);
        ctx->wl.registry = NULL;
    }

    if (ctx->wl.queue) {
        wl_event_queue_destroy(ctx->wl.queue);
        ctx->wl.queue = NULL;
    }

    if (0 != pthread_mutex_destroy(&ctx->mutex)) {
        fprintf(stderr, "failed to destroy pthread_mutex\n");
    }
}

static void send_shutdown_event(struct ilm_control_context *ctx)
{
    uint64_t buf = 1;
    while (write(ctx->shutdown_fd, &buf, sizeof buf) == -1 && errno == EINTR)
       ;
}

ILM_EXPORT void
ilmControl_destroy(void)
{
    struct ilm_control_context *ctx = &ilm_context;

    if (!ctx->initialized)
    {
        fprintf(stderr, "[Warning] The ilm_control_context is already destroyed\n");
        return;
    }

    if (ctx->shutdown_fd > -1)
        send_shutdown_event(ctx);

    if (ctx->thread > 0) {
        if (0 != pthread_join(ctx->thread, NULL)) {
            fprintf(stderr, "failed to join control thread\n");
        }
    }

    destroy_control_resources();

    if (ctx->shutdown_fd > -1)
        close(ctx->shutdown_fd);

    memset(ctx, 0, sizeof *ctx);
}

ILM_EXPORT ilmErrorTypes
ilmControl_init(t_ilm_nativedisplay nativedisplay)
{
    struct ilm_control_context *ctx = &ilm_context;

    if (ctx->initialized)
    {
        fprintf(stderr, "Already initialized!\n");
        return ILM_FAILED;
    }

    if (nativedisplay == 0) {
        return ILM_ERROR_INVALID_ARGUMENTS;
    }

    ctx->shutdown_fd = -1;

    ctx->wl.display = (struct wl_display*)nativedisplay;

    wl_list_init(&ctx->wl.list_screen);
    wl_list_init(&ctx->wl.list_layer);
    wl_list_init(&ctx->wl.list_surface);
    wl_list_init(&ctx->wl.list_seat);

    {
       pthread_mutexattr_t a;
       if (pthread_mutexattr_init(&a) != 0)
       {
          return ILM_FAILED;
       }

       if (pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE) != 0)
       {
          pthread_mutexattr_destroy(&a);
          return ILM_FAILED;
       }

       if (pthread_mutex_init(&ctx->mutex, &a) != 0)
       {
           pthread_mutexattr_destroy(&a);
           fprintf(stderr, "failed to initialize pthread_mutex\n");
           return ILM_FAILED;
       }

       pthread_mutexattr_destroy(&a);
    }

    if (init_control() != 0)
    {
        ilmControl_destroy();
        return ILM_FAILED;
    }

    return ILM_SUCCESS;
}

static void*
control_thread(void *p_ret)
{
    struct ilm_control_context *const ctx = &ilm_context;
    struct wayland_context *const wl = &ctx->wl;
    struct wl_display *const display = wl->display;
    struct wl_event_queue *const queue = wl->queue;
    int const fd = wl_display_get_fd(display);
    int const shutdown_fd = ctx->shutdown_fd;
    (void) p_ret;

    while (1)
    {
        while (wl_display_prepare_read_queue(display, queue) != 0)
        {
            lock_context(ctx);
            wl_display_dispatch_queue_pending(display, queue);
            unlock_context(ctx);
        }

        if (wl_display_flush(display) == -1)
        {
            break;
        }

        struct pollfd pfd[2] = {
           { .fd = fd,          .events = POLLIN },
           { .fd = shutdown_fd, .events = POLLIN }
        };

        int pollret = poll(pfd, 2, -1);
        if (pollret != -1 && (pfd[0].revents & POLLIN))
        {
            wl_display_read_events(display);

            lock_context(ctx);
            int ret = wl_display_dispatch_queue_pending(display, queue);
            unlock_context(ctx);

            if (ret == -1)
            {
                break;
            }
        }
        else
        {
            wl_display_cancel_read(display);

            if (pollret == -1 || (pfd[1].revents & POLLIN))
            {
                break;
            }
        }
    }

    return NULL;
}

static int
init_control(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    struct wayland_context *wl = &ctx->wl;
    int ret = 0;

    wl->queue = wl_display_create_queue(wl->display);
    if (! wl->queue) {
        fprintf(stderr, "Could not create wayland event queue\n");
        return -1;
    }

    /* registry_add_listener for request by ivi-controller */
    wl->registry = wl_display_get_registry(wl->display);
    if (wl->registry == NULL) {
        wl_event_queue_destroy(wl->queue);
        wl->queue = NULL;
        fprintf(stderr, "Failed to get registry\n");
        return -1;
    }
    wl_proxy_set_queue((void*)wl->registry, wl->queue);

    if (wl_registry_add_listener(wl->registry,
                             &registry_control_listener, ctx)) {
        fprintf(stderr, "Failed to add registry listener\n");
        return -1;
    }

    if (
       // first level objects; ivi_controller
       wl_display_roundtrip_queue(wl->display, wl->queue) == -1 ||
       // second level object: ivi_controller_surfaces/layers
       wl_display_roundtrip_queue(wl->display, wl->queue) == -1 ||
       // third level objects: ivi_controller_surfaces/layers properties
       wl_display_roundtrip_queue(wl->display, wl->queue) == -1)
    {
        fprintf(stderr, "Failed to initialize wayland connection: %s\n", strerror(errno));
        return -1;
    }

    if (! wl->controller)
    {
        fprintf(stderr, "ivi_controller not available\n");
        return -1;
    }

    ctx->shutdown_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    if (ctx->shutdown_fd == -1)
    {
        fprintf(stderr, "Could not setup shutdown-fd: %s\n", strerror(errno));
        return ILM_FAILED;
    }

    ret = pthread_create(&ctx->thread, NULL, control_thread, NULL);

    if (ret != 0) {
        fprintf(stderr, "Failed to start internal receive thread. returned %d\n", ret);
        return -1;
    }

    ctx->initialized = true;

    return 0;
}

ilmErrorTypes impl_sync_and_acquire_instance(struct ilm_control_context *ctx)
{
    if (! ctx->initialized) {
        fprintf(stderr, "Not initialized\n");
        return ILM_FAILED;
    }

    lock_context(ctx);

    if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) == -1) {
        int err = wl_display_get_error(ctx->wl.display);
        fprintf(stderr, "Error communicating with wayland: %s\n", strerror(err));
        unlock_context(ctx);
        return ILM_FAILED;
    }

    return ILM_SUCCESS;
}

void release_instance(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    unlock_context(ctx);
}

static uint32_t
gen_layer_id(struct ilm_control_context *ctx)
{
    struct layer_context *ctx_layer = NULL;
    do {
        int found = 0;
        if (wl_list_length(&ctx->wl.list_layer) == 0) {
            ctx->internal_id_layer++;
            return ctx->internal_id_layer;
        }
        wl_list_for_each(ctx_layer, &ctx->wl.list_layer, link) {
            if (ctx_layer->id_layer == ctx->internal_id_layer) {
                found = 1;
                break;
            }

            if (found == 0) {
                return ctx->internal_id_layer;
            }
        }
        ctx->internal_id_layer++;
    } while(1);
}

static struct surface_context*
get_surface_context(struct wayland_context *ctx,
                          uint32_t id_surface)
{
    struct surface_context *ctx_surf = NULL;

    if (ctx->controller == NULL) {
        fprintf(stderr, "controller is not initialized in ilmControl\n");
        return NULL;
    }

    wl_list_for_each(ctx_surf, &ctx->list_surface, link) {
        if (ctx_surf->id_surface == id_surface) {
            return ctx_surf;
        }
    }

    return NULL;
}

static struct screen_context*
get_screen_context_by_id(struct wayland_context *ctx, uint32_t id_screen)
{
    struct screen_context *ctx_scrn = NULL;

    if (ctx->controller == NULL) {
        fprintf(stderr, "get_screen_context_by_id: controller is NULL\n");
        return NULL;
    }

    wl_list_for_each(ctx_scrn, &ctx->list_screen, link) {
        if (ctx_scrn->id_screen == id_screen) {
            return ctx_scrn;
        }
    }
    return NULL;
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfLayer(t_ilm_uint layerID,
                         struct ilmLayerProperties* pLayerProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    if (pLayerProperties != NULL) {

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerID);

        if (ctx_layer != NULL) {
            *pLayerProperties = ctx_layer->prop;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

static void
create_layerids(struct screen_context *ctx_screen,
                t_ilm_layer **layer_ids, t_ilm_uint *layer_count)
{
    struct layer_context *ctx_layer = NULL;
    t_ilm_layer *ids = NULL;

    *layer_count = wl_list_length(&ctx_screen->order.list_layer);
    if (*layer_count == 0) {
        *layer_ids = NULL;
        return;
    }

    *layer_ids = malloc(*layer_count * sizeof(t_ilm_layer));
    if (*layer_ids == NULL) {
        fprintf(stderr, "memory insufficient for layerids\n");
        *layer_count = 0;
        return;
    }

    ids = *layer_ids;
    wl_list_for_each_reverse(ctx_layer, &ctx_screen->order.list_layer, order.link) {
        *ids = (t_ilm_layer)ctx_layer->id_layer;
        ids++;
    }
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfScreen(t_ilm_display screenID,
                              struct ilmScreenProperties* pScreenProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;

    if (! pScreenProperties)
    {
        return ILM_ERROR_INVALID_ARGUMENTS;
    }

    struct ilm_control_context *ctx = sync_and_acquire_instance();

    struct screen_context *ctx_screen = NULL;
    ctx_screen = get_screen_context_by_id(&ctx->wl, (uint32_t)screenID);
    if (ctx_screen != NULL) {
        *pScreenProperties = ctx_screen->prop;
        create_layerids(ctx_screen, &pScreenProperties->layerIds,
                                    &pScreenProperties->layerCount);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getScreenIDs(t_ilm_uint* pNumberOfIDs, t_ilm_uint** ppIDs)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if ((pNumberOfIDs != NULL) && (ppIDs != NULL)) {
        struct screen_context *ctx_scrn = NULL;
        t_ilm_uint length = wl_list_length(&ctx->wl.list_screen);
        *pNumberOfIDs = 0;

        *ppIDs = (t_ilm_uint*)malloc(length * sizeof **ppIDs);
        if (*ppIDs != NULL) {
            t_ilm_uint* ids = *ppIDs;
            // compositor sends screens in opposite order
            // write ids from back to front to turn them around
            wl_list_for_each_reverse(ctx_scrn, &ctx->wl.list_screen, link) {
                *ids = ctx_scrn->id_screen;
                ids++;
            }
            *pNumberOfIDs = length;

            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getScreenResolution(t_ilm_uint screenID, t_ilm_uint* pWidth, t_ilm_uint* pHeight)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if ((pWidth != NULL) && (pHeight != NULL))
    {
        struct screen_context *ctx_scrn;
        wl_list_for_each(ctx_scrn, &ctx->wl.list_screen, link) {
            if (screenID == ctx_scrn->id_screen) {
                *pWidth = ctx_scrn->prop.screenWidth;
                *pHeight = ctx_scrn->prop.screenHeight;
                returnValue = ILM_SUCCESS;
                break;
            }
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getLayerIDs(t_ilm_int* pLength, t_ilm_layer** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct layer_context *ctx_layer = NULL;
        t_ilm_uint length = wl_list_length(&ctx->wl.list_layer);
        *pLength = 0;

        *ppArray = (t_ilm_layer*)malloc(length * sizeof **ppArray);
        if (*ppArray != NULL) {
            // compositor sends layers in opposite order
            // write ids from back to front to turn them around
            t_ilm_layer* ids = *ppArray;
            wl_list_for_each_reverse(ctx_layer, &ctx->wl.list_layer, link)
            {
                *ids = ctx_layer->id_layer;
                ++ids;
            }
            *pLength = length;

            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getLayerIDsOnScreen(t_ilm_uint screenId,
                            t_ilm_int* pLength,
                            t_ilm_layer** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct screen_context *ctx_screen = NULL;
        ctx_screen = get_screen_context_by_id(&ctx->wl, screenId);
        if (ctx_screen != NULL) {
            struct layer_context *ctx_layer = NULL;
            t_ilm_int length = wl_list_length(&ctx_screen->order.list_layer);

            if (0 < length)
            {
                *ppArray = (t_ilm_layer*)malloc(length * sizeof **ppArray);
                if (*ppArray != NULL) {
                    // compositor sends layers in opposite order
                    // write ids from back to front to turn them around
                    t_ilm_layer* ids = *ppArray;
                    wl_list_for_each_reverse(ctx_layer, &ctx_screen->order.list_layer, order.link)
                    {
                        *ids = ctx_layer->id_layer;
                        ++ids;
                    }

                }
            }
            else
            {
                *ppArray = NULL;
            }

            *pLength = length;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getSurfaceIDs(t_ilm_int* pLength, t_ilm_surface** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct surface_context *ctx_surf = NULL;
        t_ilm_uint length = wl_list_length(&ctx->wl.list_surface);
        *pLength = 0;

        *ppArray = (t_ilm_surface*)malloc(length * sizeof **ppArray);
        if (*ppArray != NULL) {
            t_ilm_surface* ids = *ppArray;
            wl_list_for_each_reverse(ctx_surf, &ctx->wl.list_surface, link) {
                *ids = ctx_surf->id_surface;
                ids++;
            }
            *pLength = length;

            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getSurfaceIDsOnLayer(t_ilm_layer layer,
                             t_ilm_int* pLength,
                             t_ilm_surface** ppArray)
{
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;
    t_ilm_uint length = 0;
    t_ilm_surface* ids = NULL;

    if ((pLength == NULL) || (ppArray == NULL)) {
        release_instance();
        return ILM_FAILED;
    }

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layer);

    if (ctx_layer == NULL) {
        release_instance();
        return ILM_FAILED;
    }

    length = wl_list_length(&ctx_layer->order.list_surface);
    *ppArray = (t_ilm_surface*)malloc(length * sizeof **ppArray);
    if (*ppArray == NULL) {
        release_instance();
        return ILM_FAILED;
    }

    ids = *ppArray;
    wl_list_for_each_reverse(ctx_surf, &ctx_layer->order.list_surface, order.link) {
        *ids = (t_ilm_surface)ctx_surf->id_surface;
        ids++;
    }
    *pLength = length;

    release_instance();
    return ILM_SUCCESS;
}

static int create_controller_layer(struct wayland_context *ctx, t_ilm_uint width, t_ilm_uint height, t_ilm_layer layerid)
{
     struct layer_context *ctx_layer = calloc(1, sizeof *ctx_layer);
     if (ctx_layer == NULL) {
         fprintf(stderr, "Failed to allocate memory for layer_context\n");
         return -1;
     }

     ctx_layer->controller = ivi_controller_layer_create(
                                 ctx->controller,
                                 layerid, width, height);
     if (ctx_layer->controller == NULL) {
         fprintf(stderr, "Failed to create layer\n");
         free(ctx_layer);
         return -1;
     }
     ctx_layer->id_layer = layerid;
     ctx_layer->ctx = ctx;

     wl_list_insert(&ctx->list_layer, &ctx_layer->link);
     wl_list_init(&ctx_layer->order.link);
     wl_list_init(&ctx_layer->order.list_surface);

     ivi_controller_layer_add_listener(ctx_layer->controller,
                                   &controller_layer_listener, ctx_layer);

     if (ctx->notification != NULL) {
        ilmObjectType layer = ILM_LAYER;
        ctx->notification(layer, ctx_layer->id_layer, ILM_TRUE,
                          ctx->notification_user_data);
     }

     return 0;
}

ILM_EXPORT ilmErrorTypes
ilm_layerCreateWithDimension(t_ilm_layer* pLayerId,
                                 t_ilm_uint width,
                                 t_ilm_uint height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    uint32_t layerid = 0;
    int32_t is_inside = 0;

    do {
        if (pLayerId == NULL) {
            break;
        }

        if (*pLayerId != INVALID_ID) {
            /* Return failed, if layerid is already inside list_layer */
            is_inside = wayland_controller_is_inside_layer_list(
                            &ctx->wl.list_layer, *pLayerId);
            if (0 != is_inside) {
                fprintf(stderr, "layerid=%d is already used.\n", *pLayerId);
                break;
            }
            layerid = *pLayerId;
        }
        else {
            /* Generate ID, if layerid is INVALID_ID */
            layerid = gen_layer_id(ctx);
            *pLayerId = layerid;
        }

        if (create_controller_layer(&ctx->wl, width, height, layerid) == 0)
        {
           returnValue = ILM_SUCCESS;
        }
    } while(0);

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemove(t_ilm_layer layerId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    struct layer_context *ctx_next = NULL;
    struct surface_context *ctx_surf = NULL;
    struct surface_context *ctx_surf_next = NULL;

    wl_list_for_each_safe(ctx_layer, ctx_next,
            &ctx->wl.list_layer, link) {
        if (ctx_layer->id_layer == layerId) {
            ivi_controller_layer_destroy(ctx_layer->controller, 1);

            wl_list_remove(&ctx_layer->order.link);
            wl_list_remove(&ctx_layer->link);

            wl_list_for_each_safe(ctx_surf, ctx_surf_next,
                                  &ctx_layer->order.list_surface, order.link) {
                remove_ordersurface_from_layer(ctx_surf);
            }

            free(ctx_layer);

            returnValue = ILM_SUCCESS;
            break;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetVisibility(t_ilm_layer layerId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        uint32_t visibility = 0;
        if (newVisibility == ILM_TRUE) {
            visibility = 1;
        }
        ivi_controller_layer_set_visibility(ctx_layer->controller,
                                            visibility);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetVisibility(t_ilm_layer layerId, t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pVisibility != NULL) {
        struct layer_context *ctx_layer = NULL;

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);

        if (ctx_layer != NULL) {
            *pVisibility = ctx_layer->prop.visibility;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetOpacity(t_ilm_layer layerId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        wl_fixed_t opacity_fixed = wl_fixed_from_double((double)opacity);
        ivi_controller_layer_set_opacity(ctx_layer->controller,
                                         opacity_fixed);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetOpacity(t_ilm_layer layerId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pOpacity != NULL) {
        struct layer_context *ctx_layer = NULL;

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);

        if (ctx_layer != NULL) {
            *pOpacity = ctx_layer->prop.opacity;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetSourceRectangle(t_ilm_layer layerId,
                                t_ilm_uint x, t_ilm_uint y,
                                t_ilm_uint width, t_ilm_uint height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        ivi_controller_layer_set_source_rectangle(ctx_layer->controller,
                                                  (uint32_t)x,
                                                  (uint32_t)y,
                                                  (uint32_t)width,
                                                  (uint32_t)height);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetDestinationRectangle(t_ilm_layer layerId,
                                 t_ilm_int x, t_ilm_int y,
                                 t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);
    if (ctx_layer != NULL) {
        ivi_controller_layer_set_destination_rectangle(
                                         ctx_layer->controller,
                                         (uint32_t)x, (uint32_t)y,
                                         (uint32_t)width,
                                         (uint32_t)height);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetOrientation(t_ilm_layer layerId, ilmOrientation orientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    int32_t iviorientation = 0;

    do {
        switch(orientation) {
        case ILM_ZERO:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES;
            break;
        case ILM_NINETY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES;
            break;
        case ILM_ONEHUNDREDEIGHTY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES;
            break;
        case ILM_TWOHUNDREDSEVENTY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES;
            break;
        default:
            returnValue = ILM_ERROR_INVALID_ARGUMENTS;
            break;
        }

        ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);
        if (ctx_layer == NULL) {
            returnValue = ILM_FAILED;
            break;
        }

        ivi_controller_layer_set_orientation(ctx_layer->controller,
                                             iviorientation);

        returnValue = ILM_SUCCESS;
    } while(0);

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetOrientation(t_ilm_layer layerId, ilmOrientation *pOrientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    if (pOrientation != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            *pOrientation = ctx_layer->prop.orientation;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetRenderOrder(t_ilm_layer layerId,
                        t_ilm_surface *pSurfaceId,
                        t_ilm_int number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);

    if (ctx_layer)
    {
        struct wl_array ids;
        wl_array_init(&ids);
        uint32_t *pids = wl_array_add(&ids, number * sizeof *pids);
        t_ilm_uint i;
        for (i = 0; i < number; i++) pids[i] = (uint32_t)pSurfaceId[i];
        ivi_controller_layer_set_render_order(ctx_layer->controller, &ids);
        wl_array_release(&ids);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetVisibility(t_ilm_surface surfaceId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;
    uint32_t visibility = 0;

    if (newVisibility == ILM_TRUE) {
        visibility = 1;
    }
    ctx_surf = get_surface_context(&ctx->wl, surfaceId);
    if (ctx_surf) {
        if (ctx_surf->controller != NULL) {
            ivi_controller_surface_set_visibility(ctx_surf->controller,
                                                  visibility);
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetOpacity(t_ilm_surface surfaceId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;
    wl_fixed_t opacity_fixed = 0;

    opacity_fixed = wl_fixed_from_double((double)opacity);
    ctx_surf = get_surface_context(&ctx->wl, surfaceId);
    if (ctx_surf) {
        if (ctx_surf->controller != NULL) {
            ivi_controller_surface_set_opacity(ctx_surf->controller,
                                               opacity_fixed);
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetOpacity(t_ilm_surface surfaceId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pOpacity != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->wl, surfaceId);
        if (ctx_surf) {
            *pOpacity = ctx_surf->prop.opacity;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetDestinationRectangle(t_ilm_surface surfaceId,
                                   t_ilm_int x, t_ilm_int y,
                                   t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->wl, surfaceId);
    if (ctx_surf) {
        if (ctx_surf->controller != NULL) {
            ivi_controller_surface_set_destination_rectangle(
                                                 ctx_surf->controller,
                                                 x, y, width, height);
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetOrientation(t_ilm_surface surfaceId,
                              ilmOrientation orientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;
    int32_t iviorientation = 0;

    do {
        switch(orientation) {
        case ILM_ZERO:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES;
            break;
        case ILM_NINETY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES;
            break;
        case ILM_ONEHUNDREDEIGHTY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES;
            break;
        case ILM_TWOHUNDREDSEVENTY:
            iviorientation = IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES;
            break;
        default:
            returnValue = ILM_ERROR_INVALID_ARGUMENTS;
            break;
        }

        ctx_surf = get_surface_context(&ctx->wl, surfaceId);
        if (ctx_surf == NULL) {
            returnValue = ILM_FAILED;
            break;
        }

        ivi_controller_surface_set_orientation(ctx_surf->controller,
                                               iviorientation);

        returnValue = ILM_SUCCESS;
    } while(0);

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetOrientation(t_ilm_surface surfaceId,
                              ilmOrientation *pOrientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pOrientation != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->wl, surfaceId);
        if (ctx_surf) {
            *pOrientation = ctx_surf->prop.orientation;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetPixelformat(t_ilm_layer surfaceId,
                              ilmPixelFormat *pPixelformat)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pPixelformat != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->wl, surfaceId);
        if (ctx_surf) {
            *pPixelformat = ctx_surf->prop.pixelformat;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_displaySetRenderOrder(t_ilm_display display,
                          t_ilm_layer *pLayerId, const t_ilm_uint number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct screen_context *ctx_scrn = NULL;

    ctx_scrn = get_screen_context_by_id(&ctx->wl, (uint32_t)display);
    if (ctx_scrn != NULL) {
        struct wl_array ids;
        wl_array_init(&ids);
        uint32_t *pids = wl_array_add(&ids, number * sizeof *pids);
        t_ilm_uint i;
        for (i = 0; i < number; i++) pids[i] = (uint32_t)pLayerId[i];
        ivi_controller_screen_set_render_order(ctx_scrn->controller, &ids);
        wl_array_release(&ids);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_takeScreenshot(t_ilm_uint screen, t_ilm_const_string filename)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct screen_context *ctx_scrn = NULL;

    ctx_scrn = get_screen_context_by_id(&ctx->wl, (uint32_t)screen);
    if (ctx_scrn != NULL) {
        ivi_controller_screen_screenshot(ctx_scrn->controller,
                                        filename);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_takeLayerScreenshot(t_ilm_const_string filename, t_ilm_layer layerid)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerid);
    if (ctx_layer != NULL) {
        ivi_controller_layer_screenshot(ctx_layer->controller,
                                        filename);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_takeSurfaceScreenshot(t_ilm_const_string filename,
                              t_ilm_surface surfaceid)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceid);
    if (ctx_surf) {
        ivi_controller_surface_screenshot(ctx_surf->controller,
                                          filename);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerAddNotification(t_ilm_layer layer,
                             layerNotificationFunc callback)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layer);
    if (ctx_layer == NULL) {
        returnValue = ILM_ERROR_INVALID_ARGUMENTS;
    } else {
        ctx_layer->notification = callback;

        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveNotification(t_ilm_layer layer)
{
   return ilm_layerAddNotification(layer, NULL);
}

static struct surface_context *
create_surface_context(struct wayland_context *ctx, uint32_t id_surface)
{
    struct surface_context *ctx_surf = NULL;

    ctx_surf = calloc(1, sizeof *ctx_surf);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface_context\n");
        return NULL;
    }

    ctx_surf->id_surface = id_surface;
    ctx_surf->ctx = ctx;
    ctx_surf->is_surface_creation_noticed = false;

    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
    wl_list_init(&ctx_surf->order.link);
    wl_list_init(&ctx_surf->list_accepted_seats);

    return ctx_surf;
}

ILM_EXPORT ilmErrorTypes
ilm_registerNotification(notificationFunc callback, void *user_data)
{
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;

    ctx->wl.notification = callback;
    ctx->wl.notification_user_data = user_data;
    if (callback != NULL) {
        wl_list_for_each(ctx_layer, &ctx->wl.list_layer, link) {
            if (ctx_layer->controller) {
                 callback(ILM_LAYER, ctx_layer->id_layer, ILM_TRUE, user_data);
            }
        }

        wl_list_for_each(ctx_surf, &ctx->wl.list_surface, link) {
            if (ctx_surf->controller) {
                 callback(ILM_SURFACE, ctx_surf->id_surface, ILM_TRUE, user_data);
            }
        }
    }
    release_instance();
    return ILM_SUCCESS;
}

ILM_EXPORT ilmErrorTypes
ilm_unregisterNotification(void)
{
   return ilm_registerNotification(NULL, NULL);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceAddNotification(t_ilm_surface surface,
                             surfaceNotificationFunc callback)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = (struct surface_context*)get_surface_context(
                    &ctx->wl, (uint32_t)surface);
    if (ctx_surf == NULL) {
        if (callback != NULL) {
            callback((uint32_t)surface, NULL, ILM_NOTIFICATION_CONTENT_REMOVED);
            ctx_surf = create_surface_context(&ctx->wl, (uint32_t)surface);
        }
    }
    else {
        if (callback != NULL) {
            callback(ctx_surf->id_surface,
                     &ctx_surf->prop,
                     (ctx_surf->controller) ? ILM_NOTIFICATION_CONTENT_AVAILABLE
                                            : ILM_NOTIFICATION_CONTENT_REMOVED);
        }
    }

    if (ctx_surf == NULL) {
        returnValue = ILM_ERROR_INVALID_ARGUMENTS;
    }
    else {
        ctx_surf->notification = callback;
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceRemoveNotification(t_ilm_surface surface)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = (struct surface_context*)get_surface_context(
                    &ctx->wl, (uint32_t)surface);
    if (ctx_surf != NULL) {
        if (ctx_surf->notification != NULL) {
            ctx_surf->notification = NULL;
            returnValue = ILM_SUCCESS;
        } else {
            returnValue = ILM_ERROR_INVALID_ARGUMENTS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfSurface(t_ilm_uint surfaceID,
                        struct ilmSurfaceProperties* pSurfaceProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (pSurfaceProperties != NULL) {
        struct surface_context *ctx_surf = NULL;

        ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceID);
        if (ctx_surf != NULL) {
            // request statistics for surface
            ivi_controller_surface_send_stats(ctx_surf->controller);
            // force submission
            int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

            // If we got an error here, there is really no sense
            // in returning the properties as something is fundamentally
            // broken.
            if (ret != -1)
            {
               *pSurfaceProperties = ctx_surf->prop;
               returnValue = ILM_SUCCESS;
            }
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerAddSurface(t_ilm_layer layerId,
                        t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);
    ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);
    if ((ctx_layer != NULL) && (ctx_surf != NULL)) {
        ivi_controller_layer_add_surface(ctx_layer->controller,
                                         ctx_surf->controller);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveSurface(t_ilm_layer layerId,
                           t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layerId);
    ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);
    if ((ctx_layer != NULL) && (ctx_surf != NULL)) {
        ivi_controller_layer_remove_surface(ctx_layer->controller,
                                            ctx_surf->controller);
        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetVisibility(t_ilm_surface surfaceId,
                             t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    if (pVisibility != NULL) {
        ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);
        if (ctx_surf != NULL) {
            *pVisibility = (t_ilm_bool)ctx_surf->prop.visibility;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetSourceRectangle(t_ilm_surface surfaceId,
                                  t_ilm_int x, t_ilm_int y,
                                  t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);
    if (ctx_surf != NULL) {
        if (ctx_surf->controller != NULL) {
            ivi_controller_surface_set_source_rectangle(
                    ctx_surf->controller,
                    x, y, width, height);
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_commitChanges(void)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (ctx->wl.controller != NULL) {
        ivi_controller_commit_changes(ctx->wl.controller);

        if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) != -1)
        {
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();

    return returnValue;
}
