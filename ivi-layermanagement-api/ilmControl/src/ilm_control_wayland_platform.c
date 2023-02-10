/**************************************************************************
 *
 * Copyright (C) 2013 DENSO CORPORATION
 * Copyright (C) 2017 Advanced Driver Information Technology Joint Venture GmbH
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

#include <sys/mman.h>
#include <sys/eventfd.h>

#include "writepng.h"
#include "bitmap.h"
#include "ilm_common.h"
#include "ilm_control_platform.h"
#include "wayland-util.h"
#include "ivi-wm-client-protocol.h"
#include "ivi-input-client-protocol.h"

struct layer_context {
    struct wl_list link;

    t_ilm_uint id_layer;

    struct ilmLayerProperties prop;
    layerNotificationFunc notification;

    struct wl_array render_order;

    struct wayland_context *ctx;
};

struct screen_context {
    struct wl_list link;

    struct wl_output *output;
    struct ivi_wm_screen *controller;
    t_ilm_uint id_screen;
    t_ilm_uint name;
    int32_t transform;

    struct ilmScreenProperties prop;

    struct wl_array render_order;

    struct wayland_context *ctx;
};

struct screenshot_context {
    const char *filename;
    ilmErrorTypes result;
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

static void
wm_listener_layer_visibility(void *data, struct ivi_wm *controller,
                             uint32_t layer_id, int32_t visibility)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    if (ctx_layer->prop.visibility == (t_ilm_bool)visibility)
        return;

    ctx_layer->prop.visibility = (t_ilm_bool)visibility;

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_VISIBILITY);
    }
}

static void
wm_listener_layer_opacity(void *data, struct ivi_wm *controller,
                          uint32_t layer_id, wl_fixed_t opacity)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    if (ctx_layer->prop.opacity == (t_ilm_float)wl_fixed_to_double(opacity))
        return;

    ctx_layer->prop.opacity = (t_ilm_float)wl_fixed_to_double(opacity);

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_OPACITY);
    }
}

static void
wm_listener_layer_source_rectangle(void *data, struct ivi_wm *controller,
                                   uint32_t layer_id, int32_t x, int32_t y,
                                   int32_t width, int32_t height)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    if ((ctx_layer->prop.sourceX == (t_ilm_uint)x) &&
        (ctx_layer->prop.sourceY == (t_ilm_uint)y) &&
        (ctx_layer->prop.sourceWidth == (t_ilm_uint)width &&
        (ctx_layer->prop.sourceHeight == (t_ilm_uint)height)))
        return;

    ctx_layer->prop.sourceX = (t_ilm_uint)x;
    ctx_layer->prop.sourceY = (t_ilm_uint)y;
    ctx_layer->prop.sourceWidth = (t_ilm_uint)width;
    ctx_layer->prop.sourceHeight = (t_ilm_uint)height;

    if (ctx_layer->notification != NULL) {
        ctx_layer->notification(ctx_layer->id_layer,
                                &ctx_layer->prop,
                                ILM_NOTIFICATION_SOURCE_RECT);
    }
}

static void
wm_listener_layer_destination_rectangle(void *data, struct ivi_wm *controller,
                                        uint32_t layer_id, int32_t x, int32_t y,
                                        int32_t width, int32_t height)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    if ((ctx_layer->prop.destX == (t_ilm_uint)x) &&
        (ctx_layer->prop.destY == (t_ilm_uint)y) &&
        (ctx_layer->prop.destWidth == (t_ilm_uint)width &&
        (ctx_layer->prop.destHeight == (t_ilm_uint)height)))
        return;

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
wm_listener_layer_created(void *data, struct ivi_wm *controller, uint32_t layer_id)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(ctx_layer)
        return;

    ctx_layer = calloc(1, sizeof *ctx_layer);
    if (!ctx_layer) {
        fprintf(stderr, "Failed to allocate memory for layer_context\n");
        return;
    }

    ctx_layer->id_layer = layer_id;
    ctx_layer->ctx = ctx;

    wl_list_insert(&ctx->list_layer, &ctx_layer->link);

    if (ctx->notification != NULL) {
       ilmObjectType layer = ILM_LAYER;
       ctx->notification(layer, ctx_layer->id_layer, ILM_TRUE,
                         ctx->notification_user_data);
    }
}

static void
wm_listener_layer_destroyed(void *data, struct ivi_wm *controller, uint32_t layer_id)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    wl_list_remove(&ctx_layer->link);

    if (ctx_layer->ctx->notification != NULL) {
        ilmObjectType layer = ILM_LAYER;
        ctx_layer->ctx->notification(layer, ctx_layer->id_layer, ILM_FALSE,
                                     ctx_layer->ctx->notification_user_data);
    }

    free(ctx_layer);
}

static void
wm_listener_layer_surface_added(void *data, struct ivi_wm *controller,
                                uint32_t layer_id, uint32_t surface_id)
{
    struct wayland_context *ctx = data;
    struct layer_context *ctx_layer;
    (void)controller;

    ctx_layer = wayland_controller_get_layer_context(ctx, layer_id);
    if(!ctx_layer)
        return;

    uint32_t *add_id = wl_array_add(&ctx_layer->render_order, sizeof(*add_id));
    *add_id = surface_id;
}

static void
wm_listener_layer_error(void *data, struct ivi_wm *controller, uint32_t object_id,
                        uint32_t code, const char *message)
{
    struct wayland_context *ctx = data;
    ilmErrorTypes error_code;

    switch (code) {
    case IVI_WM_LAYER_ERROR_NO_SURFACE:
        error_code = ILM_ERROR_RESOURCE_NOT_FOUND;
        fprintf(stderr, "The surface with id: %d does not exist\n", object_id);
        break;
    case IVI_WM_LAYER_ERROR_NO_LAYER:
        error_code = ILM_ERROR_RESOURCE_NOT_FOUND;
        fprintf(stderr, "The layer with id: %d does not exist\n", object_id);
        break;
    case IVI_WM_LAYER_ERROR_BAD_PARAM:
        error_code = ILM_ERROR_INVALID_ARGUMENTS;
        fprintf(stderr, "The layer with id: %d is used with invalid parameter\n",
                object_id);
        break;
    default:
        error_code = ILM_ERROR_ON_CONNECTION;
    }

    fprintf(stderr, "%s", message);
    fprintf(stderr, "\n");

    /*Do not override old error message*/
    if (ctx->error_flag == ILM_SUCCESS)
        ctx->error_flag = error_code;
}

static void
wm_listener_surface_visibility(void *data, struct ivi_wm *controller,
                               uint32_t surface_id, int32_t visibility)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    if (ctx_surf->prop.visibility == (t_ilm_bool)visibility)
        return;

    ctx_surf->prop.visibility = (t_ilm_bool)visibility;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_VISIBILITY);
    }
}

static void
wm_listener_surface_opacity(void *data, struct ivi_wm *controller,
                            uint32_t surface_id, wl_fixed_t opacity)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    if (ctx_surf->prop.opacity == (t_ilm_float)wl_fixed_to_double(opacity))
        return;

    ctx_surf->prop.opacity = (t_ilm_float)wl_fixed_to_double(opacity);

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_OPACITY);
    }
}

static void
wm_listener_surface_size(void *data, struct ivi_wm *controller,
                         uint32_t surface_id, int32_t width, int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    if ((ctx_surf->prop.origSourceWidth == (t_ilm_uint)width) &&
        (ctx_surf->prop.origSourceHeight == (t_ilm_uint)height))
        return;

    ctx_surf->prop.origSourceWidth = (t_ilm_uint)width;
    ctx_surf->prop.origSourceHeight = (t_ilm_uint)height;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_CONFIGURED);
    }
}

static void
wm_listener_surface_source_rectangle(void *data, struct ivi_wm *controller,
                                     uint32_t surface_id, int32_t x, int32_t y,
                                     int32_t width, int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    if ((ctx_surf->prop.sourceX == (t_ilm_uint)x) &&
        (ctx_surf->prop.sourceY == (t_ilm_uint)y) &&
        (ctx_surf->prop.sourceWidth == (t_ilm_uint)width &&
        (ctx_surf->prop.sourceHeight == (t_ilm_uint)height)))
        return;

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
wm_listener_surface_destination_rectangle(void *data, struct ivi_wm *controller,
                                          uint32_t surface_id, int32_t x,
                                          int32_t y, int32_t width, int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    if ((ctx_surf->prop.destX == (t_ilm_uint)x) &&
        (ctx_surf->prop.destY == (t_ilm_uint)y) &&
        (ctx_surf->prop.destWidth == (t_ilm_uint)width &&
        (ctx_surf->prop.destHeight == (t_ilm_uint)height)))
        return;

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
wm_listener_surface_stats(void *data, struct ivi_wm *controller,
                          uint32_t surface_id, uint32_t frame_count,
                          uint32_t pid)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

    ctx_surf->prop.frameCounter = (t_ilm_uint)frame_count;
    ctx_surf->prop.creatorPid = (t_ilm_uint)pid;
}

static void
wm_listener_surface_created(void *data, struct ivi_wm *controller,
                            uint32_t surface_id)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(ctx_surf)
        return;

    ctx_surf = calloc(1, sizeof *ctx_surf);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface_context\n");
        return;
    }

    ctx_surf->id_surface = surface_id;
    ctx_surf->ctx = ctx;

    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
    wl_list_init(&ctx_surf->list_accepted_seats);

    if (ctx->notification != NULL) {
        ilmObjectType surface = ILM_SURFACE;
        ctx->notification(surface, ctx_surf->id_surface, ILM_TRUE,
                          ctx->notification_user_data);
    }
}

static void
wm_listener_surface_destroyed(void *data, struct ivi_wm *controller,
                              uint32_t surface_id)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf;
    struct accepted_seat *seat, *seat_next;

    ctx_surf = get_surface_context(ctx, surface_id);
    if(!ctx_surf)
        return;

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

    wl_list_for_each_safe(seat, seat_next, &ctx_surf->list_accepted_seats, link) {
        wl_list_remove(&seat->link);
        free(seat->seat_name);
        free(seat);
    }

    wl_list_remove(&ctx_surf->link);
    free(ctx_surf);
}

static void
wm_listener_surface_error(void *data, struct ivi_wm *controller,
                          uint32_t object_id, uint32_t code, const char *message)
{
    struct wayland_context *ctx = data;
    ilmErrorTypes error_code;

    switch (code) {
    case IVI_WM_SURFACE_ERROR_NO_SURFACE:
        error_code = ILM_ERROR_RESOURCE_NOT_FOUND;
        fprintf(stderr, "The surface with id: %d does not exist\n", object_id);
        break;
    case IVI_WM_SURFACE_ERROR_NOT_SUPPORTED:
        error_code = ILM_ERROR_NOT_IMPLEMENTED;
        fprintf(stderr, "The surface with id: %d is used for unsupported operation\n", object_id);
        break;
    case IVI_WM_SURFACE_ERROR_BAD_PARAM:
        error_code = ILM_ERROR_INVALID_ARGUMENTS;
        fprintf(stderr, "The surface with id: %d is used with invalid parameter\n",
                object_id);
        break;
    default:
        error_code = ILM_ERROR_ON_CONNECTION;
        break;
    }

    fprintf(stderr, "%s", message);
    fprintf(stderr, "\n");

    if (ctx->error_flag == ILM_SUCCESS)
        ctx->error_flag = error_code;
}

static struct ivi_wm_listener wm_listener=
{
    wm_listener_surface_visibility,
    wm_listener_layer_visibility,
    wm_listener_surface_opacity,
    wm_listener_layer_opacity,
    wm_listener_surface_source_rectangle,
    wm_listener_layer_source_rectangle,
    wm_listener_surface_destination_rectangle,
    wm_listener_layer_destination_rectangle,
    wm_listener_surface_created,
    wm_listener_layer_created,
    wm_listener_surface_destroyed,
    wm_listener_layer_destroyed,
    wm_listener_surface_error,
    wm_listener_layer_error,
    wm_listener_surface_size,
    wm_listener_surface_stats,
    wm_listener_layer_surface_added,
};

static void
wm_screen_listener_screen_id(void *data, struct ivi_wm_screen *controller,
                             uint32_t screen_id)
{
    struct screen_context *ctx_screen = data;

    ctx_screen->id_screen = screen_id;
}

static void
wm_screen_listener_layer_added(void *data, struct ivi_wm_screen *controller,
                               uint32_t layer_id)
{
    struct screen_context *ctx_screen = data;
    (void) controller;

    uint32_t *add_id = wl_array_add(&ctx_screen->render_order, sizeof(*add_id));
    *add_id = layer_id;
}

static void
wm_screen_listener_connector_name(void *data, struct ivi_wm_screen *controller,
                                  const char *connector_name)
{
    struct screen_context *ctx_screen = data;
    (void) controller;

    strcpy(ctx_screen->prop.connectorName, connector_name);
}

static void
wm_screen_listener_error(void *data, struct ivi_wm_screen *controller,
                         uint32_t code, const char *message)
{
    struct screen_context *ctx_screen = data;
    ilmErrorTypes error_code;

    switch (code) {
    case IVI_WM_SCREEN_ERROR_NO_LAYER:
        error_code = ILM_ERROR_RESOURCE_NOT_FOUND;
        fprintf(stderr, "A non-existing layer is used with the screen: %d\n",
                ctx_screen->id_screen);
        break;
    case IVI_WM_SCREEN_ERROR_NO_SCREEN:
        error_code = ILM_ERROR_RESOURCE_NOT_FOUND;
        fprintf(stderr, "The screen with id: %d does not exist\n",
                ctx_screen->id_screen);
        break;
    case IVI_WM_SCREEN_ERROR_BAD_PARAM:
        error_code = ILM_ERROR_INVALID_ARGUMENTS;
        fprintf(stderr, "The screen with id: %d is used with invalid parameter\n",
                         ctx_screen->id_screen);
        break;
    default:
        error_code = ILM_ERROR_ON_CONNECTION;
        break;
    }

    fprintf(stderr, "%s", message);
    fprintf(stderr, "\n");

    if (ctx_screen->ctx->error_flag == ILM_SUCCESS)
        ctx_screen->ctx->error_flag = error_code;
}

static struct ivi_wm_screen_listener wm_screen_listener=
{
    wm_screen_listener_screen_id,
    wm_screen_listener_layer_added,
    wm_screen_listener_connector_name,
    wm_screen_listener_error
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
    int surface_found = 0;
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

    accepted_seat = calloc(1, sizeof(*accepted_seat));
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

    if (strcmp(interface, "ivi_wm") == 0) {
        ctx->controller = wl_registry_bind(registry, name,
                                           &ivi_wm_interface, 1);
        if (ctx->controller == NULL) {
            fprintf(stderr, "Failed to registry bind ivi_wm\n");
            return;
        }

        ivi_wm_add_listener(ctx->controller, &wm_listener, ctx);

    } else if (strcmp(interface, "ivi_input") == 0) {
        ctx->input_controller =
            wl_registry_bind(registry, name, &ivi_input_interface, 1);

        if (ctx->input_controller == NULL) {
            fprintf(stderr, "Failed to registry bind input controller\n");
            return;
        }
        ivi_input_add_listener(ctx->input_controller, &input_listener, ctx);

    } else if (strcmp(interface, "wl_output") == 0) {
        struct screen_context *ctx_scrn = calloc(1, sizeof *ctx_scrn);

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

        if (ctx->controller) {
            ctx_scrn->controller = ivi_wm_create_screen(ctx->controller, ctx_scrn->output);
            ivi_wm_screen_add_listener(ctx_scrn->controller, &wm_screen_listener,
                                       ctx_scrn);
        }

        ctx_scrn->ctx = ctx;
        ctx_scrn->name = name;
        wl_list_insert(&ctx->list_screen, &ctx_scrn->link);
    }
}

static void
registry_handle_control_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    struct wayland_context *ctx = data;
    struct screen_context *ctx_scrn, *next;

    /*remove wl_output and corresponding screen context*/
    wl_list_for_each_safe(ctx_scrn, next, &ctx->list_screen, link) {
        if(ctx_scrn->name == name)
        { fprintf(stderr,"output_removed \n");
            if (ctx_scrn->controller != NULL) {
                ivi_wm_screen_destroy(ctx_scrn->controller);
            }

            if (ctx_scrn->output != NULL) {
                wl_output_destroy(ctx_scrn->output);
            }

            wl_list_remove(&ctx_scrn->link);
            wl_array_release(&ctx_scrn->render_order);
            free(ctx_scrn);
        }
    }
}

static const struct wl_registry_listener
registry_control_listener= {
    registry_handle_control,
    registry_handle_control_remove
};

struct ilm_control_context ilm_context;

static void destroy_control_resources(void)
{
    struct ilm_control_context *ctx = &ilm_context;

    // free resources of output objects
    if (ctx->wl.controller) {
        struct screen_context *ctx_scrn;
        struct screen_context *next;

        wl_list_for_each_safe(ctx_scrn, next, &ctx->wl.list_screen, link) {
            if (ctx_scrn->controller != NULL) {
                ivi_wm_screen_destroy(ctx_scrn->controller);
            }

            if (ctx_scrn->output != NULL) {
                wl_output_destroy(ctx_scrn->output);
            }

            wl_list_remove(&ctx_scrn->link);
            wl_array_release(&ctx_scrn->render_order);
            free(ctx_scrn);
        }
    }

    if (ctx->wl.controller != NULL) {
        {
            struct surface_context *l;
            struct surface_context *n;
            struct accepted_seat *seat, *seat_next;
            wl_list_for_each_safe(l, n, &ctx->wl.list_surface, link) {
                wl_list_for_each_safe(seat, seat_next, &l->list_accepted_seats, link) {
                    wl_list_remove(&seat->link);
                    free(seat->seat_name);
                    free(seat);
                }

                wl_list_remove(&l->link);
                free(l);
            }
        }

        {
            struct layer_context *l;
            struct layer_context *n;
            wl_list_for_each_safe(l, n, &ctx->wl.list_layer, link) {
                wl_list_remove(&l->link);
                wl_array_release(&l->render_order);
                free(l);
            }
        }

        ivi_wm_destroy(ctx->wl.controller);
        ctx->wl.controller = NULL;
    }

    {
        struct seat_context *s, *n;
        wl_list_for_each_safe(s, n, &ctx->wl.list_seat, link) {
            wl_list_remove(&s->link);
            free(s->seat_name);
            free(s);
        }
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

    if (ctx->wl.input_controller) {
        ivi_input_destroy(ctx->wl.input_controller);
        ctx->wl.input_controller = NULL;
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

ILM_EXPORT ilmErrorTypes
ilmControl_registerShutdownNotification(shutdownNotificationFunc callback, void *user_data)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();

    if (!callback)
    {
        fprintf(stderr, "[Error] shutdownNotificationFunc is invalid\n");
        goto error;
    }

    ctx->notification = callback;
    ctx->notification_user_data = user_data;

    returnValue = ILM_SUCCESS;

error:
    release_instance();
    return returnValue;
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
    ctx->notification = NULL;
    ctx->notification_user_data = NULL;

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

static void
handle_shutdown(struct ilm_control_context *ctx,
                t_ilm_shutdown_error_type error_type)
{
    struct wayland_context *wl_ctx = &ctx->wl;
    struct wl_display *display = wl_ctx->display;
    int errornum;

    switch (error_type)
    {
        case ILM_ERROR_WAYLAND:
            errornum = wl_display_get_error(display);
            break;
        case ILM_ERROR_POLL:
        default:
            errornum = errno;
    }

    fprintf(stderr, "[Error] ilm services shutdown due to error %s\n",
            strerror(errornum));

    if (!ctx->notification)
        return;

    ctx->notification(error_type, errornum, ctx->notification_user_data);
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
            handle_shutdown(ctx, ILM_ERROR_WAYLAND);
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
                handle_shutdown(ctx, ILM_ERROR_WAYLAND);
                break;
            }
        }
        else
        {
            if (pollret == -1)
                handle_shutdown(ctx, ILM_ERROR_POLL);

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
    struct screen_context *ctx_scrn;
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

    // get globals
    if (wl_display_roundtrip_queue(wl->display, wl->queue) == -1)
    {
        fprintf(stderr, "Failed to initialize wayland connection: %s\n", strerror(errno));
        return -1;
    }

    if (! wl->controller)
    {
        fprintf(stderr, "ivi_wm not available\n");
        return -1;
    }

    wl_list_for_each(ctx_scrn, &ctx->wl.list_screen, link) {
        if (!ctx_scrn->controller) {
            ctx_scrn->controller = ivi_wm_create_screen(wl->controller, ctx_scrn->output);
            ivi_wm_screen_add_listener(ctx_scrn->controller, &wm_screen_listener,
                                       ctx_scrn);
        }
    }

    // get screen-ids
    if (wl_display_roundtrip_queue(wl->display, wl->queue) == -1)
    {
        fprintf(stderr, "Failed to do roundtrip queue: %s\n", strerror(errno));
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
    struct ilm_control_context *const ctx = &ilm_context;
    struct layer_context *ctx_layer = NULL;
    int32_t mask;

    mask = IVI_WM_PARAM_OPACITY | IVI_WM_PARAM_VISIBILITY | IVI_WM_PARAM_SIZE;

    if (pLayerProperties != NULL) {
        lock_context(ctx);

        ivi_wm_layer_get(ctx->wl.controller, layerID, mask);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerID);

        if ((ret != -1) && (ctx_layer != NULL))
        {
            *pLayerProperties = ctx_layer->prop;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

static void
create_layerids(struct screen_context *ctx_screen,
                t_ilm_layer **layer_ids, t_ilm_uint *layer_count)
{
    t_ilm_layer *ids = NULL;
    uint32_t *id = NULL;

    if (ctx_screen->render_order.size == 0) {
        *layer_ids = NULL;
        *layer_count = 0;
        return;
    }

    *layer_ids = malloc(ctx_screen->render_order.size);
    if (*layer_ids == NULL) {
        fprintf(stderr, "memory insufficient for layerids\n");
        *layer_count = 0;
        wl_array_release(&ctx_screen->render_order);
        wl_array_init(&ctx_screen->render_order);
        return;
    }

    ids = *layer_ids;
    wl_array_for_each(id, &ctx_screen->render_order) {
        *ids = (t_ilm_layer) *id;
        ids++;
        (*layer_count)++;
    }

    wl_array_release(&ctx_screen->render_order);
    wl_array_init(&ctx_screen->render_order);
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfScreen(t_ilm_display screenID,
                              struct ilmScreenProperties* pScreenProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    if (! pScreenProperties)
    {
        return ILM_ERROR_INVALID_ARGUMENTS;
    }

    lock_context(ctx);
    struct screen_context *ctx_screen = NULL;
    ctx_screen = get_screen_context_by_id(&ctx->wl, (uint32_t)screenID);
    if (ctx_screen != NULL) {
        ivi_wm_screen_get(ctx_screen->controller, IVI_WM_PARAM_RENDER_ORDER);

        if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) != -1 ) {
            *pScreenProperties = ctx_screen->prop;
            create_layerids(ctx_screen, &pScreenProperties->layerIds,
                                        &pScreenProperties->layerCount);
            returnValue = ILM_SUCCESS;
        }
    }

    unlock_context(ctx);
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
    struct ilm_control_context *const ctx = &ilm_context;

    if ((pLength != NULL) && (ppArray != NULL)) {
        lock_context(ctx);
        struct screen_context *ctx_screen = NULL;
        ctx_screen = get_screen_context_by_id(&ctx->wl, screenId);
        if (ctx_screen != NULL) {
            *pLength = 0;
            *ppArray = NULL;

            ivi_wm_screen_get(ctx_screen->controller, IVI_WM_PARAM_RENDER_ORDER);

            if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) != -1 ) {
                create_layerids(ctx_screen, ppArray, (t_ilm_uint*)pLength);
                returnValue = ILM_SUCCESS;
            }
        }
    }

    unlock_context(ctx);
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
    struct ilm_control_context *const ctx = &ilm_context;
    struct layer_context *ctx_layer = NULL;
    t_ilm_uint length = 0;
    t_ilm_surface* ids = NULL;
    uint32_t *id = NULL;

    if ((pLength == NULL) || (ppArray == NULL)) {
        release_instance();
        return ILM_FAILED;
    }

    lock_context(ctx);

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layer);

    if (ctx_layer == NULL) {
        unlock_context(ctx);
        return ILM_FAILED;
    }

    ivi_wm_layer_get(ctx->wl.controller, layer, IVI_WM_PARAM_RENDER_ORDER);
    int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

    if (ret < 0) {
        wl_array_release(&ctx_layer->render_order);
        wl_array_init(&ctx_layer->render_order);
        unlock_context(ctx);
        return ILM_FAILED;
    }

    *ppArray = (t_ilm_surface*)malloc(ctx_layer->render_order.size);
    if (*ppArray == NULL) {
        wl_array_release(&ctx_layer->render_order);
        wl_array_init(&ctx_layer->render_order);
        unlock_context(ctx);
        return ILM_FAILED;
    }

    ids = *ppArray;
    wl_array_for_each(id, &ctx_layer->render_order) {
        *ids = (t_ilm_surface) *id;
        ids++;
        length++;
    }

    wl_array_release(&ctx_layer->render_order);
    wl_array_init(&ctx_layer->render_order);
    *pLength = length;

    unlock_context(ctx);
    return ILM_SUCCESS;
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

        ivi_wm_create_layout_layer(ctx->wl.controller, layerid, width, height);
        wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        returnValue = ILM_SUCCESS;
    } while(0);

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemove(t_ilm_layer layerId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_destroy_layout_layer(ctx->wl.controller, layerId);
        wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetVisibility(t_ilm_layer layerId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    uint32_t visibility = 0;

    if (newVisibility == ILM_TRUE) {
        visibility = 1;
    }

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_layer_visibility(ctx->wl.controller, layerId, visibility);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetVisibility(t_ilm_layer layerId, t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct layer_context *ctx_layer = NULL;

    if (pVisibility != NULL) {
        lock_context(ctx);

        ivi_wm_layer_get(ctx->wl.controller, layerId, IVI_WM_PARAM_VISIBILITY);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);

        if ((ret != -1) && (ctx_layer != NULL))
        {
            *pVisibility = ctx_layer->prop.visibility;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetOpacity(t_ilm_layer layerId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    wl_fixed_t opacity_fixed = wl_fixed_from_double((double)opacity);

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_layer_opacity(ctx->wl.controller, layerId, opacity_fixed);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetOpacity(t_ilm_layer layerId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct layer_context *ctx_layer = NULL;

    if (pOpacity != NULL) {
        lock_context(ctx);

        ivi_wm_layer_get(ctx->wl.controller, layerId, IVI_WM_PARAM_OPACITY);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->wl, (uint32_t)layerId);

        if ((ret != -1) && (ctx_layer != NULL))
        {
            *pOpacity = ctx_layer->prop.opacity;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetSourceRectangle(t_ilm_layer layerId,
                                t_ilm_uint x, t_ilm_uint y,
                                t_ilm_uint width, t_ilm_uint height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_layer_source_rectangle(ctx->wl.controller, layerId,
                                          (uint32_t)x, (uint32_t)y,
                                          (uint32_t)width, (uint32_t)height);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetDestinationRectangle(t_ilm_layer layerId,
                                 t_ilm_int x, t_ilm_int y,
                                 t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_layer_destination_rectangle(ctx->wl.controller,
                                               layerId, (uint32_t)x,
                                               (uint32_t)y, (uint32_t)width,
                                               (uint32_t)height);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetRenderOrder(t_ilm_layer layerId,
                        t_ilm_surface *pSurfaceId,
                        t_ilm_int number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    t_ilm_int i;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_layer_clear(ctx->wl.controller, layerId);

        for (i = 0; i < number; i++) {
            ivi_wm_layer_add_surface(ctx->wl.controller, layerId,
                                     (uint32_t)pSurfaceId[i]);
        }

        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetVisibility(t_ilm_surface surfaceId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    uint32_t visibility = 0;

    if (newVisibility == ILM_TRUE) {
        visibility = 1;
    }

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_surface_visibility(ctx->wl.controller, surfaceId, visibility);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetOpacity(t_ilm_surface surfaceId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    wl_fixed_t opacity_fixed = wl_fixed_from_double((double)opacity);

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_surface_opacity(ctx->wl.controller, surfaceId, opacity_fixed);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetOpacity(t_ilm_surface surfaceId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct surface_context *ctx_surf = NULL;

    if (pOpacity != NULL) {
        lock_context(ctx);

        ivi_wm_surface_get(ctx->wl.controller, surfaceId, IVI_WM_PARAM_OPACITY);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);

        if ((ret != -1) && (ctx_surf != NULL))
        {
            *pOpacity = ctx_surf->prop.opacity;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetDestinationRectangle(t_ilm_surface surfaceId,
                                   t_ilm_int x, t_ilm_int y,
                                   t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_surface_destination_rectangle(ctx->wl.controller, surfaceId,
                                                 x, y, width, height);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetType(t_ilm_surface surfaceId, ilmSurfaceType type)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    int32_t ivitype = 0;

    switch(type) {
    case ILM_SURFACETYPE_RESTRICTED:
        ivitype = IVI_WM_SURFACE_TYPE_RESTRICTED;
        break;
    case ILM_SURFACETYPE_DESKTOP:
        ivitype = IVI_WM_SURFACE_TYPE_DESKTOP;
        break;
    default:
        ivitype = -1;
        returnValue = ILM_ERROR_INVALID_ARGUMENTS;
        break;
    }

    lock_context(ctx);
    if ((ivitype >= 0) && ctx->wl.controller) {
        ivi_wm_set_surface_type(ctx->wl.controller, surfaceId, type);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_displaySetRenderOrder(t_ilm_display display,
                          t_ilm_layer *pLayerId, const t_ilm_uint number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct screen_context *ctx_scrn = NULL;
    t_ilm_uint i;

    lock_context(ctx);
    ctx_scrn = get_screen_context_by_id(&ctx->wl, (uint32_t)display);
    if (ctx_scrn != NULL) {
        ivi_wm_screen_clear(ctx_scrn->controller);

        for (i = 0; i < number; i++) {
            ivi_wm_screen_add_layer(ctx_scrn->controller, (uint32_t)pLayerId[i]);
        }

        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

static void screenshot_done(void *data, struct ivi_screenshot *ivi_screenshot,
                            int32_t fd, int32_t width, int32_t height,
                            int32_t stride, uint32_t format, uint32_t timestamp)
{
    struct screenshot_context *ctx_scrshot = data;
    char *buffer;
    size_t size = stride * height;
    const char *filename = ctx_scrshot->filename;
    char *filename_ext = NULL;

    ctx_scrshot->filename = NULL;
    ivi_screenshot_destroy(ivi_screenshot);

    if (filename == NULL) {
        ctx_scrshot->result = ILM_FAILED;
        fprintf(stderr, "screenshot file name not provided: %m\n");
        return;
    }

    buffer = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);

    if (buffer == MAP_FAILED) {
        ctx_scrshot->result = ILM_FAILED;
        fprintf(stderr, "failed to mmap screenshot file: %m\n");
        return;
    }

    if ((filename_ext = strstr(filename, ".png")) && (strlen(filename_ext) == 4)) {
        if (save_as_png(filename, (const char *)buffer,
                        width, height, format) == 0) {
            ctx_scrshot->result = ILM_SUCCESS;
        } else {
            ctx_scrshot->result = ILM_FAILED;
            fprintf(stderr, "failed to write screenshot as png file: %m\n");
        }
    } else {
        if (!((filename_ext = strstr(filename, ".bmp")) && (strlen(filename_ext) == 4))) {
            fprintf(stderr, "trying to write screenshot as bmp file, although file extension does not match: %m\n");
        }

        if (save_as_bitmap(filename, (const char *)buffer,
                           width, height, format) == 0) {
            ctx_scrshot->result = ILM_SUCCESS;
        } else {
            ctx_scrshot->result = ILM_FAILED;
            fprintf(stderr, "failed to write screenshot as bmp file: %m\n");
        }
    }

    munmap(buffer, size);
}

static void screenshot_error(void *data, struct ivi_screenshot *ivi_screenshot,
                             uint32_t error, const char *message)
{
    struct screenshot_context *ctx_scrshot = data;
    ctx_scrshot->filename = NULL;
    ivi_screenshot_destroy(ivi_screenshot);
    fprintf(stderr, "screenshot failed, error 0x%x: %s\n", error, message);
}

static struct ivi_screenshot_listener screenshot_listener = {
    screenshot_done,
    screenshot_error,
};

ILM_EXPORT ilmErrorTypes
ilm_takeScreenshot(t_ilm_uint screen, t_ilm_const_string filename)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct screen_context *ctx_scrn = NULL;

    lock_context(ctx);
    ctx_scrn = get_screen_context_by_id(&ctx->wl, (uint32_t)screen);
    if (ctx_scrn != NULL) {
        struct screenshot_context ctx_scrshot = {
            .filename = filename,
            .result = ILM_FAILED,
        };

        struct ivi_screenshot *scrshot =
            ivi_wm_screen_screenshot(ctx_scrn->controller);
        if (scrshot) {
            ivi_screenshot_add_listener(scrshot, &screenshot_listener,
                                        &ctx_scrshot);
            // dispatch until filename has been reset in done or error callback
            int ret;
            do {
                ret =
                    wl_display_dispatch_queue(ctx->wl.display, ctx->wl.queue);
            } while ((ret != -1) && ctx_scrshot.filename);

            returnValue = ctx_scrshot.result;
        }
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_takeSurfaceScreenshot(t_ilm_const_string filename,
                              t_ilm_surface surfaceid)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
          struct screenshot_context ctx_scrshot = {
            .filename = filename,
            .result = ILM_FAILED,
        };

        struct ivi_screenshot *scrshot =
            ivi_wm_surface_screenshot(ctx->wl.controller, surfaceid);
        if (scrshot) {
            ivi_screenshot_add_listener(scrshot, &screenshot_listener,
                                        &ctx_scrshot);
            // dispatch until filename has been reset in done or error callback
            int ret;
            do {
                ret =
                    wl_display_dispatch_queue(ctx->wl.display, ctx->wl.queue);
            } while ((ret != -1) && ctx_scrshot.filename);

            returnValue = ctx_scrshot.result;
        }
    }
    unlock_context(ctx);

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
        ivi_wm_layer_sync(ctx->wl.controller, layer, IVI_WM_SYNC_ADD);
        if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) == -1)
            fprintf(stderr, "wl_display_roundtrip queue failed\n");

        returnValue = ILM_SUCCESS;
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveNotification(t_ilm_layer layer)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = sync_and_acquire_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->wl, (uint32_t)layer);
    if (ctx_layer != NULL) {
        if (ctx_layer->notification != NULL) {
            ivi_wm_layer_sync(ctx->wl.controller, layer, IVI_WM_SYNC_REMOVE);
            wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

            ctx_layer->notification = NULL;
            returnValue = ILM_SUCCESS;
        } else {
            returnValue = ILM_ERROR_INVALID_ARGUMENTS;
        }
    }

    release_instance();
    return returnValue;
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

    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
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
            callback(ILM_LAYER, ctx_layer->id_layer, ILM_TRUE, user_data);
        }

        wl_list_for_each(ctx_surf, &ctx->wl.list_surface, link) {
            callback(ILM_SURFACE, ctx_surf->id_surface, ILM_TRUE, user_data);
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
            ctx_surf->notification = callback;
            ivi_wm_surface_sync(ctx->wl.controller, surface, IVI_WM_SYNC_ADD);
            if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) == -1)
                fprintf(stderr, "wl_display_roundtrip queue failed\n");

            callback(ctx_surf->id_surface,
                     &ctx_surf->prop, ILM_NOTIFICATION_CONTENT_AVAILABLE);
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
            ivi_wm_surface_sync(ctx->wl.controller, surface, IVI_WM_SYNC_REMOVE);
            wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

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
    struct ilm_control_context *const ctx = &ilm_context;
    struct surface_context *ctx_surface = NULL;
    int32_t mask = 0;

    mask |= IVI_WM_PARAM_OPACITY;
    mask |= IVI_WM_PARAM_VISIBILITY;
    mask |= IVI_WM_PARAM_SIZE;

    if (pSurfaceProperties != NULL) {
        lock_context(ctx);

        ivi_wm_surface_get(ctx->wl.controller, surfaceID, mask);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_surface = get_surface_context(&ctx->wl, (uint32_t)surfaceID);

        if ((ret != -1) && (ctx_surface != NULL))
        {
            *pSurfaceProperties = ctx_surface->prop;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerAddSurface(t_ilm_layer layerId,
                        t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_layer_add_surface(ctx->wl.controller, layerId, surfaceId);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveSurface(t_ilm_layer layerId,
                           t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_layer_remove_surface(ctx->wl.controller, layerId, surfaceId);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetVisibility(t_ilm_surface surfaceId,
                             t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;
    struct surface_context *ctx_surf = NULL;

    if (pVisibility != NULL) {
        lock_context(ctx);

        ivi_wm_surface_get(ctx->wl.controller, surfaceId, IVI_WM_PARAM_VISIBILITY);
        int ret = wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue);

        ctx_surf = get_surface_context(&ctx->wl, (uint32_t)surfaceId);

        if ((ret != -1) && (ctx_surf != NULL))
        {
            *pVisibility = (t_ilm_bool)ctx_surf->prop.visibility;
            returnValue = ILM_SUCCESS;
        }

        unlock_context(ctx);
    }

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetSourceRectangle(t_ilm_surface surfaceId,
                                  t_ilm_int x, t_ilm_int y,
                                  t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;


    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_set_surface_source_rectangle(ctx->wl.controller, surfaceId, x, y,
                                            width, height);
        wl_display_flush(ctx->wl.display);
        returnValue = ILM_SUCCESS;
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_commitChanges(void)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        ivi_wm_commit_changes(ctx->wl.controller);

        if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) != -1)
        {
            returnValue = ILM_SUCCESS;
        }
    }
    unlock_context(ctx);

    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getError(void)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *const ctx = &ilm_context;

    lock_context(ctx);
    if (ctx->wl.controller) {
        if (wl_display_roundtrip_queue(ctx->wl.display, ctx->wl.queue) != -1)
        {
            returnValue = ctx->wl.error_flag;
            ctx->wl.error_flag = ILM_SUCCESS;
        }
    }
    unlock_context(ctx);

    return returnValue;
}
