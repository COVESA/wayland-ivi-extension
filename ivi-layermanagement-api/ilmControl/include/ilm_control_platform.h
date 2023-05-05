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

struct wayland_context {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_event_queue *queue;
    struct wl_compositor *compositor;

    struct ivi_wm *controller;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;
    struct wl_list list_seat;
    notificationFunc notification;
    void *notification_user_data;

    ilmErrorTypes error_flag;

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

    shutdownNotificationFunc notification;
    void *notification_user_data;
};

struct seat_context {
    struct wl_list link;
    char *seat_name;
    bool is_default;
    ilmInputDevice capabilities;
};

struct accepted_seat {
    struct wl_list link;
    char *seat_name;
};

struct surface_context {
    struct wl_list link;

    t_ilm_uint id_surface;
    struct ilmSurfaceProperties prop;
    struct wl_list list_accepted_seats;
    surfaceNotificationFunc notification;

    struct wayland_context *ctx;
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
