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

#ifndef IVI_CONTROLLER_BASE_CLASS
#define IVI_CONTROLLER_BASE_CLASS

#include <dlfcn.h>
#include <string>
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"

extern "C"
{
#include "ivi-controller.h"
}

/**
 * @note struct ivicontroller is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct ivicontroller {
    struct wl_resource *resource;
    uint32_t id;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;

    struct wl_list layer_notifications;
    struct wl_list surface_notifications;
};

/**
 * @note struct iviscreen is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct iviscreen {
    struct wl_list link;
    struct ivishell *shell;
    uint32_t id_screen;
    struct weston_output *output;
    struct wl_list resource_list;
};

/**
 * @note struct screenshot_frame_listener is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct screenshot_frame_listener {
    struct wl_listener frame_listener;
    struct wl_listener output_destroyed;
    struct wl_resource *screenshot;
    struct weston_output *output;
};

/**
 * @note struct ivilayer is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct ivilayer {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_layer *layout_layer;
    const struct ivi_layout_layer_properties *prop;
    struct wl_listener property_changed;
    struct wl_list notification_list;
};

/**
 * @note struct screen_id_info is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct screen_id_info {
    char *screen_name;
    uint32_t screen_id;
};

/**
 * @note struct notification is in ivi-controller.c
 * @note any changes from the original file should be updated here
*/
struct notification {
    struct wl_list link;
    struct wl_resource *resource;
    struct wl_list layout_link;
};

/**
 * \brief: Controllerbase will help to show all callback functions in weston-ivi-shell, there are 47 callbacks
 */
class ControllerBase
{
public:
    ControllerBase() {};
    virtual ~ControllerBase() {};
    virtual bool initBaseModule();
    void controller_screen_destroy(struct wl_client *client, struct wl_resource *resource);
    void controller_screen_clear(struct wl_client *client, struct wl_resource *resource);
    void controller_screen_add_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id);
    void controller_screen_remove_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id);
    void controller_screen_screenshot(struct wl_client *client, struct wl_resource *resource,uint32_t screenshot);
    void controller_screen_get(struct wl_client *client, struct wl_resource *resource, int32_t param);
    void controller_commit_changes(struct wl_client *client, struct wl_resource *resource);
    void controller_create_screen(struct wl_client *client,struct wl_resource *resource, struct wl_resource *output, uint32_t id);
    void controller_set_surface_visibility(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, uint32_t visibility);
    void controller_set_layer_visibility(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, uint32_t visibility);
    void controller_set_surface_opacity(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id,wl_fixed_t opacity);
    void controller_set_layer_opacity(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, wl_fixed_t opacity);
    void controller_set_surface_source_rectangle(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void controller_set_layer_source_rectangle(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height);
    void controller_set_surface_destination_rectangle(struct wl_client *client, struct wl_resource *resource,uint32_t surface_id,int32_t x,int32_t y,int32_t width,int32_t height);
    void controller_set_layer_destination_rectangle(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t x,int32_t y,int32_t width,int32_t height);
    void controller_surface_sync(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t sync_state);
    void controller_layer_sync(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, int32_t sync_state);
    void controller_surface_get(struct wl_client *client,struct wl_resource *resource,uint32_t surface_id,int32_t param);
    void controller_layer_get(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t param);
    void controller_surface_screenshot(struct wl_client *client, struct wl_resource *resource, uint32_t screenshot, uint32_t surface_id);
    void controller_set_surface_type(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t type);
    void controller_layer_clear(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id);
    void controller_layer_add_surface(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,uint32_t surface_id);
    void controller_layer_remove_surface(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, uint32_t surface_id);
    void controller_create_layout_layer(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t width,int32_t height);
    void controller_destroy_layout_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id);
    void send_surface_prop(struct wl_listener *listener, void *data);
    void send_layer_prop(struct wl_listener *listener, void *data);
    void output_destroyed_event(struct wl_listener *listener, void *data);
    void output_resized_event(struct wl_listener *listener, void *data);
    void output_created_event(struct wl_listener *listener, void *data);
    void layer_event_create(struct wl_listener *listener, void *data);
    void layer_event_remove(struct wl_listener *listener, void *data);
    void surface_event_create(struct wl_listener *listener, void *data);
    void surface_event_remove(struct wl_listener *listener, void *data);
    void surface_event_configure(struct wl_listener *listener, void *data);
    void ivi_shell_destroy(struct wl_listener *listener, void *data);
    void surface_committed(struct wl_listener *listener, void *data);
    void controller_screenshot_notify(struct wl_listener *listener, void *data);
    void ivi_shell_client_destroy(struct wl_listener *listener, void *data);
    void screenshot_output_destroyed(struct wl_listener *listener, void *data);
    void launch_client_process(void *data);
    void bind_ivi_controller(struct wl_client *client, void *data, uint32_t version, uint32_t id);
    void unbind_resource_controller(struct wl_resource *resource);
    void screenshot_frame_listener_destroy(struct wl_resource *resource);
    void destroy_ivicontroller_screen(struct wl_resource *resource);
};

#endif //IVI_CONTROLLER_BASE_CLASS