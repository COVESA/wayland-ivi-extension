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

#ifndef SERVER_API_FAKE
#define SERVER_API_FAKE

#include "fff.h"
#include "ivi-wm-server-protocol.h"
#include "ivi-layout-export.h"
#include "libweston-desktop/libweston-desktop.h"
#include "weston.h"
#include "common_fake_api.h"

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(struct weston_config_section *, weston_config_get_section, struct weston_config *, const char *, const char *, const char *);
DECLARE_FAKE_VALUE_FUNC(const void *, weston_plugin_api_get, struct weston_compositor *, const char *, size_t );
DECLARE_FAKE_VALUE_FUNC(void *, wet_load_module_entrypoint, const char *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, weston_config_section_get_bool, struct weston_config_section *, const char *, bool *, bool );
DECLARE_FAKE_VALUE_FUNC(int, weston_config_section_get_string, struct weston_config_section *, const char *, char **, const char *);
DECLARE_FAKE_VALUE_FUNC(struct weston_view *, weston_view_create, struct weston_surface *);
DECLARE_FAKE_VALUE_FUNC(struct wl_client *, weston_client_start, struct weston_compositor *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, weston_config_section_get_color, struct weston_config_section *, const char *, uint32_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, weston_config_next_section, struct weston_config *, struct weston_config_section **, const char **);
DECLARE_FAKE_VALUE_FUNC(int, weston_config_section_get_int, struct weston_config_section *, const char *, int32_t *, int32_t);
DECLARE_FAKE_VALUE_FUNC(struct weston_config *, wet_get_config, struct weston_compositor *);
DECLARE_FAKE_VALUE_FUNC(int, weston_config_section_get_uint, struct weston_config_section *, const char *, uint32_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(struct weston_view *, weston_compositor_pick_view, struct weston_compositor *, wl_fixed_t, wl_fixed_t, wl_fixed_t *, wl_fixed_t *);
DECLARE_FAKE_VALUE_FUNC(const char *, weston_desktop_surface_get_app_id, struct weston_desktop_surface *);
DECLARE_FAKE_VALUE_FUNC(const char *, weston_desktop_surface_get_title, struct weston_desktop_surface *);
DECLARE_FAKE_VALUE_FUNC(struct weston_desktop_surface *, weston_surface_get_desktop_surface, struct weston_surface *);
DECLARE_FAKE_VALUE_FUNC(struct weston_keyboard *, weston_seat_get_keyboard, struct weston_seat *);
DECLARE_FAKE_VALUE_FUNC(struct weston_pointer *, weston_seat_get_pointer, struct weston_seat *);
DECLARE_FAKE_VALUE_FUNC(struct weston_touch *, weston_seat_get_touch, struct weston_seat *);
DECLARE_FAKE_VALUE_FUNC(struct weston_surface *, weston_surface_get_main_surface, struct weston_surface *);
DECLARE_FAKE_VALUE_FUNC(bool, weston_touch_has_focus_resource, struct weston_touch *);
DECLARE_FAKE_VALUE_FUNC(struct wl_event_loop *, wl_display_get_event_loop, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(uint32_t, wl_display_next_serial, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(struct wl_event_source *, wl_event_loop_add_idle, struct wl_event_loop *, wl_event_loop_idle_func_t , void *);
DECLARE_FAKE_VALUE_FUNC(struct wl_global *, wl_global_create, struct wl_display *, const struct wl_interface *, int ,  void *, wl_global_bind_func_t);
DECLARE_FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int , uint32_t );
// DECLARE_FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_from_link, struct wl_list *);
DECLARE_FAKE_VALUE_FUNC(struct wl_client *, wl_resource_get_client, struct wl_resource *);
// DECLARE_FAKE_VALUE_FUNC(struct wl_list *, wl_resource_get_link, struct wl_resource *);
DECLARE_FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *);
DECLARE_FAKE_VALUE_FUNC(int, wl_resource_get_version, struct wl_resource *);
DECLARE_FAKE_VOID_FUNC(weston_layer_entry_remove, struct weston_layer_entry *);
DECLARE_FAKE_VOID_FUNC(weston_layer_set_position, struct weston_layer *, enum weston_layer_position );
DECLARE_FAKE_VOID_FUNC(weston_view_destroy, struct weston_view *);
DECLARE_FAKE_VOID_FUNC(weston_matrix_scale, struct weston_matrix *, float, float, float);
DECLARE_FAKE_VOID_FUNC(weston_matrix_init, struct weston_matrix *);
DECLARE_FAKE_VOID_FUNC(weston_surface_schedule_repaint, struct weston_surface *);
DECLARE_FAKE_VOID_FUNC(weston_surface_set_color, struct weston_surface *, float, float, float , float);
DECLARE_FAKE_VOID_FUNC(weston_compositor_schedule_repaint, struct weston_compositor *);
DECLARE_FAKE_VOID_FUNC(weston_output_damage, struct weston_output *);
DECLARE_FAKE_VOID_FUNC(weston_view_update_transform, struct weston_view *);
DECLARE_FAKE_VOID_FUNC(weston_matrix_translate, struct weston_matrix *, float, float, float);
DECLARE_FAKE_VOID_FUNC(weston_compositor_read_presentation_clock, const struct weston_compositor *, struct timespec *);
DECLARE_FAKE_VOID_FUNC(weston_layer_init, struct weston_layer *, struct weston_compositor *);
DECLARE_FAKE_VOID_FUNC(weston_layer_entry_insert, struct weston_layer_entry *, struct weston_layer_entry *);
DECLARE_FAKE_VOID_FUNC(weston_keyboard_send_keymap, struct weston_keyboard *, struct wl_resource *);
DECLARE_FAKE_VOID_FUNC(weston_keyboard_start_grab, struct weston_keyboard *, struct weston_keyboard_grab *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_clear_focus, struct weston_pointer *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_send_axis, struct weston_pointer *, const struct timespec *, struct weston_pointer_axis_event *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_send_axis_source, struct weston_pointer *, uint32_t);
DECLARE_FAKE_VOID_FUNC(weston_pointer_send_button, struct weston_pointer *, const struct timespec *, uint32_t, uint32_t);
DECLARE_FAKE_VOID_FUNC(weston_pointer_send_frame, struct weston_pointer *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_send_motion, struct weston_pointer *, const struct timespec *, struct weston_pointer_motion_event *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_set_focus, struct weston_pointer *, struct weston_view *, wl_fixed_t, wl_fixed_t);
DECLARE_FAKE_VOID_FUNC(weston_pointer_start_grab, struct weston_pointer *, struct weston_pointer_grab *);
DECLARE_FAKE_VOID_FUNC(weston_touch_send_down, struct weston_touch *, const struct timespec *, int , wl_fixed_t , wl_fixed_t );
DECLARE_FAKE_VOID_FUNC(weston_touch_send_frame, struct weston_touch *);
DECLARE_FAKE_VOID_FUNC(weston_touch_send_motion, struct weston_touch *, const struct timespec *, int , wl_fixed_t , wl_fixed_t );
DECLARE_FAKE_VOID_FUNC(weston_touch_send_up, struct weston_touch *, const struct timespec *, int );
DECLARE_FAKE_VOID_FUNC(weston_touch_set_focus, struct weston_touch *, struct weston_view *);
DECLARE_FAKE_VOID_FUNC(weston_touch_start_grab, struct weston_touch *, struct weston_touch_grab *);
DECLARE_FAKE_VOID_FUNC(weston_view_from_global_fixed, struct weston_view *, wl_fixed_t, wl_fixed_t, wl_fixed_t *, wl_fixed_t *);
DECLARE_FAKE_VOID_FUNC(wl_client_add_destroy_listener, struct wl_client *, struct wl_listener *);
DECLARE_FAKE_VOID_FUNC(wl_client_destroy, struct wl_client *);
DECLARE_FAKE_VOID_FUNC(wl_client_get_credentials, struct wl_client *, pid_t *, uid_t *, gid_t *);
DECLARE_FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *);
DECLARE_FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *);
DECLARE_FAKE_VOID_FUNC(wl_resource_post_no_memory, struct wl_resource *);
DECLARE_FAKE_VOID_FUNC(wl_resource_set_destructor, struct wl_resource *, wl_resource_destroy_func_t );
DECLARE_FAKE_VOID_FUNC(wl_resource_set_implementation, struct wl_resource *, const void *, void *, wl_resource_destroy_func_t);
DECLARE_FAKE_VALUE_FUNC_VARARG(int, weston_log, const char *, ...);
DECLARE_FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...);
DECLARE_FAKE_VALUE_FUNC(void *, wl_array_add, struct wl_array *, size_t);
DECLARE_FAKE_VOID_FUNC(wl_list_insert, struct wl_list *, struct wl_list *);
DECLARE_FAKE_VOID_FUNC(wl_array_init, struct wl_array *);
DECLARE_FAKE_VOID_FUNC(wl_array_release, struct wl_array *);
DECLARE_FAKE_VOID_FUNC(wl_list_init, struct wl_list *);
DECLARE_FAKE_VOID_FUNC(wl_list_remove, struct wl_list *);
DECLARE_FAKE_VALUE_FUNC(int, wl_list_empty, const struct wl_list *);
DECLARE_FAKE_VOID_FUNC(weston_keyboard_end_grab, struct weston_keyboard *);
DECLARE_FAKE_VOID_FUNC(weston_pointer_end_grab, struct weston_pointer *);
DECLARE_FAKE_VOID_FUNC(weston_touch_end_grab, struct weston_touch *);
DECLARE_FAKE_VOID_FUNC(weston_output_schedule_repaint, struct weston_output *);
DECLARE_FAKE_VALUE_FUNC(pixman_bool_t, pixman_region32_union, pixman_region32_t *, pixman_region32_t *, pixman_region32_t *);

#define SERVER_API_FAKE_LIST(FAKE) \
    FAKE(weston_config_get_section) \
    FAKE(weston_plugin_api_get) \
    FAKE(wet_load_module_entrypoint) \
    FAKE(weston_config_section_get_string) \
    FAKE(weston_config_section_get_bool) \
    FAKE(weston_view_create) \
    FAKE(weston_client_start) \
    FAKE(weston_config_section_get_color) \
    FAKE(weston_config_next_section) \
    FAKE(weston_config_section_get_int) \
    FAKE(wet_get_config) \
    FAKE(weston_config_section_get_uint) \
    FAKE(weston_compositor_pick_view) \
    FAKE(weston_desktop_surface_get_app_id) \
    FAKE(weston_desktop_surface_get_title) \
    FAKE(weston_surface_get_desktop_surface) \
    FAKE(weston_seat_get_keyboard) \
    FAKE(weston_seat_get_pointer) \
    FAKE(weston_seat_get_touch) \
    FAKE(weston_surface_get_main_surface) \
    FAKE(weston_touch_has_focus_resource) \
    FAKE(wl_display_get_event_loop) \
    FAKE(wl_display_next_serial) \
    FAKE(wl_event_loop_add_idle) \
    FAKE(wl_global_create) \
    FAKE(wl_resource_create) \
    FAKE(wl_resource_get_client) \
    FAKE(wl_resource_get_user_data) \
    FAKE(wl_resource_get_version) \
    FAKE(weston_layer_entry_remove) \
    FAKE(weston_layer_set_position) \
    FAKE(weston_view_destroy) \
    FAKE(weston_matrix_scale) \
    FAKE(weston_matrix_init) \
    FAKE(weston_surface_schedule_repaint) \
    FAKE(weston_surface_set_color) \
    FAKE(weston_compositor_schedule_repaint) \
    FAKE(weston_output_damage) \
    FAKE(weston_view_update_transform) \
    FAKE(weston_matrix_translate) \
    FAKE(weston_compositor_read_presentation_clock) \
    FAKE(weston_layer_init) \
    FAKE(weston_layer_entry_insert) \
    FAKE(weston_keyboard_send_keymap) \
    FAKE(weston_keyboard_start_grab) \
    FAKE(weston_pointer_clear_focus) \
    FAKE(weston_pointer_send_axis) \
    FAKE(weston_pointer_send_axis_source) \
    FAKE(weston_pointer_send_button) \
    FAKE(weston_pointer_send_frame) \
    FAKE(weston_pointer_send_motion) \
    FAKE(weston_pointer_set_focus) \
    FAKE(weston_pointer_start_grab) \
    FAKE(weston_touch_send_down) \
    FAKE(weston_touch_send_frame) \
    FAKE(weston_touch_send_motion ) \
    FAKE(weston_touch_send_up) \
    FAKE(weston_touch_set_focus) \
    FAKE(weston_touch_start_grab) \
    FAKE(weston_view_from_global_fixed) \
    FAKE(wl_client_add_destroy_listener) \
    FAKE(wl_client_destroy) \
    FAKE(wl_client_get_credentials) \
    FAKE(wl_client_post_no_memory) \
    FAKE(wl_resource_destroy) \
    FAKE(wl_resource_post_no_memory) \
    FAKE(wl_resource_set_destructor) \
    FAKE(wl_resource_set_implementation) \
    FAKE(wl_resource_post_event) \
    FAKE(wl_array_add) \
    FAKE(wl_list_insert) \
    FAKE(wl_array_init) \
    FAKE(wl_array_release) \
    FAKE(wl_list_init) \
    FAKE(wl_list_remove) \
    FAKE(wl_list_empty) \
    FAKE(weston_keyboard_end_grab) \
    FAKE(weston_pointer_end_grab) \
    FAKE(weston_touch_end_grab) \
    FAKE(weston_output_schedule_repaint) \
    FAKE(pixman_region32_union) \
    FFF_RESET_HISTORY()
     // FAKE(weston_log) // make a common custom weston_log to print the log

int custom_weston_log(const char *format, va_list ap);

#ifdef __cplusplus
}
#endif
#endif  // SERVER_API_FAKE
