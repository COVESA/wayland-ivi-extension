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

#include "server_api_fake.h"
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(struct weston_config_section *, weston_config_get_section, struct weston_config *, const char *, const char *, const char *);
DEFINE_FAKE_VALUE_FUNC(const void *, weston_plugin_api_get, struct weston_compositor *, const char *, size_t );
DEFINE_FAKE_VALUE_FUNC(void *, wet_load_module_entrypoint, const char *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, weston_config_section_get_bool, struct weston_config_section *, const char *, bool *, bool );
DEFINE_FAKE_VALUE_FUNC(int, weston_config_section_get_string, struct weston_config_section *, const char *, char **, const char *);
DEFINE_FAKE_VALUE_FUNC(struct weston_view *, weston_view_create, struct weston_surface *);
DEFINE_FAKE_VALUE_FUNC(struct wl_client *, weston_client_start, struct weston_compositor *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, weston_config_section_get_color, struct weston_config_section *, const char *, uint32_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, weston_config_next_section, struct weston_config *, struct weston_config_section **, const char **);
DEFINE_FAKE_VALUE_FUNC(int, weston_config_section_get_int, struct weston_config_section *, const char *, int32_t *, int32_t);
DEFINE_FAKE_VALUE_FUNC(struct weston_config *, wet_get_config, struct weston_compositor *);
DEFINE_FAKE_VALUE_FUNC(int, weston_config_section_get_uint, struct weston_config_section *, const char *, uint32_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(struct weston_view *, weston_compositor_pick_view, struct weston_compositor *, wl_fixed_t, wl_fixed_t, wl_fixed_t *, wl_fixed_t *);
DEFINE_FAKE_VALUE_FUNC(const char *, weston_desktop_surface_get_app_id, struct weston_desktop_surface *);
DEFINE_FAKE_VALUE_FUNC(const char *, weston_desktop_surface_get_title, struct weston_desktop_surface *);
DEFINE_FAKE_VALUE_FUNC(struct weston_desktop_surface *, weston_surface_get_desktop_surface, struct weston_surface *);
DEFINE_FAKE_VALUE_FUNC(struct weston_keyboard *, weston_seat_get_keyboard, struct weston_seat *);
DEFINE_FAKE_VALUE_FUNC(struct weston_pointer *, weston_seat_get_pointer, struct weston_seat *);
DEFINE_FAKE_VALUE_FUNC(struct weston_touch *, weston_seat_get_touch, struct weston_seat *);
DEFINE_FAKE_VALUE_FUNC(struct weston_surface *, weston_surface_get_main_surface, struct weston_surface *);
DEFINE_FAKE_VALUE_FUNC(bool, weston_touch_has_focus_resource, struct weston_touch *);
DEFINE_FAKE_VALUE_FUNC(struct wl_event_loop *, wl_display_get_event_loop, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(uint32_t, wl_display_next_serial, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(struct wl_event_source *, wl_event_loop_add_idle, struct wl_event_loop *, wl_event_loop_idle_func_t , void *);
DEFINE_FAKE_VALUE_FUNC(struct wl_global *, wl_global_create, struct wl_display *, const struct wl_interface *, int ,  void *, wl_global_bind_func_t);
DEFINE_FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int , uint32_t );
// DEFINE_FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_from_link, struct wl_list *);
DEFINE_FAKE_VALUE_FUNC(struct wl_client *, wl_resource_get_client, struct wl_resource *);
// DEFINE_FAKE_VALUE_FUNC(struct wl_list *, wl_resource_get_link, struct wl_resource *);
DEFINE_FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *);
DEFINE_FAKE_VALUE_FUNC(int, wl_resource_get_version, struct wl_resource *);
DEFINE_FAKE_VOID_FUNC(weston_layer_entry_remove, struct weston_layer_entry *);
DEFINE_FAKE_VOID_FUNC(weston_layer_set_position, struct weston_layer *, enum weston_layer_position );
DEFINE_FAKE_VOID_FUNC(weston_view_destroy, struct weston_view *);
DEFINE_FAKE_VOID_FUNC(weston_matrix_scale, struct weston_matrix *, float, float, float);
DEFINE_FAKE_VOID_FUNC(weston_matrix_init, struct weston_matrix *);
DEFINE_FAKE_VOID_FUNC(weston_surface_schedule_repaint, struct weston_surface *);
DEFINE_FAKE_VOID_FUNC(weston_surface_set_color, struct weston_surface *, float, float, float , float);
DEFINE_FAKE_VOID_FUNC(weston_compositor_schedule_repaint, struct weston_compositor *);
DEFINE_FAKE_VOID_FUNC(weston_output_damage, struct weston_output *);
DEFINE_FAKE_VOID_FUNC(weston_view_update_transform, struct weston_view *);
DEFINE_FAKE_VOID_FUNC(weston_matrix_translate, struct weston_matrix *, float, float, float);
DEFINE_FAKE_VOID_FUNC(weston_compositor_read_presentation_clock, const struct weston_compositor *, struct timespec *);
DEFINE_FAKE_VOID_FUNC(weston_layer_init, struct weston_layer *, struct weston_compositor *);
DEFINE_FAKE_VOID_FUNC(weston_layer_entry_insert, struct weston_layer_entry *, struct weston_layer_entry *);
DEFINE_FAKE_VOID_FUNC(weston_keyboard_send_keymap, struct weston_keyboard *, struct wl_resource *);
DEFINE_FAKE_VOID_FUNC(weston_keyboard_start_grab, struct weston_keyboard *, struct weston_keyboard_grab *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_clear_focus, struct weston_pointer *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_send_axis, struct weston_pointer *, const struct timespec *, struct weston_pointer_axis_event *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_send_axis_source, struct weston_pointer *, uint32_t);
DEFINE_FAKE_VOID_FUNC(weston_pointer_send_button, struct weston_pointer *, const struct timespec *, uint32_t, uint32_t);
DEFINE_FAKE_VOID_FUNC(weston_pointer_send_frame, struct weston_pointer *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_send_motion, struct weston_pointer *, const struct timespec *, struct weston_pointer_motion_event *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_set_focus, struct weston_pointer *, struct weston_view *, wl_fixed_t, wl_fixed_t);
DEFINE_FAKE_VOID_FUNC(weston_pointer_start_grab, struct weston_pointer *, struct weston_pointer_grab *);
DEFINE_FAKE_VOID_FUNC(weston_touch_send_down, struct weston_touch *, const struct timespec *, int , wl_fixed_t , wl_fixed_t );
DEFINE_FAKE_VOID_FUNC(weston_touch_send_frame, struct weston_touch *);
DEFINE_FAKE_VOID_FUNC(weston_touch_send_motion, struct weston_touch *, const struct timespec *, int , wl_fixed_t , wl_fixed_t );
DEFINE_FAKE_VOID_FUNC(weston_touch_send_up, struct weston_touch *, const struct timespec *, int );
DEFINE_FAKE_VOID_FUNC(weston_touch_set_focus, struct weston_touch *, struct weston_view *);
DEFINE_FAKE_VOID_FUNC(weston_touch_start_grab, struct weston_touch *, struct weston_touch_grab *);
DEFINE_FAKE_VOID_FUNC(weston_view_from_global_fixed, struct weston_view *, wl_fixed_t, wl_fixed_t, wl_fixed_t *, wl_fixed_t *);
DEFINE_FAKE_VOID_FUNC(wl_client_add_destroy_listener, struct wl_client *, struct wl_listener *);
DEFINE_FAKE_VOID_FUNC(wl_client_destroy, struct wl_client *);
DEFINE_FAKE_VOID_FUNC(wl_client_get_credentials, struct wl_client *, pid_t *, uid_t *, gid_t *);
DEFINE_FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *);
DEFINE_FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *);
DEFINE_FAKE_VOID_FUNC(wl_resource_post_no_memory, struct wl_resource *);
DEFINE_FAKE_VOID_FUNC(wl_resource_set_destructor, struct wl_resource *, wl_resource_destroy_func_t );
DEFINE_FAKE_VOID_FUNC(wl_resource_set_implementation, struct wl_resource *, const void *, void *, wl_resource_destroy_func_t);
DEFINE_FAKE_VALUE_FUNC_VARARG(int, weston_log, const char *, ...);
DEFINE_FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...);
DEFINE_FAKE_VOID_FUNC(wl_list_insert, struct wl_list *, struct wl_list *);
DEFINE_FAKE_VALUE_FUNC(void *, wl_array_add, struct wl_array *, size_t);
DEFINE_FAKE_VOID_FUNC(wl_list_init, struct wl_list *);
DEFINE_FAKE_VOID_FUNC(wl_list_remove, struct wl_list *);
DEFINE_FAKE_VOID_FUNC(wl_array_init, struct wl_array *);
DEFINE_FAKE_VOID_FUNC(wl_array_release, struct wl_array *);
DEFINE_FAKE_VALUE_FUNC(int, wl_list_empty, const struct wl_list *);
DEFINE_FAKE_VOID_FUNC(weston_keyboard_end_grab, struct weston_keyboard *);
DEFINE_FAKE_VOID_FUNC(weston_pointer_end_grab, struct weston_pointer *);
DEFINE_FAKE_VOID_FUNC(weston_touch_end_grab, struct weston_touch *);
DEFINE_FAKE_VOID_FUNC(weston_output_schedule_repaint, struct weston_output *);
DEFINE_FAKE_VALUE_FUNC(pixman_bool_t, pixman_region32_union, pixman_region32_t *, pixman_region32_t *, pixman_region32_t *);

int custom_weston_log(const char *format, va_list ap)
{
    vfprintf(stderr, format, ap);
    return 1;
}