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

#ifndef IVI_INPUT_CONTROLLER_BASE_CLASS
#define IVI_INPUT_CONTROLLER_BASE_CLASS
#include <string>
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"
#include "ilm_types.h"

extern "C"{
    WL_EXPORT int input_controller_module_init(struct ivishell *shell);
}

/**
 * @note struct input_context is in ivi-input-controller.c
 * @note any changes from the original file should be updated here
*/
struct input_context {
    struct wl_list resource_list;
    struct wl_list seat_list;
    int successful_init_stage;
    struct ivishell *ivishell;

    struct wl_listener surface_created;
    struct wl_listener surface_destroyed;
    struct wl_listener compositor_destroy_listener;
    struct wl_listener seat_create_listener;
};

/**
 * @note struct seat_ctx is in ivi-input-controller.c
 * @note any changes from the original file should be updated here
*/
struct seat_ctx {
    struct input_context *input_ctx;
    struct weston_keyboard_grab keyboard_grab;
    struct weston_pointer_grab pointer_grab;
    struct weston_touch_grab touch_grab;
    struct weston_seat *west_seat;

    /* pointer focus can be forced to specific surfaces
     * when there are no motion events at all. motion
     * event will re-evaulate the focus. A rotary knob
     * is one of the examples, where it is used as pointer
     * axis.*/
    struct ivisurface *forced_ptr_focus_surf;
    int32_t  forced_surf_enabled;

    struct wl_listener updated_caps_listener;
    struct wl_listener destroy_listener;
    struct wl_list seat_node;
};

/**
 * @note struct seat_focus is in ivi-input-controller.c
 * @note any changes from the original file should be updated here
*/
struct seat_focus {
    struct seat_ctx *seat_ctx;
    ilmInputDevice focus;
    struct wl_list link;
};

/**
 * \brief: InputControllerBase will help to show all callback functions in ivi-input-modules, there are 25 callbacks
 */
class InputControllerBase
{
public:
    InputControllerBase() {}
    virtual ~InputControllerBase() {};
    virtual bool initBaseModule();
    void keyboard_grab_key(struct weston_keyboard_grab *grab, const struct timespec *time, uint32_t key, uint32_t state);
    void keyboard_grab_modifiers(struct weston_keyboard_grab *grab, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    void keyboard_grab_cancel(struct weston_keyboard_grab *grab);
    void pointer_grab_focus(struct weston_pointer_grab *grab);
    void pointer_grab_motion(struct weston_pointer_grab *grab, const struct timespec *time, struct weston_pointer_motion_event *event);
    void pointer_grab_button(struct weston_pointer_grab *grab, const struct timespec *time, uint32_t button, uint32_t state);
    void pointer_grab_axis(struct weston_pointer_grab *grab, const struct timespec *time, struct weston_pointer_axis_event *event);
    void pointer_grab_axis_source(struct weston_pointer_grab *grab, uint32_t source);
    void pointer_grab_frame(struct weston_pointer_grab *grab);
    void pointer_grab_cancel(struct weston_pointer_grab *grab);
    void touch_grab_down(struct weston_touch_grab *grab, const struct timespec *time, int touch_id, wl_fixed_t sx, wl_fixed_t sy);
    void touch_grab_up(struct weston_touch_grab *grab, const struct timespec *time, int touch_id);
    void touch_grab_motion(struct weston_touch_grab *grab, const struct timespec *time, int touch_id, wl_fixed_t sx, wl_fixed_t sy);
    void touch_grab_frame(struct weston_touch_grab *grab);
    void touch_grab_cancel(struct weston_touch_grab *grab);
    void set_input_focus(struct wl_client *client, struct wl_resource *resource, uint32_t surface, uint32_t device, int32_t enabled);
    void set_input_acceptance(struct wl_client *client, struct wl_resource *resource, uint32_t surface, const char *seat, int32_t accepted);
    void bind_ivi_input(struct wl_client *client, void *data, uint32_t version, uint32_t id);
    void unbind_resource_controller(struct wl_resource *resource);
    void input_controller_destroy(struct wl_listener *listener, void *data);
    void handle_surface_create(struct wl_listener *listener, void *data);
    void handle_surface_destroy(struct wl_listener *listener, void *data);
    void handle_seat_create(struct wl_listener *listener, void *data);
    void handle_seat_destroy(struct wl_listener *listener, void *data);
    void handle_seat_updated_caps(struct wl_listener *listener, void *data);
};

#endif //IVI_INPUT_CONTROLLER_BASE_CLASS