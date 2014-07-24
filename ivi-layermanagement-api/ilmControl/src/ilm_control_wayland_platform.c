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
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include "ilm_common.h"
#include "ilm_control_platform.h"
#include "wayland-util.h"
#include "ivi-controller-client-protocol.h"

static ilmErrorTypes wayland_getPropertiesOfLayer(t_ilm_uint layerID,
                         struct ilmLayerProperties* pLayerProperties);
static ilmErrorTypes wayland_getPropertiesOfScreen(t_ilm_display screenID,
                         struct ilmScreenProperties* pScreenProperties);
static ilmErrorTypes wayland_getNumberOfHardwareLayers(t_ilm_uint screenID,
                         t_ilm_uint* pNumberOfHardwareLayers);
static ilmErrorTypes wayland_getScreenIDs(t_ilm_uint* pNumberOfIDs,
                         t_ilm_uint** ppIDs);
static ilmErrorTypes wayland_getLayerIDs(t_ilm_int* pLength,
                         t_ilm_layer** ppArray);
static ilmErrorTypes wayland_getLayerIDsOnScreen(t_ilm_uint screenId,
                         t_ilm_int* pLength, t_ilm_layer** ppArray);
static ilmErrorTypes wayland_getSurfaceIDs(t_ilm_int* pLength,
                         t_ilm_surface** ppArray);
static ilmErrorTypes wayland_getSurfaceIDsOnLayer(t_ilm_layer layer,
                         t_ilm_int* pLength, t_ilm_surface** ppArray);
static ilmErrorTypes wayland_layerCreateWithDimension(t_ilm_layer* pLayerId,
                         t_ilm_uint width, t_ilm_uint height);
static ilmErrorTypes wayland_layerRemove(t_ilm_layer layerId);
static ilmErrorTypes wayland_layerGetType(t_ilm_layer layerId,
                         ilmLayerType* pLayerType);
static ilmErrorTypes wayland_layerSetVisibility(t_ilm_layer layerId,
                         t_ilm_bool newVisibility);
static ilmErrorTypes wayland_layerGetVisibility(t_ilm_layer layerId,
                         t_ilm_bool *pVisibility);
static ilmErrorTypes wayland_layerSetOpacity(t_ilm_layer layerId,
                         t_ilm_float opacity);
static ilmErrorTypes wayland_layerGetOpacity(t_ilm_layer layerId,
                         t_ilm_float *pOpacity);
static ilmErrorTypes wayland_layerSetSourceRectangle(t_ilm_layer layerId,
                         t_ilm_uint x, t_ilm_uint y,
                         t_ilm_uint width, t_ilm_uint height);
static ilmErrorTypes wayland_layerSetDestinationRectangle(t_ilm_layer layerId,
                         t_ilm_int x, t_ilm_int y,
                         t_ilm_int width, t_ilm_int height);
static ilmErrorTypes wayland_layerGetDimension(t_ilm_layer layerId,
                         t_ilm_uint *pDimension);
static ilmErrorTypes wayland_layerSetDimension(t_ilm_layer layerId,
                         t_ilm_uint *pDimension);
static ilmErrorTypes wayland_layerGetPosition(t_ilm_layer layerId,
                         t_ilm_uint *pPosition);
static ilmErrorTypes wayland_layerSetPosition(t_ilm_layer layerId,
                         t_ilm_uint *pPosition);
static ilmErrorTypes wayland_layerSetOrientation(t_ilm_layer layerId,
                         ilmOrientation orientation);
static ilmErrorTypes wayland_layerGetOrientation(t_ilm_layer layerId,
                         ilmOrientation *pOrientation);
static ilmErrorTypes wayland_layerSetChromaKey(t_ilm_layer layerId,
                         t_ilm_int* pColor);
static ilmErrorTypes wayland_layerSetRenderOrder(t_ilm_layer layerId,
                         t_ilm_layer *pSurfaceId,
                         t_ilm_int number);
static ilmErrorTypes wayland_layerGetCapabilities(t_ilm_layer layerId,
                         t_ilm_layercapabilities *pCapabilities);
static ilmErrorTypes wayland_layerTypeGetCapabilities(ilmLayerType layerType,
                         t_ilm_layercapabilities *pCapabilities);
static ilmErrorTypes wayland_surfaceSetVisibility(t_ilm_surface surfaceId,
                         t_ilm_bool newVisibility);
static ilmErrorTypes wayland_surfaceSetOpacity(t_ilm_surface surfaceId,
                         t_ilm_float opacity);
static ilmErrorTypes wayland_surfaceGetOpacity(t_ilm_surface surfaceId,
                         t_ilm_float *pOpacity);
static ilmErrorTypes wayland_SetKeyboardFocusOn(t_ilm_surface surfaceId);
static ilmErrorTypes wayland_GetKeyboardFocusSurfaceId(
                         t_ilm_surface* pSurfaceId);
static ilmErrorTypes wayland_surfaceSetDestinationRectangle(
                         t_ilm_surface surfaceId,
                         t_ilm_int x, t_ilm_int y,
                         t_ilm_int width, t_ilm_int height);
static ilmErrorTypes wayland_surfaceSetDimension(t_ilm_surface surfaceId,
                         t_ilm_uint *pDimension);
static ilmErrorTypes wayland_surfaceGetPosition(t_ilm_surface surfaceId,
                         t_ilm_uint *pPosition);
static ilmErrorTypes wayland_surfaceSetPosition(t_ilm_surface surfaceId,
                         t_ilm_uint *pPosition);
static ilmErrorTypes wayland_surfaceSetOrientation(t_ilm_surface surfaceId,
                         ilmOrientation orientation);
static ilmErrorTypes wayland_surfaceGetOrientation(t_ilm_surface surfaceId,
                         ilmOrientation *pOrientation);
static ilmErrorTypes wayland_surfaceGetPixelformat(t_ilm_layer surfaceId,
                         ilmPixelFormat *pPixelformat);
static ilmErrorTypes wayland_surfaceSetChromaKey(t_ilm_surface surfaceId,
                         t_ilm_int* pColor);
static ilmErrorTypes wayland_displaySetRenderOrder(t_ilm_display display,
                         t_ilm_layer *pLayerId, const t_ilm_uint number);
static ilmErrorTypes wayland_takeScreenshot(t_ilm_uint screen,
                         t_ilm_const_string filename);
static ilmErrorTypes wayland_takeLayerScreenshot(t_ilm_const_string filename,
                         t_ilm_layer layerid);
static ilmErrorTypes wayland_takeSurfaceScreenshot(t_ilm_const_string filename,
                         t_ilm_surface surfaceid);
static ilmErrorTypes wayland_SetOptimizationMode(ilmOptimization id,
                         ilmOptimizationMode mode);
static ilmErrorTypes wayland_GetOptimizationMode(ilmOptimization id,
                         ilmOptimizationMode* pMode);
static ilmErrorTypes wayland_layerAddNotification(t_ilm_layer layer,
                         layerNotificationFunc callback);
static ilmErrorTypes wayland_layerRemoveNotification(t_ilm_layer layer);
static ilmErrorTypes wayland_surfaceAddNotification(t_ilm_surface surface,
                         surfaceNotificationFunc callback);
static ilmErrorTypes wayland_surfaceRemoveNotification(t_ilm_surface surface);
static ilmErrorTypes wayland_init(t_ilm_nativedisplay nativedisplay);
static void wayland_destroy(void);
static ilmErrorTypes wayland_getNativeHandle(t_ilm_uint pid,
                         t_ilm_int *n_handle,
                         t_ilm_nativehandle **p_handles);
static ilmErrorTypes wayland_getPropertiesOfSurface(t_ilm_uint surfaceID,
                         struct ilmSurfaceProperties* pSurfaceProperties);
static ilmErrorTypes wayland_layerAddSurface(t_ilm_layer layerId,
                         t_ilm_surface surfaceId);
static ilmErrorTypes wayland_layerRemoveSurface(t_ilm_layer layerId,
                         t_ilm_surface surfaceId);
static ilmErrorTypes wayland_surfaceGetDimension(t_ilm_surface surfaceId,
                         t_ilm_uint *pDimension);
static ilmErrorTypes wayland_surfaceGetVisibility(t_ilm_surface surfaceId,
                         t_ilm_bool *pVisibility);
static ilmErrorTypes wayland_surfaceSetSourceRectangle(t_ilm_surface surfaceId,
                         t_ilm_int x, t_ilm_int y,
                         t_ilm_int width, t_ilm_int height);
static ilmErrorTypes wayland_commitChanges(void);

void init_ilmControlPlatformTable(void)
{
    gIlmControlPlatformFunc.getPropertiesOfLayer =
        wayland_getPropertiesOfLayer;
    gIlmControlPlatformFunc.getPropertiesOfScreen =
        wayland_getPropertiesOfScreen;
    gIlmControlPlatformFunc.getNumberOfHardwareLayers =
        wayland_getNumberOfHardwareLayers;
    gIlmControlPlatformFunc.getScreenIDs =
        wayland_getScreenIDs;
    gIlmControlPlatformFunc.getLayerIDs =
        wayland_getLayerIDs;
    gIlmControlPlatformFunc.getLayerIDsOnScreen =
        wayland_getLayerIDsOnScreen;
    gIlmControlPlatformFunc.getSurfaceIDs =
        wayland_getSurfaceIDs;
    gIlmControlPlatformFunc.getSurfaceIDsOnLayer =
        wayland_getSurfaceIDsOnLayer;
    gIlmControlPlatformFunc.layerCreateWithDimension =
        wayland_layerCreateWithDimension;
    gIlmControlPlatformFunc.layerRemove =
        wayland_layerRemove;
    gIlmControlPlatformFunc.layerGetType =
        wayland_layerGetType;
    gIlmControlPlatformFunc.layerSetVisibility =
        wayland_layerSetVisibility;
    gIlmControlPlatformFunc.layerGetVisibility =
        wayland_layerGetVisibility;
    gIlmControlPlatformFunc.layerSetOpacity =
        wayland_layerSetOpacity;
    gIlmControlPlatformFunc.layerGetOpacity =
        wayland_layerGetOpacity;
    gIlmControlPlatformFunc.layerSetSourceRectangle =
        wayland_layerSetSourceRectangle;
    gIlmControlPlatformFunc.layerSetDestinationRectangle =
        wayland_layerSetDestinationRectangle;
    gIlmControlPlatformFunc.layerGetDimension =
        wayland_layerGetDimension;
    gIlmControlPlatformFunc.layerSetDimension =
        wayland_layerSetDimension;
    gIlmControlPlatformFunc.layerGetPosition =
        wayland_layerGetPosition;
    gIlmControlPlatformFunc.layerSetPosition =
        wayland_layerSetPosition;
    gIlmControlPlatformFunc.layerSetOrientation =
        wayland_layerSetOrientation;
    gIlmControlPlatformFunc.layerGetOrientation =
        wayland_layerGetOrientation;
    gIlmControlPlatformFunc.layerSetChromaKey =
        wayland_layerSetChromaKey;
    gIlmControlPlatformFunc.layerSetRenderOrder =
        wayland_layerSetRenderOrder;
    gIlmControlPlatformFunc.layerGetCapabilities =
        wayland_layerGetCapabilities;
    gIlmControlPlatformFunc.layerTypeGetCapabilities =
        wayland_layerTypeGetCapabilities;
    gIlmControlPlatformFunc.surfaceSetVisibility =
        wayland_surfaceSetVisibility;
    gIlmControlPlatformFunc.surfaceSetOpacity =
        wayland_surfaceSetOpacity;
    gIlmControlPlatformFunc.surfaceGetOpacity =
        wayland_surfaceGetOpacity;
    gIlmControlPlatformFunc.SetKeyboardFocusOn =
        wayland_SetKeyboardFocusOn;
    gIlmControlPlatformFunc.GetKeyboardFocusSurfaceId =
        wayland_GetKeyboardFocusSurfaceId;
    gIlmControlPlatformFunc.surfaceSetDestinationRectangle =
        wayland_surfaceSetDestinationRectangle;
    gIlmControlPlatformFunc.surfaceSetDimension =
        wayland_surfaceSetDimension;
    gIlmControlPlatformFunc.surfaceGetPosition =
        wayland_surfaceGetPosition;
    gIlmControlPlatformFunc.surfaceSetPosition =
        wayland_surfaceSetPosition;
    gIlmControlPlatformFunc.surfaceSetOrientation =
        wayland_surfaceSetOrientation;
    gIlmControlPlatformFunc.surfaceGetOrientation =
        wayland_surfaceGetOrientation;
    gIlmControlPlatformFunc.surfaceGetPixelformat =
        wayland_surfaceGetPixelformat;
    gIlmControlPlatformFunc.surfaceSetChromaKey =
        wayland_surfaceSetChromaKey;
    gIlmControlPlatformFunc.displaySetRenderOrder =
        wayland_displaySetRenderOrder;
    gIlmControlPlatformFunc.takeScreenshot =
        wayland_takeScreenshot;
    gIlmControlPlatformFunc.takeLayerScreenshot =
        wayland_takeLayerScreenshot;
    gIlmControlPlatformFunc.takeSurfaceScreenshot =
        wayland_takeSurfaceScreenshot;
    gIlmControlPlatformFunc.SetOptimizationMode =
        wayland_SetOptimizationMode;
    gIlmControlPlatformFunc.GetOptimizationMode =
        wayland_GetOptimizationMode;
    gIlmControlPlatformFunc.layerAddNotification =
        wayland_layerAddNotification;
    gIlmControlPlatformFunc.layerRemoveNotification =
        wayland_layerRemoveNotification;
    gIlmControlPlatformFunc.surfaceAddNotification =
        wayland_surfaceAddNotification;
    gIlmControlPlatformFunc.surfaceRemoveNotification =
        wayland_surfaceRemoveNotification;
    gIlmControlPlatformFunc.init =
        wayland_init;
    gIlmControlPlatformFunc.destroy =
        wayland_destroy;
    gIlmControlPlatformFunc.getNativeHandle =
        wayland_getNativeHandle;
    gIlmControlPlatformFunc.getPropertiesOfSurface =
        wayland_getPropertiesOfSurface;
    gIlmControlPlatformFunc.layerAddSurface =
        wayland_layerAddSurface;
    gIlmControlPlatformFunc.layerRemoveSurface =
        wayland_layerRemoveSurface;
    gIlmControlPlatformFunc.surfaceGetDimension =
        wayland_surfaceGetDimension;
    gIlmControlPlatformFunc.surfaceGetVisibility =
        wayland_surfaceGetVisibility;
    gIlmControlPlatformFunc.surfaceSetSourceRectangle =
        wayland_surfaceSetSourceRectangle;
    gIlmControlPlatformFunc.commitChanges =
        wayland_commitChanges;
}

struct surface_context {
    struct wl_list link;

    struct ivi_surface *surface;
    struct ivi_controller_surface *controller;

    t_ilm_uint id_surface;
    struct ilmSurfaceProperties prop;
    surfaceNotificationFunc notification;

    struct {
        struct wl_list link;
    } order;

    struct ilm_control_context *ctx;
};

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

    struct ilmScreenProperties prop;

    struct {
        struct wl_list list_layer;
        struct wl_list link;
    } order;

    struct ilm_control_context *ctx;
};

struct nativehandle_context {
    uint32_t pid;
    uint32_t nativehandle;
    struct wl_list link;
};

struct wayland_context {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct ivi_controller *controller;
    uint32_t num_screen;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;
};

struct ilm_control_context {
    struct wayland_context main_ctx;
    struct wayland_context child_ctx;
    int32_t valid;

    uint32_t num_screen;

    uint32_t internal_id_layer;

    struct wl_list list_nativehandle;

    pthread_t thread;
    pthread_mutex_t mutex;
    uint32_t internal_id_surface;
};

static int create_controller_layer(struct wayland_context *ctx, t_ilm_uint width, t_ilm_uint height, t_ilm_layer layerid);

static int32_t
wayland_controller_is_inside_surface_list(struct wl_list *list,
                                          uint32_t id_surface)
{
    struct surface_context *ctx_surf = NULL;
    wl_list_for_each(ctx_surf, list, link) {
        if (ctx_surf->id_surface == id_surface) {
            return 1;
        }
    }

    return 0;
}

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

    fprintf(stderr, "failed to get layer context in ilmControl\n");
    return NULL;
}

static struct ilm_control_context* get_instance(void);

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
    (void)transform;
}

static void
output_listener_mode(void *data,
                     struct wl_output *output,
                     uint32_t flags,
                     int32_t width,
                     int32_t height,
                     int32_t refresh)
{
    (void)data;
    (void)output;
    (void)flags;
    (void)width;
    (void)height;
    (void)refresh;
    struct screen_context *ctx_scrn = data;
    ctx_scrn->prop.screenWidth = width;
    ctx_scrn->prop.screenHeight = height;
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
get_screen_context_by_output(struct wayland_context *ctx,
                             struct wl_output *output)
{
    struct screen_context *ctx_scrn = NULL;
    struct wl_proxy *pxy_out = NULL;
    struct wl_proxy *pxy_out_in_scrn = NULL;
    uint32_t pxy_id = 0;
    uint32_t pxy_id_in_scrn = 0;

    wl_list_for_each(ctx_scrn, &ctx->list_screen, link) {
        pxy_out = (struct wl_proxy*)output;
        pxy_out_in_scrn = (struct wl_proxy*)ctx_scrn->output;
        pxy_id = wl_proxy_get_id(pxy_out);
        pxy_id_in_scrn = wl_proxy_get_id(pxy_out_in_scrn);
        if (pxy_id == pxy_id_in_scrn) {
            return ctx_scrn;
        }
    }
    return NULL;
}

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
    struct screen_context *ctx_scrn = NULL;

    ctx_scrn = get_screen_context_by_output(ctx_layer->ctx, output);
    if (ctx_scrn == NULL) {
        fprintf(stderr, "failed to add_orderlayer_to_screen\n");
        return;
    }

    int found = 0;
    struct layer_context *layer_link;
    wl_list_for_each(layer_link, &ctx_scrn->order.list_layer, order.link) {
        if (layer_link == ctx_layer) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        wl_list_init(&ctx_layer->order.link);
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
controller_layer_listener_visibility_child(void *data,
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
controller_layer_listener_opacity_child(void *data,
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
controller_layer_listener_source_rectangle_child(void *data,
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
controller_layer_listener_destination_rectangle_child(void *data,
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
controller_layer_listener_configuration_child(void *data,
                          struct ivi_controller_layer *controller,
                          int32_t width,
                          int32_t height)
{
    struct layer_context *ctx_layer = data;

    ctx_layer->prop.sourceWidth = (t_ilm_uint)width;
    ctx_layer->prop.sourceHeight = (t_ilm_uint)height;
}

static void
controller_layer_listener_orientation_child(void *data,
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
controller_layer_listener_screen_child(void *data,
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
controller_layer_listener_destroyed_child(void *data,
                                    struct ivi_controller_layer *controller)
{
    struct layer_context *ctx_layer = data;
    wl_list_remove(&ctx_layer->link);
    free(ctx_layer);
}

static struct ivi_controller_layer_listener controller_layer_listener =
{
    controller_layer_listener_visibility_child,
    controller_layer_listener_opacity_child,
    controller_layer_listener_source_rectangle_child,
    controller_layer_listener_destination_rectangle_child,
    controller_layer_listener_configuration_child,
    controller_layer_listener_orientation_child,
    controller_layer_listener_screen_child,
    controller_layer_listener_destroyed_child
};

static void
add_ordersurface_to_layer(struct wayland_context *ctx,
                          struct surface_context *ctx_surf,
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
        wl_list_init(&ctx_surf->order.link);
        wl_list_insert(&ctx_layer->order.list_surface, &ctx_surf->order.link);
    }
}

static void
remove_ordersurface_from_layer(struct wayland_context *ctx,
                               struct surface_context *ctx_surf)
{
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_ordersurf = NULL;
    struct surface_context *next = NULL;
    struct wl_proxy *pxy_surf = NULL;
    struct wl_proxy *pxy_ordersurf = NULL;
    uint32_t id_surf = 0;
    uint32_t id_ordersurf = 0;

    wl_list_for_each(ctx_layer, &ctx->list_layer, link) {
        wl_list_for_each_safe(ctx_ordersurf, next,
                         &ctx_layer->order.list_surface,
                         order.link) {
            if (ctx_surf->id_surface == ctx_ordersurf->id_surface) {
                wl_list_remove(&ctx_ordersurf->order.link);
            }
        }
    }
}

static struct surface_context*
get_surface_context_by_controller(struct wayland_context *ctx,
                                  struct ivi_controller_surface *controller)
{
    struct surface_context *ctx_surf = NULL;
    wl_list_for_each(ctx_surf, &ctx->list_surface, link) {
        if (ctx_surf->controller == controller) {
            return ctx_surf;
        }
    }

    fprintf(stderr, "failed to get surface context in %s\n", __FUNCTION__);
    return NULL;
}

static void
controller_surface_listener_visibility_child(void *data,
                            struct ivi_controller_surface *controller,
                            int32_t visibility)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    ctx_surf->prop.visibility = (t_ilm_bool)visibility;

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_VISIBILITY);
    }
}

static void
controller_surface_listener_opacity_child(void *data,
                         struct ivi_controller_surface *controller,
                         wl_fixed_t opacity)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    ctx_surf->prop.opacity = (t_ilm_float)wl_fixed_to_double(opacity);

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_OPACITY);
    }
}

static void
controller_surface_listener_configuration_child(void *data,
                           struct ivi_controller_surface *controller,
                           int32_t width,
                           int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    if (ctx_surf != NULL) {
        ctx_surf->prop.sourceWidth = (t_ilm_uint)width;
        ctx_surf->prop.sourceHeight = (t_ilm_uint)height;
    }
}

static void
controller_surface_listener_source_rectangle_child(void *data,
                                  struct ivi_controller_surface *controller,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    ctx_surf->prop.sourceX = (t_ilm_uint)x;
    ctx_surf->prop.sourceY = (t_ilm_uint)y;
    ctx_surf->prop.sourceWidth = (t_ilm_uint)width;
    ctx_surf->prop.sourceHeight = (t_ilm_uint)height;
    if (ctx_surf->prop.origSourceWidth == 0) {
        ctx_surf->prop.origSourceWidth = (t_ilm_uint)width;
    }
    if (ctx_surf->prop.origSourceHeight == 0) {
        ctx_surf->prop.origSourceHeight = (t_ilm_uint)height;
    }

    if (ctx_surf->notification != NULL) {
        ctx_surf->notification(ctx_surf->id_surface,
                                &ctx_surf->prop,
                                ILM_NOTIFICATION_SOURCE_RECT);
    }
}

static void
controller_surface_listener_destination_rectangle_child(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

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
controller_surface_listener_orientation_child(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t orientation)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;
    ilmOrientation ilmorientation = ILM_ZERO;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

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
controller_surface_listener_pixelformat_child(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t pixelformat)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    if (ctx_surf != NULL) {
        ctx_surf->prop.pixelformat = (t_ilm_uint)pixelformat;
    }
}

static void
controller_surface_listener_layer_child(void *data,
                                  struct ivi_controller_surface *controller,
                                  struct ivi_controller_layer *layer)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    if (layer == NULL) {
        remove_ordersurface_from_layer(ctx, ctx_surf);
    } else {
        add_ordersurface_to_layer(ctx, ctx_surf, layer);
    }
}

static void
controller_surface_listener_stats_child(void *data,
                                  struct ivi_controller_surface *controller,
                                  uint32_t redraw_count,
                                  uint32_t frame_count,
                                  uint32_t update_count,
                                  uint32_t pid,
                                  const char *process_name)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;
    (void)process_name;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);

    }

    if (ctx_surf != NULL) {
        ctx_surf->prop.drawCounter = (t_ilm_uint)redraw_count;
        ctx_surf->prop.frameCounter = (t_ilm_uint)frame_count;
        ctx_surf->prop.updateCounter = (t_ilm_uint)update_count;
        ctx_surf->prop.creatorPid = (t_ilm_uint)pid;
    }
}

static void
controller_surface_listener_destroyed_child(void *data,
                  struct ivi_controller_surface *controller)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    wl_list_remove(&ctx_surf->link);
    free(ctx_surf);
}

static void
controller_surface_listener_content_child(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t content_state)
{
    // if client surface (=content) was removed with ilm_surfaceDestroy()
    // the expected behavior within ILM API mandates a full removal
    // of the surface from the scene. We must remove the controller
    // from scene, too.
    if (IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED == content_state)
    {
        struct wayland_context *ctx = data;
        struct surface_context *ctx_surf = NULL;

        ctx_surf = get_surface_context_by_controller(ctx, controller);
        if (ctx_surf == NULL) {
            fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
            return;
        }

        ivi_controller_surface_destroy(controller, 1);

        wl_list_remove(&ctx_surf->link);
        free(ctx_surf);
    }
}

static void
controller_surface_listener_input_focus_child(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t enabled)
{
    (void)data;
    (void)controller;
    (void)enabled;
}

static struct ivi_controller_surface_listener controller_surface_listener_child =
{
    controller_surface_listener_visibility_child,
    controller_surface_listener_opacity_child,
    controller_surface_listener_source_rectangle_child,
    controller_surface_listener_destination_rectangle_child,
    controller_surface_listener_configuration_child,
    controller_surface_listener_orientation_child,
    controller_surface_listener_pixelformat_child,
    controller_surface_listener_layer_child,
    controller_surface_listener_stats_child,
    controller_surface_listener_destroyed_child,
    controller_surface_listener_content_child,
    controller_surface_listener_input_focus_child
};

static void
controller_surface_listener_visibility_main(void *data,
                            struct ivi_controller_surface *controller,
                            int32_t visibility)
{
    (void)data;
    (void)controller;
    (void)visibility;
}

static void
controller_surface_listener_opacity_main(void *data,
                         struct ivi_controller_surface *controller,
                         wl_fixed_t opacity)
{
    (void)data;
    (void)controller;
    (void)opacity;
}

static void
controller_surface_listener_source_rectangle_main(void *data,
                                  struct ivi_controller_surface *controller,
                                  int32_t x,
                                  int32_t y,
                                  int32_t width,
                                  int32_t height)
{
    (void)data;
    (void)controller;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

static void
controller_surface_listener_destination_rectangle_main(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    (void)data;
    (void)controller;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

static void
controller_surface_listener_configuration_main(void *data,
                           struct ivi_controller_surface *controller,
                           int32_t width,
                           int32_t height)
{
    (void)data;
    (void)controller;
    (void)width;
    (void)height;
}

static void
controller_surface_listener_orientation_main(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t orientation)
{
    (void)data;
    (void)controller;
    (void)orientation;
}

static void
controller_surface_listener_pixelformat_main(void *data,
                             struct ivi_controller_surface *controller,
                             int32_t pixelformat)
{
    (void)data;
    (void)controller;
    (void)pixelformat;
}

static void
controller_surface_listener_layer_main(void *data,
                                  struct ivi_controller_surface *controller,
                                  struct ivi_controller_layer *layer)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    if (layer == NULL) {
        remove_ordersurface_from_layer(ctx, ctx_surf);
    } else {
        add_ordersurface_to_layer(ctx, ctx_surf, layer);
    }
}

static void
controller_surface_listener_stats_main(void *data,
                                  struct ivi_controller_surface *controller,
                                  uint32_t redraw_count,
                                  uint32_t frame_count,
                                  uint32_t update_count,
                                  uint32_t pid,
                                  const char *process_name)
{
    (void)data;
    (void)controller;
    (void)redraw_count;
    (void)frame_count;
    (void)update_count;
    (void)pid;
    (void)process_name;
}

static void
controller_surface_listener_destroyed_main(void *data,
                  struct ivi_controller_surface *controller)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_controller(ctx, controller);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
        return;
    }

    wl_list_remove(&ctx_surf->link);
    free(ctx_surf);
}

static void
controller_surface_listener_content_main(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t content_state)
{
    // if client surface (=content) was removed with ilm_surfaceDestroy()
    // the expected behavior within ILM API mandates a full removal
    // of the surface from the scene. We must remove the controller
    // from scene, too.
    if (IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED == content_state)
    {
        struct wayland_context *ctx = data;
        struct surface_context *ctx_surf = NULL;

        ctx_surf = get_surface_context_by_controller(ctx, controller);
        if (ctx_surf == NULL) {
            fprintf(stderr, "Invalid controller_surface in %s\n", __FUNCTION__);
            return;
        }

        ivi_controller_surface_destroy(controller, 1);

        wl_list_remove(&ctx_surf->link);
        free(ctx_surf);
    }
}

static void
controller_surface_listener_input_focus_main(void *data,
                   struct ivi_controller_surface *controller,
                   int32_t enabled)
{
    (void)data;
    (void)controller;
    (void)enabled;
}

static struct ivi_controller_surface_listener controller_surface_listener_main =
{
    controller_surface_listener_visibility_main,
    controller_surface_listener_opacity_main,
    controller_surface_listener_source_rectangle_main,
    controller_surface_listener_destination_rectangle_main,
    controller_surface_listener_configuration_main,
    controller_surface_listener_orientation_main,
    controller_surface_listener_pixelformat_main,
    controller_surface_listener_layer_main,
    controller_surface_listener_stats_main,
    controller_surface_listener_destroyed_main,
    controller_surface_listener_content_main,
    controller_surface_listener_input_focus_main
};

static void
controller_listener_screen_for_child(void *data,
                           struct ivi_controller *ivi_controller,
                           uint32_t id_screen,
                           struct ivi_controller_screen *controller_screen)
{
    (void)data;
    (void)ivi_controller;
    (void)id_screen;
    (void)controller_screen;
}

static void
controller_listener_layer_for_child(void *data,
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
controller_listener_surface_for_child(void *data,
                            struct ivi_controller *controller,
                            uint32_t id_surface)
{
    struct wayland_context *ctx = data;
    struct surface_context *ctx_surf = NULL;
    int32_t is_inside = 0;

    is_inside = wayland_controller_is_inside_surface_list(
                    &ctx->list_surface, id_surface);

    if (is_inside != 0) {
        fprintf(stderr, "invalid id_surface in controller_listener_surface\n");
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
        fprintf(stderr, "Failed to create controller surface\n");
        return;
    }
    ctx_surf->id_surface = id_surface;
    ctx_surf->prop.inputDevicesAcceptance = ILM_INPUT_DEVICE_ALL;

    wl_list_init(&ctx_surf->link);
    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
    ivi_controller_surface_add_listener(ctx_surf->controller,
                                        &controller_surface_listener_child, ctx);
    wl_display_roundtrip(ctx->display);
}

static void
controller_listener_error_for_child(void *data,
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

static struct ivi_controller_listener controller_listener_for_child = {
    controller_listener_screen_for_child,
    controller_listener_layer_for_child,
    controller_listener_surface_for_child,
    controller_listener_error_for_child
};

static void
controller_listener_screen_for_main(void *data,
                           struct ivi_controller *ivi_controller,
                           uint32_t id_screen,
                           struct ivi_controller_screen *controller_screen)
{
    struct ilm_control_context *ctx = data;
    struct screen_context *ctx_screen;
    (void)ivi_controller;

    ctx_screen = get_screen_context_by_serverid(&ctx->main_ctx, id_screen);
    if (ctx_screen == NULL) {
        fprintf(stderr, "Failed to allocate memory for screen_context\n");
        return;
    }
    ctx_screen->controller = controller_screen;
}

static struct ivi_controller_listener controller_listener_for_main = {
    controller_listener_screen_for_main,
    controller_listener_layer_for_child,
    controller_listener_surface_for_child,
    controller_listener_error_for_child
};

static void
registry_handle_control_for_child(void *data,
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
                                       &controller_listener_for_child,
                                       ctx)) {
            fprintf(stderr, "Failed to add ivi_controller listener\n");
            return;
        }
    } else if (strcmp(interface, "wl_output") == 0) {
        struct screen_context *ctx_scrn = calloc(1, sizeof *ctx_scrn);
        if (ctx_scrn == NULL) {
            fprintf(stderr, "Failed to allocate memory for screen_context\n");
            return;
        }
        wl_list_init(&ctx_scrn->link);
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

        ctx_scrn->id_screen = ctx->num_screen;
        ctx->num_screen++;
        wl_list_init(&ctx_scrn->order.list_layer);
        wl_list_insert(&ctx->list_screen, &ctx_scrn->link);
    }
}

static const struct wl_registry_listener
registry_control_listener_for_child = {
    registry_handle_control_for_child,
    NULL
};

static void
registry_handle_control_for_main(void *data,
                       struct wl_registry *registry,
                       uint32_t name, const char *interface,
                       uint32_t version)
{
    struct ilm_control_context *ctx = data;
    (void)version;

    if (strcmp(interface, "ivi_controller") == 0) {
        ctx->main_ctx.controller = wl_registry_bind(registry, name,
                                           &ivi_controller_interface, 1);
        if (ctx->main_ctx.controller == NULL) {
            fprintf(stderr, "Failed to registry bind ivi_controller\n");
            return;
        }
        if (ivi_controller_add_listener(ctx->main_ctx.controller,
                                       &controller_listener_for_main,
                                       ctx)) {
            fprintf(stderr, "Failed to add ivi_controller listener\n");
            return;
        }
    } else if (strcmp(interface, "wl_output") == 0) {

        struct screen_context *ctx_scrn = calloc(1, sizeof *ctx_scrn);
        struct wl_proxy *pxy = NULL;

        if (ctx_scrn == NULL) {
            fprintf(stderr, "Failed to allocate memory for screen_context\n");
            return;
        }
        wl_list_init(&ctx_scrn->link);
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
        ctx_scrn->controller = NULL;
        ctx_scrn->prop.layerCount = 0;
        ctx_scrn->prop.layerIds = NULL;
        ctx_scrn->prop.harwareLayerCount = 0;
        ctx_scrn->prop.screenWidth = 0;
        ctx_scrn->prop.screenHeight = 0;
        wl_list_init(&ctx_scrn->order.list_layer);
        wl_list_insert(&ctx->main_ctx.list_screen, &ctx_scrn->link);
    }
}

static const struct wl_registry_listener
registry_control_listener_for_main = {
    registry_handle_control_for_main,
    NULL
};

static struct ilm_control_context ilm_context = {0};

static void
wayland_destroy(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    ctx->valid = 0;
    void* threadRetVal = NULL;
    pthread_cancel(ctx->thread);
    if (0 != pthread_join(ctx->thread, &threadRetVal)) {
        fprintf(stderr, "failed to join control thread\n");
    }
}

static void
destroy_control_resources(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    struct screen_context *ctx_scrn;
    struct screen_context *next;

    wl_list_for_each_safe(ctx_scrn, next, &ctx->child_ctx.list_screen, link) {
        if (ctx_scrn->output != NULL) {
            wl_list_remove(&ctx_scrn->link);
            wl_output_destroy(ctx_scrn->output);
            free(ctx_scrn);
        }
    }
    if (ctx->child_ctx.controller != NULL) {
        ivi_controller_destroy(ctx->child_ctx.controller);
        ctx->child_ctx.controller = NULL;
    }

    wl_display_flush(ctx->child_ctx.display);

    if (ctx->child_ctx.display != NULL) {
        wl_display_disconnect(ctx->child_ctx.display);
        ctx->child_ctx.display = NULL;
    }

    if (0 != pthread_mutex_destroy(&ctx->mutex)) {
        fprintf(stderr, "failed to destroy pthread_mutex\n");
    }
}

static void
wayland_context_init(struct wayland_context *ctx)
{
    wl_list_init(&ctx->list_screen);
    wl_list_init(&ctx->list_layer);
    wl_list_init(&ctx->list_surface);
}

static ilmErrorTypes
wayland_init(t_ilm_nativedisplay nativedisplay)
{
    struct ilm_control_context *ctx = &ilm_context;

    if (nativedisplay == 0) {
        return ILM_ERROR_INVALID_ARGUMENTS;
    }

    memset(ctx, 0, sizeof *ctx);

    ctx->main_ctx.display = (struct wl_display*)nativedisplay;

    int ans = 0;
    ans = pthread_mutex_init(&ctx->mutex, NULL);
    if (ans != 0) {
        fprintf(stderr, "failed to initialize pthread_mutex\n");
    }

    ctx->internal_id_surface = 0;
    ctx->num_screen = 0;

    wayland_context_init(&ctx->main_ctx);
    wayland_context_init(&ctx->child_ctx);

    return ILM_SUCCESS;
}

static void*
control_thread(void *p_ret)
{
    struct ilm_control_context *ctx = &ilm_context;
    struct wayland_context *child_ctx = &ctx->child_ctx;
    (void)p_ret;

    ctx->num_screen = 0;
    wl_list_init(&child_ctx->list_screen);
    wl_list_init(&child_ctx->list_layer);
    wl_list_init(&child_ctx->list_surface);

    child_ctx->display = wl_display_connect(NULL);
    if (child_ctx->display == NULL) {
        fprintf(stderr, "Failed to connect display in libilmCommon\n");
        return NULL;
    }

    child_ctx->registry = wl_display_get_registry(child_ctx->display);
    if (child_ctx->registry == NULL) {
        fprintf(stderr, "Failed to get registry\n");
        return NULL;
    }
    if (wl_registry_add_listener(child_ctx->registry,
            &registry_control_listener_for_child, &ctx->child_ctx)) {
        fprintf(stderr, "Failed to add registry listener\n");
        return NULL;
    }

    wl_display_dispatch(child_ctx->display);
    wl_display_roundtrip(child_ctx->display);

    ctx->valid = 1;
    while (0 < ctx->valid)
    {
        wl_display_dispatch(child_ctx->display);
    }

    destroy_control_resources();

    return NULL;
}

static void
init_control(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    struct wayland_context *main_ctx = &ctx->main_ctx;
    int wait_count = 0;
    int ret = 0;
    pthread_attr_t thread_attrs;

    wl_list_init(&ctx->list_nativehandle);

    pthread_attr_init(&thread_attrs);
    pthread_attr_setdetachstate(&thread_attrs, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&ctx->thread, &thread_attrs,
                         control_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to start internal receive \
                thread. returned %d\n", ret);
        return;
    }

    /* registry_add_listener for request by ivi-controller */
    main_ctx->registry = wl_display_get_registry(main_ctx->display);
    if (main_ctx->registry == NULL) {
        fprintf(stderr, "Failed to get registry\n");
        return;
    }
    if (wl_registry_add_listener(main_ctx->registry,
                             &registry_control_listener_for_main, ctx)) {
        fprintf(stderr, "Failed to add registry listener\n");
        return;
    }

    wl_display_dispatch(main_ctx->display);
    wl_display_roundtrip(main_ctx->display);

    ctx->valid = 1;

    /* Wait for bind to wayland interface */
    do {
        if (ctx->child_ctx.controller != NULL) {
            break;
        }
        usleep(10000);
    } while (++wait_count <= 1000); /* wait until 10sec */

    if ((ctx->child_ctx.display == NULL) || (ctx->child_ctx.controller == NULL)) {
        fprintf(stderr, "Failed to connect display\n");
        return;
    }
}

static struct ilm_control_context*
get_instance(void)
{
    struct ilm_control_context *ctx = &ilm_context;
    if (ctx->valid == 0) {
        init_control();
    }

    if (ctx->valid < 0) {
        exit(0);
    }

    wl_display_roundtrip(ctx->main_ctx.display);
    return ctx;
}

static uint32_t
gen_layer_id(struct ilm_control_context *ctx)
{
    struct layer_context *ctx_layer = NULL;
    do {
        int found = 0;
        if (wl_list_length(&ctx->main_ctx.list_layer) == 0) {
            ctx->internal_id_layer++;
            return ctx->internal_id_layer;
        }
        wl_list_for_each(ctx_layer, &ctx->main_ctx.list_layer, link) {
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

    fprintf(stderr, "failed to get surface context in ilmControl\n");
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

static ilmErrorTypes
wayland_getPropertiesOfLayer(t_ilm_uint layerID,
                         struct ilmLayerProperties* pLayerProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pLayerProperties != NULL) {

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerID);

        if (ctx_layer != NULL) {
            *pLayerProperties = ctx_layer->prop;
            returnValue = ILM_SUCCESS;
        }
    }

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

static ilmErrorTypes
wayland_getPropertiesOfScreen(t_ilm_display screenID,
                              struct ilmScreenProperties* pScreenProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pScreenProperties != NULL) {
        struct screen_context *ctx_screen = NULL;
        ctx_screen = get_screen_context_by_id(&ctx->main_ctx, (uint32_t)screenID);
        if (ctx_screen != NULL) {
            *pScreenProperties = ctx_screen->prop;
            create_layerids(ctx_screen, &pScreenProperties->layerIds,
                                        &pScreenProperties->layerCount);
            returnValue = ILM_SUCCESS;
        }
    }
    else {
        pScreenProperties->layerCount = 0;
        pScreenProperties->harwareLayerCount = 0;
        pScreenProperties->layerIds = NULL;
        pScreenProperties->screenWidth = 0;
        pScreenProperties->screenHeight = 0;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_getNumberOfHardwareLayers(t_ilm_uint screenID,
                                  t_ilm_uint* pNumberOfHardwareLayers)
{
    (void)screenID;
    /* Not supported */
    if (pNumberOfHardwareLayers != NULL) {
        *pNumberOfHardwareLayers = 0;
        return ILM_SUCCESS;
    } else {
        return ILM_FAILED;
    }
}

static ilmErrorTypes
wayland_getScreenIDs(t_ilm_uint* pNumberOfIDs, t_ilm_uint** ppIDs)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if ((pNumberOfIDs != NULL) && (ppIDs != NULL)) {
        struct screen_context *ctx_scrn = NULL;
        t_ilm_uint length = wl_list_length(&ctx->main_ctx.list_screen);
        *pNumberOfIDs = 0;

        *ppIDs = (t_ilm_uint*)malloc(length * sizeof *ppIDs);
        if (*ppIDs != NULL) {
            t_ilm_uint* ids = *ppIDs;
            wl_list_for_each(ctx_scrn, &ctx->main_ctx.list_screen, link) {
                *ids = ctx_scrn->id_screen;
                ids++;
            }
            *pNumberOfIDs = length;

            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_getLayerIDs(t_ilm_int* pLength, t_ilm_layer** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct layer_context *ctx_layer = NULL;
        t_ilm_uint length = wl_list_length(&ctx->main_ctx.list_layer);
        *pLength = 0;

        *ppArray = (t_ilm_layer*)malloc(length * sizeof *ppArray);
        if (*ppArray != NULL) {
            // compositor sends layers in opposite order
            // write ids from back to front to turn them around
            t_ilm_layer* ids = *ppArray;
            wl_list_for_each_reverse(ctx_layer, &ctx->main_ctx.list_layer, link)
            {
                *ids = ctx_layer->id_layer;
                ++ids;
            }
            *pLength = length;

            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_getLayerIDsOnScreen(t_ilm_uint screenId,
                            t_ilm_int* pLength,
                            t_ilm_layer** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct screen_context *ctx_screen = NULL;
        ctx_screen = get_screen_context_by_id(&ctx->main_ctx, screenId);
        if (ctx_screen != NULL) {
            struct layer_context *ctx_layer = NULL;
            t_ilm_int length = wl_list_length(&ctx_screen->order.list_layer);

            if (0 < length)
            {
                *ppArray = (t_ilm_layer*)malloc(length * sizeof *ppArray);
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

    return returnValue;
}

static ilmErrorTypes
wayland_getSurfaceIDs(t_ilm_int* pLength, t_ilm_surface** ppArray)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if ((pLength != NULL) && (ppArray != NULL)) {
        struct surface_context *ctx_surf = NULL;
        t_ilm_uint length = wl_list_length(&ctx->main_ctx.list_surface);
        *pLength = 0;

        *ppArray = (t_ilm_surface*)malloc(length * sizeof *ppArray);
        if (*ppArray != NULL) {
            t_ilm_surface* ids = *ppArray;
            wl_list_for_each_reverse(ctx_surf, &ctx->main_ctx.list_surface, link) {
                *ids = ctx_surf->id_surface;
                ids++;
            }
            *pLength = length;

            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_getSurfaceIDsOnLayer(t_ilm_layer layer,
                             t_ilm_int* pLength,
                             t_ilm_surface** ppArray)
{
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;
    t_ilm_uint length = 0;
    t_ilm_surface* ids = NULL;

    if ((pLength == NULL) || (ppArray == NULL)) {
        return ILM_FAILED;
    }

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layer);

    if (ctx_layer == NULL) {
        return ILM_FAILED;
    }

    length = wl_list_length(&ctx_layer->order.list_surface);
    *ppArray = (t_ilm_surface*)malloc(length * sizeof *ppArray);
    if (*ppArray == NULL) {
        return ILM_FAILED;
    }

    ids = *ppArray;
    wl_list_for_each_reverse(ctx_surf, &ctx_layer->order.list_surface, order.link) {
        *ids = (t_ilm_surface)ctx_surf->id_surface;
        ids++;
    }
    *pLength = length;

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

     wl_list_init(&ctx_layer->link);
     wl_list_insert(&ctx->list_layer, &ctx_layer->link);
     wl_list_init(&ctx_layer->order.link);
     wl_list_init(&ctx_layer->order.list_surface);

     ivi_controller_layer_add_listener(ctx_layer->controller,
                                   &controller_layer_listener, ctx_layer);

     return 0;
}

static ilmErrorTypes
wayland_layerCreateWithDimension(t_ilm_layer* pLayerId,
                                 t_ilm_uint width,
                                 t_ilm_uint height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    uint32_t layerid = 0;
    int32_t is_inside = 0;

    do {
        if (pLayerId == NULL) {
            break;
        }

        if (*pLayerId != INVALID_ID) {
            /* Return failed, if layerid is already inside list_layer */
            is_inside = wayland_controller_is_inside_layer_list(
                            &ctx->main_ctx.list_layer, *pLayerId);
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

        if (create_controller_layer(&ctx->main_ctx, width, height, layerid) == 0)
        {
           returnValue = ILM_SUCCESS;
        }
    } while(0);

    return returnValue;
}

static ilmErrorTypes
wayland_layerRemove(t_ilm_layer layerId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;
    struct layer_context *ctx_next = NULL;

    wl_list_for_each_safe(ctx_layer, ctx_next,
            &ctx->main_ctx.list_layer, link) {
        if (ctx_layer->id_layer == layerId) {
            ivi_controller_layer_destroy(ctx_layer->controller, 1);

            wl_list_remove(&ctx_layer->link);
            free(ctx_layer);

            returnValue = ILM_SUCCESS;
            break;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetType(t_ilm_layer layerId, ilmLayerType* pLayerType)
{
    if (!pLayerType)
    {
       return ILM_ERROR_INVALID_ARGUMENTS;
    }

    struct ilm_control_context *ctx = get_instance();

    *pLayerType = wayland_controller_is_inside_layer_list(&ctx->main_ctx.list_layer, layerId) ?
       ILM_LAYERTYPE_SOFTWARE2D :
       ILM_LAYERTYPE_UNKNOWN;

    return ILM_SUCCESS; // even if non existent?
}

static ilmErrorTypes
wayland_layerSetVisibility(t_ilm_layer layerId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        uint32_t visibility = 0;
        if (newVisibility == ILM_TRUE) {
            visibility = 1;
        }
        ivi_controller_layer_set_visibility(ctx_layer->controller,
                                            visibility);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetVisibility(t_ilm_layer layerId, t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pVisibility != NULL) {
        struct layer_context *ctx_layer = NULL;

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);

        if (ctx_layer != NULL) {
            *pVisibility = ctx_layer->prop.visibility;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetOpacity(t_ilm_layer layerId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        wl_fixed_t opacity_fixed = wl_fixed_from_double((double)opacity);
        ivi_controller_layer_set_opacity(ctx_layer->controller,
                                         opacity_fixed);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetOpacity(t_ilm_layer layerId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pOpacity != NULL) {
        struct layer_context *ctx_layer = NULL;

        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);

        if (ctx_layer != NULL) {
            *pOpacity = ctx_layer->prop.opacity;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetSourceRectangle(t_ilm_layer layerId,
                                t_ilm_uint x, t_ilm_uint y,
                                t_ilm_uint width, t_ilm_uint height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);

    if (ctx_layer != NULL) {
        ivi_controller_layer_set_source_rectangle(ctx_layer->controller,
                                                  (uint32_t)x,
                                                  (uint32_t)y,
                                                  (uint32_t)width,
                                                  (uint32_t)height);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetDestinationRectangle(t_ilm_layer layerId,
                                 t_ilm_int x, t_ilm_int y,
                                 t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);
    if (ctx_layer != NULL) {
        ivi_controller_layer_set_destination_rectangle(
                                         ctx_layer->controller,
                                         (uint32_t)x, (uint32_t)y,
                                         (uint32_t)width,
                                         (uint32_t)height);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetDimension(t_ilm_layer layerId, t_ilm_uint *pDimension)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pDimension != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            *pDimension = ctx_layer->prop.destWidth;
            *(pDimension + 1) = ctx_layer->prop.destHeight;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetDimension(t_ilm_layer layerId, t_ilm_uint *pDimension)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pDimension != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            ivi_controller_layer_set_destination_rectangle(
                ctx_layer->controller,
                ctx_layer->prop.destX, ctx_layer->prop.destY,
                (int32_t)*pDimension, (int32_t)*(pDimension + 1));
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetPosition(t_ilm_layer layerId, t_ilm_uint *pPosition)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pPosition != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            *pPosition = ctx_layer->prop.destX;
            *(pPosition + 1) = ctx_layer->prop.destY;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetPosition(t_ilm_layer layerId, t_ilm_uint *pPosition)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pPosition != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            ivi_controller_layer_set_destination_rectangle(
                ctx_layer->controller,
                (int32_t)*pPosition, (int32_t)*(pPosition + 1),
                ctx_layer->prop.destWidth, ctx_layer->prop.destHeight);
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetOrientation(t_ilm_layer layerId, ilmOrientation orientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
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
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer == NULL) {
            returnValue = ILM_FAILED;
            break;
        }

        ivi_controller_layer_set_orientation(ctx_layer->controller,
                                             iviorientation);

        returnValue = ILM_SUCCESS;
    } while(0);

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetOrientation(t_ilm_layer layerId, ilmOrientation *pOrientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    if (pOrientation != NULL) {
        ctx_layer = (struct layer_context*)
                    wayland_controller_get_layer_context(
                        &ctx->main_ctx, (uint32_t)layerId);
        if (ctx_layer != NULL) {
            *pOrientation = ctx_layer->prop.orientation;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerSetChromaKey(t_ilm_layer layerId, t_ilm_int* pColor)
{
    (void)layerId;
    (void)pColor;
    /* Not supported */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_layerSetRenderOrder(t_ilm_layer layerId,
                        t_ilm_surface *pSurfaceId,
                        t_ilm_int number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);
    if (ctx_layer) {
        int cnt = 0;
        uint32_t id = 0;
        struct surface_context *ctx_surf = NULL;
        ivi_controller_layer_clear_surfaces(ctx_layer->controller);

        for (cnt = 0; cnt < number; cnt++) {
            id = (uint32_t)*(pSurfaceId + cnt);
            ctx_surf = get_surface_context(&ctx->main_ctx, id);

            if (ctx_surf == NULL) {
                fprintf(stderr, "invalud argument \
                        in ilm_layerSetRenderOrder\n");
                continue;
            }
            ivi_controller_layer_add_surface(ctx_layer->controller,
                                             ctx_surf->controller);
        }

        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerGetCapabilities(t_ilm_layer layerId,
                         t_ilm_layercapabilities *pCapabilities)
{
    (void)layerId;
    (void)pCapabilities;
    /* Not supported */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_layerTypeGetCapabilities(ilmLayerType layerType,
                             t_ilm_layercapabilities *pCapabilities)
{
    (void)layerType;
    (void)pCapabilities;
    /* Not supported */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_surfaceSetVisibility(t_ilm_surface surfaceId, t_ilm_bool newVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;
    uint32_t visibility = 0;

    if (newVisibility == ILM_TRUE) {
        visibility = 1;
    }
    ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
    if (ctx_surf) {
        ivi_controller_surface_set_visibility(ctx_surf->controller,
                                              visibility);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetOpacity(t_ilm_surface surfaceId, t_ilm_float opacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;
    wl_fixed_t opacity_fixed = 0;

    opacity_fixed = wl_fixed_from_double((double)opacity);
    ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
    if (ctx_surf) {
        ivi_controller_surface_set_opacity(ctx_surf->controller,
                                           opacity_fixed);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetOpacity(t_ilm_surface surfaceId, t_ilm_float *pOpacity)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pOpacity != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->child_ctx, surfaceId);
        if (ctx_surf) {
            *pOpacity = ctx_surf->prop.opacity;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_SetKeyboardFocusOn(t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    (void)surfaceId;
    returnValue = ILM_SUCCESS;
    return returnValue;
}

static ilmErrorTypes
wayland_GetKeyboardFocusSurfaceId(t_ilm_surface* pSurfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    (void)pSurfaceId;
    returnValue = ILM_SUCCESS;
    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetDestinationRectangle(t_ilm_surface surfaceId,
                                   t_ilm_int x, t_ilm_int y,
                                   t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
    if (ctx_surf) {
        ivi_controller_surface_set_destination_rectangle(
                                             ctx_surf->controller,
                                             x, y, width, height);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetDimension(t_ilm_surface surfaceId, t_ilm_uint *pDimension)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pDimension != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
        if (ctx_surf) {
            uint32_t width = *pDimension;
            uint32_t height = *(pDimension + 1);
            ivi_controller_surface_set_destination_rectangle(
                ctx_surf->controller,
                ctx_surf->prop.destX, ctx_surf->prop.destY, width, height);
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetPosition(t_ilm_surface surfaceId, t_ilm_uint *pPosition)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pPosition != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->child_ctx, surfaceId);
        if (ctx_surf) {
            *pPosition = ctx_surf->prop.destX;
            *(pPosition + 1) = ctx_surf->prop.destY;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetPosition(t_ilm_surface surfaceId, t_ilm_uint *pPosition)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pPosition != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
        if (ctx_surf) {
            int32_t destX = (int32_t)*pPosition;
            int32_t destY = (int32_t)*(pPosition + 1);
            ivi_controller_surface_set_destination_rectangle(
                ctx_surf->controller, destX, destY,
                ctx_surf->prop.destWidth, ctx_surf->prop.destHeight);
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetOrientation(t_ilm_surface surfaceId,
                              ilmOrientation orientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
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

        ctx_surf = get_surface_context(&ctx->main_ctx, surfaceId);
        if (ctx_surf == NULL) {
            returnValue = ILM_FAILED;
            break;
        }

        ivi_controller_surface_set_orientation(ctx_surf->controller,
                                               iviorientation);

        returnValue = ILM_SUCCESS;
    } while(0);

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetOrientation(t_ilm_surface surfaceId,
                              ilmOrientation *pOrientation)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pOrientation != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->child_ctx, surfaceId);
        if (ctx_surf) {
            *pOrientation = ctx_surf->prop.orientation;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetPixelformat(t_ilm_layer surfaceId,
                              ilmPixelFormat *pPixelformat)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pPixelformat != NULL) {
        struct surface_context *ctx_surf = NULL;
        ctx_surf = get_surface_context(&ctx->child_ctx, surfaceId);
        if (ctx_surf) {
            *pPixelformat = ctx_surf->prop.pixelformat;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetChromaKey(t_ilm_surface surfaceId, t_ilm_int* pColor)
{
    (void)surfaceId;
    (void)pColor;
    /* Not supported */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_displaySetRenderOrder(t_ilm_display display,
                          t_ilm_layer *pLayerId, const t_ilm_uint number)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct screen_context *ctx_scrn = NULL;

    ctx_scrn = get_screen_context_by_id(&ctx->main_ctx, (uint32_t)display);
    if (ctx_scrn != NULL) {
        int cnt = 0;
        uint32_t id = 0;
        struct layer_context *ctx_layer = NULL;

        ivi_controller_screen_clear(ctx_scrn->controller);

        for (cnt = 0; cnt < (int)number; cnt++) {
            id = (uint32_t)*(pLayerId + cnt);
            ctx_layer =
                 (struct layer_context*)wayland_controller_get_layer_context(
                            &ctx->main_ctx, id);
            if (ctx_layer != NULL) {
                ivi_controller_screen_add_layer(ctx_scrn->controller,
                                                ctx_layer->controller);
            }
        }

        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_takeScreenshot(t_ilm_uint screen, t_ilm_const_string filename)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct screen_context *ctx_scrn = NULL;

    ctx_scrn = get_screen_context_by_id(&ctx->main_ctx, (uint32_t)screen);
    if (ctx_scrn != NULL) {
        ivi_controller_screen_screenshot(ctx_scrn->controller,
                                        filename);
        wl_display_flush(ctx->main_ctx.display);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_takeLayerScreenshot(t_ilm_const_string filename, t_ilm_layer layerid)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerid);
    if (ctx_layer != NULL) {
        ivi_controller_layer_screenshot(ctx_layer->controller,
                                        filename);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_takeSurfaceScreenshot(t_ilm_const_string filename,
                              t_ilm_surface surfaceid)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->main_ctx, (uint32_t)surfaceid);
    if (ctx_surf) {
        ivi_controller_surface_screenshot(ctx_surf->controller,
                                          filename);
        wl_display_flush(ctx->main_ctx.display);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_SetOptimizationMode(ilmOptimization id, ilmOptimizationMode mode)
{
    (void)id;
    (void)mode;
    /* Not supported */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_GetOptimizationMode(ilmOptimization id, ilmOptimizationMode* pMode)
{
    (void)id;
    (void)pMode;
    /* Not supported */
    return ILM_FAILED;
}

// TODO
static ilmErrorTypes
wayland_layerAddNotification(t_ilm_layer layer,
                             layerNotificationFunc callback)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->child_ctx, (uint32_t)layer);
    if (ctx_layer == NULL) {
        returnValue = ILM_ERROR_INVALID_ARGUMENTS;
    } else {
        ctx_layer->notification = callback;

        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

// TODO
static ilmErrorTypes
wayland_layerRemoveNotification(t_ilm_layer layer)
{
   return wayland_layerAddNotification(layer, NULL);
}

// TODO
static ilmErrorTypes
wayland_surfaceAddNotification(t_ilm_surface surface,
                             surfaceNotificationFunc callback)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = (struct surface_context*)get_surface_context(
                    &ctx->child_ctx, (uint32_t)surface);
    if (ctx_surf == NULL) {
        returnValue = ILM_ERROR_INVALID_ARGUMENTS;
    } else {
        ctx_surf->notification = callback;

        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

// TODO
static ilmErrorTypes
wayland_surfaceRemoveNotification(t_ilm_surface surface)
{
    return wayland_surfaceAddNotification(surface, NULL);
}

static ilmErrorTypes
wayland_getNativeHandle(t_ilm_uint pid, t_ilm_int *n_handle,
                        t_ilm_nativehandle **p_handles)
{
    struct ilm_control_context *ctx = get_instance();
    struct nativehandle_context *p_nh_ctx = NULL;

    *n_handle = 0;
    *p_handles = NULL;

    wl_list_for_each(p_nh_ctx, &ctx->list_nativehandle, link)
    {
        if (p_nh_ctx->pid == pid)
        {
            *n_handle = 1;
            *p_handles =
                 (t_ilm_nativehandle*)malloc(sizeof(t_ilm_nativehandle));
            (*p_handles)[0] = p_nh_ctx->nativehandle;
            break;
        }
    }

    return (*n_handle > 0) ? ILM_SUCCESS : ILM_FAILED;
}

static ilmErrorTypes
wayland_getPropertiesOfSurface(t_ilm_uint surfaceID,
                        struct ilmSurfaceProperties* pSurfaceProperties)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pSurfaceProperties != NULL) {
        struct surface_context *ctx_surf = NULL;

        ctx_surf = get_surface_context(&ctx->child_ctx, (uint32_t)surfaceID);
        if (ctx_surf != NULL) {

            *pSurfaceProperties = ctx_surf->prop;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerAddSurface(t_ilm_layer layerId,
                        t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);
    ctx_surf = get_surface_context(&ctx->main_ctx, (uint32_t)surfaceId);
    if ((ctx_layer != NULL) && (ctx_surf != NULL)) {
        ivi_controller_layer_add_surface(ctx_layer->controller,
                                         ctx_surf->controller);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_layerRemoveSurface(t_ilm_layer layerId,
                           t_ilm_surface surfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct layer_context *ctx_layer = NULL;
    struct surface_context *ctx_surf = NULL;

    ctx_layer = (struct layer_context*)wayland_controller_get_layer_context(
                    &ctx->main_ctx, (uint32_t)layerId);
    ctx_surf = get_surface_context(&ctx->main_ctx, (uint32_t)surfaceId);
    if ((ctx_layer != NULL) && (ctx_surf != NULL)) {
        ivi_controller_layer_remove_surface(ctx_layer->controller,
                                            ctx_surf->controller);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetDimension(t_ilm_surface surfaceId,
                            t_ilm_uint *pDimension)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (pDimension != NULL) {
        struct surface_context *ctx_surf = NULL;

        ctx_surf = get_surface_context(&ctx->child_ctx, (uint32_t)surfaceId);
        if (ctx_surf != NULL) {
            *pDimension = (t_ilm_uint)ctx_surf->prop.destWidth;
            *(pDimension + 1) = (t_ilm_uint)ctx_surf->prop.destHeight;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceGetVisibility(t_ilm_surface surfaceId,
                             t_ilm_bool *pVisibility)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;

    if (pVisibility != NULL) {
        ctx_surf = get_surface_context(&ctx->child_ctx, (uint32_t)surfaceId);
        if (ctx_surf != NULL) {
            *pVisibility = (t_ilm_bool)ctx_surf->prop.visibility;
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceSetSourceRectangle(t_ilm_surface surfaceId,
                                  t_ilm_int x, t_ilm_int y,
                                  t_ilm_int width, t_ilm_int height)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context(&ctx->main_ctx, (uint32_t)surfaceId);
    if (ctx_surf != NULL) {
        if (ctx_surf->controller != NULL) {
            ivi_controller_surface_set_source_rectangle(
                    ctx_surf->controller,
                    x, y, width, height);
            returnValue = ILM_SUCCESS;
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_commitChanges(void)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx = get_instance();

    if (ctx->main_ctx.controller != NULL) {
        ivi_controller_commit_changes(ctx->main_ctx.controller);

        wl_display_roundtrip(ctx->main_ctx.display);
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}
