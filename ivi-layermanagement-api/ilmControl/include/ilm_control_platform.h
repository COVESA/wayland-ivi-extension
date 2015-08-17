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
#ifndef _ILM_CONTROL_PLATFORM_H_
#define _ILM_CONTROL_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pthread.h>
#include <stdbool.h>

#include "ilm_common.h"
#include "wayland-util.h"

typedef struct _ILM_CONTROL_PLATFORM_FUNC
{
    ilmErrorTypes (*getPropertiesOfLayer)(t_ilm_uint layerID,
                   struct ilmLayerProperties* pLayerProperties);
    ilmErrorTypes (*getPropertiesOfScreen)(t_ilm_display screenID,
                   struct ilmScreenProperties* pScreenProperties);
    ilmErrorTypes (*getScreenIDs)(t_ilm_uint* pNumberOfIDs,
                   t_ilm_uint** ppIDs);
    ilmErrorTypes (*getLayerIDs)(t_ilm_int* pLength,
                   t_ilm_layer** ppArray);
    ilmErrorTypes (*getLayerIDsOnScreen)(t_ilm_uint screenId,
                   t_ilm_int* pLength, t_ilm_layer** ppArray);
    ilmErrorTypes (*getSurfaceIDs)(t_ilm_int* pLength,
                   t_ilm_surface** ppArray);
    ilmErrorTypes (*getSurfaceIDsOnLayer)(t_ilm_layer layer,
                   t_ilm_int* pLength, t_ilm_surface** ppArray);
    ilmErrorTypes (*layerCreateWithDimension)(t_ilm_layer* pLayerId,
                   t_ilm_uint width, t_ilm_uint height);
    ilmErrorTypes (*layerRemove)(t_ilm_layer layerId);
    ilmErrorTypes (*layerSetVisibility)(t_ilm_layer layerId,
                   t_ilm_bool newVisibility);
    ilmErrorTypes (*layerGetVisibility)(t_ilm_layer layerId,
                   t_ilm_bool *pVisibility);
    ilmErrorTypes (*layerSetOpacity)(t_ilm_layer layerId,
                   t_ilm_float opacity);
    ilmErrorTypes (*layerGetOpacity)(t_ilm_layer layerId,
                   t_ilm_float *pOpacity);
    ilmErrorTypes (*layerSetSourceRectangle)(t_ilm_layer layerId,
                   t_ilm_uint x, t_ilm_uint y,
                   t_ilm_uint width, t_ilm_uint height);
    ilmErrorTypes (*layerSetDestinationRectangle)(t_ilm_layer layerId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*layerSetOrientation)(t_ilm_layer layerId,
                   ilmOrientation orientation);
    ilmErrorTypes (*layerGetOrientation)(t_ilm_layer layerId,
                   ilmOrientation *pOrientation);
    ilmErrorTypes (*layerSetRenderOrder)(t_ilm_layer layerId,
                   t_ilm_layer *pSurfaceId, t_ilm_int number);
    ilmErrorTypes (*surfaceSetVisibility)(t_ilm_surface surfaceId,
                   t_ilm_bool newVisibility);
    ilmErrorTypes (*surfaceSetOpacity)(t_ilm_surface surfaceId,
                   t_ilm_float opacity);
    ilmErrorTypes (*surfaceGetOpacity)(t_ilm_surface surfaceId,
                   t_ilm_float *pOpacity);
    ilmErrorTypes (*surfaceSetDestinationRectangle)(t_ilm_surface surfaceId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*surfaceSetOrientation)(t_ilm_surface surfaceId,
                   ilmOrientation orientation);
    ilmErrorTypes (*surfaceGetOrientation)(t_ilm_surface surfaceId,
                   ilmOrientation *pOrientation);
    ilmErrorTypes (*surfaceGetPixelformat)(t_ilm_layer surfaceId,
                   ilmPixelFormat *pPixelformat);
    ilmErrorTypes (*displaySetRenderOrder)(t_ilm_display display,
                   t_ilm_layer *pLayerId, const t_ilm_uint number);
    ilmErrorTypes (*takeScreenshot)(t_ilm_uint screen,
                   t_ilm_const_string filename);
    ilmErrorTypes (*takeLayerScreenshot)(t_ilm_const_string filename,
                   t_ilm_layer layerid);
    ilmErrorTypes (*takeSurfaceScreenshot)(t_ilm_const_string filename,
                   t_ilm_surface surfaceid);
    ilmErrorTypes (*layerAddNotification)(t_ilm_layer layer,
                   layerNotificationFunc callback);
    ilmErrorTypes (*layerRemoveNotification)(t_ilm_layer layer);
    ilmErrorTypes (*surfaceAddNotification)(t_ilm_surface surface,
                   surfaceNotificationFunc callback);
    ilmErrorTypes (*surfaceRemoveNotification)(t_ilm_surface surface);
    ilmErrorTypes (*init)(t_ilm_nativedisplay nativedisplay);
    void (*destroy)();
    ilmErrorTypes (*getPropertiesOfSurface)(t_ilm_uint surfaceID,
                   struct ilmSurfaceProperties* pSurfaceProperties);
    ilmErrorTypes (*layerAddSurface)(t_ilm_layer layerId,
                   t_ilm_surface surfaceId);
    ilmErrorTypes (*layerRemoveSurface)(t_ilm_layer layerId,
                   t_ilm_surface surfaceId);
    ilmErrorTypes (*surfaceGetVisibility)(t_ilm_surface surfaceId,
                   t_ilm_bool *pVisibility);
    ilmErrorTypes (*surfaceSetSourceRectangle)(t_ilm_surface surfaceId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*commitChanges)();
} ILM_CONTROL_PLATFORM_FUNC;

ILM_CONTROL_PLATFORM_FUNC gIlmControlPlatformFunc;

void init_ilmControlPlatformTable();

struct wayland_context {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_event_queue *queue;
    struct wl_compositor *compositor;
    struct ivi_controller *controller;
    uint32_t num_screen;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;
    struct wl_list list_seat;
    notificationFunc notification;
    void *notification_user_data;

    struct ivi_input *input_controller;
};

struct ilm_control_context {
    struct wayland_context wl;
    bool initialized;

    uint32_t internal_id_layer;

    pthread_t thread;
    pthread_mutex_t mutex;
    int shutdown_fd;
    uint32_t internal_id_surface;
};

struct seat_context {
    struct wl_list link;
    char *seat_name;
    ilmInputDevice capabilities;
};

struct accepted_seat {
    struct wl_list link;
    char *seat_name;
};

struct surface_context {
    struct wl_list link;

    struct ivi_surface *surface;
    struct ivi_controller_surface *controller;

    t_ilm_uint id_surface;
    struct ilmSurfaceProperties prop;
    struct wl_list list_accepted_seats;
    surfaceNotificationFunc notification;

    struct {
        struct wl_list link;
    } order;

    struct wayland_context *ctx;
    bool is_surface_creation_noticed;
};

ilmErrorTypes impl_sync_and_acquire_instance(struct ilm_control_context *ctx);

void release_instance(void);

#define sync_and_acquire_instance() ({ \
    struct ilm_control_context *ctx = &ilm_context; \
    { \
        ilmErrorTypes status = impl_sync_and_acquire_instance(ctx); \
        if (status != ILM_SUCCESS) { \
            return status; \
        } \
    } \
    ctx; \
})

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CONTROL_PLATFORM_H_ */
