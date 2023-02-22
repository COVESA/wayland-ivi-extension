/***************************************************************************
 *
 * Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
 *
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

#ifndef ILM_CONTROL_BASE_CLASS
#define ILM_CONTROL_BASE_CLASS

#include "client_api_fake.h"
#include "ivi-wm-client-protocol.h"
#include "ivi-input-client-protocol.h"
#include "ilm_control_platform.h"

/**
 * @note struct layer_context is in ilm_control_wayland_platform.c
 * @note any changes from the original file should be updated here
*/
struct layer_context {
    struct wl_list link;
    t_ilm_uint id_layer;
    struct ilmLayerProperties prop;
    layerNotificationFunc notification;
    struct wl_array render_order;
    struct wayland_context *ctx;
};

/** @note struct screen_context is in ilm_control_wayland_platform.c
 * @note any changes from the original file should be updated here
*/
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

/**
 * \brief: IlmControlInitBase will help to show all callback functions in ilmControl, there are 34 callbacks
 */
class IlmControlInitBase
{
public:
    IlmControlInitBase() {}
    virtual ~IlmControlInitBase() {}
    virtual bool initBaseModule();
    void wm_screen_listener_screen_id(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t id);
    void wm_screen_listener_layer_added(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t layer_id);
    void wm_screen_listener_connector_name(void *data, struct ivi_wm_screen *ivi_wm_screen, const char *process_name);
    void wm_screen_listener_error(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t error, const char *message);
    void wm_listener_surface_visibility(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t visibility);
    void wm_listener_layer_visibility(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t visibility);
    void wm_listener_surface_opacity(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, wl_fixed_t opacity);
    void wm_listener_layer_opacity(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, wl_fixed_t opacity);
    void wm_listener_surface_source_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void wm_listener_layer_source_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void wm_listener_surface_destination_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void wm_listener_layer_destination_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void wm_listener_surface_created(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id);
    void wm_listener_layer_created(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id);
    void wm_listener_surface_destroyed(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id);
    void wm_listener_layer_destroyed(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id);
    void wm_listener_surface_error(void *data, struct ivi_wm *ivi_wm, uint32_t object_id, uint32_t error, const char *message);
    void wm_listener_layer_error(void *data, struct ivi_wm *ivi_wm, uint32_t object_id, uint32_t error, const char *message);
    void wm_listener_surface_size(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t width, int32_t height);
    void wm_listener_surface_stats(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, uint32_t frame_count, uint32_t pid);
    void wm_listener_layer_surface_added(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, uint32_t surface_id);
    void input_listener_seat_created(void *data, struct ivi_input *ivi_input, const char *name, uint32_t capabilities);
    void input_listener_seat_capabilities(void *data, struct ivi_input *ivi_input, const char *name, uint32_t capabilities);
    void input_listener_seat_destroyed(void *data, struct ivi_input *ivi_input, const char *name);
    void input_listener_input_focus(void *data, struct ivi_input *ivi_input, uint32_t surface, uint32_t device, int32_t enabled);
    void input_listener_input_acceptance(void *data, struct ivi_input *ivi_input, uint32_t surface, const char *seat, int32_t accepted);
    void output_listener_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform);
    void output_listener_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    void output_listener_done(void *data, struct wl_output *wl_output);
    void output_listener_scale(void *data, struct wl_output *wl_output, int32_t factor);
    void registry_handle_control(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);
    void registry_handle_control_remove(void *data, struct wl_registry *wl_registry, uint32_t name);
    void screenshot_done(void *data, struct ivi_screenshot *ivi_screenshot, int32_t fd, int32_t width, int32_t height, int32_t stride, uint32_t format, uint32_t timestamp);
    void screenshot_error(void *data, struct ivi_screenshot *ivi_screenshot, uint32_t error, const char *message);
};
#endif  // ILM_CONTROL_BASE_CLASS
