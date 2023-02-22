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

#include "ivi_layout_interface_fake.h"

DEFINE_FAKE_VALUE_FUNC(const struct ivi_layout_layer_properties *, get_properties_of_layer, struct ivi_layout_layer *);
DEFINE_FAKE_VALUE_FUNC(const struct ivi_layout_surface_properties *, get_properties_of_surface, struct ivi_layout_surface *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_configure_desktop_surface, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_configure_surface, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_create_layer, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_create_surface, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_remove_layer, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, add_listener_remove_surface, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, commit_changes);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_layers, int32_t *, struct ivi_layout_layer ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_layers_on_screen, struct weston_output *, int32_t *, struct ivi_layout_layer ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_layers_under_surface, struct ivi_layout_surface *, int32_t *, struct ivi_layout_layer ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_screens_under_layer, struct ivi_layout_layer *, int32_t *, struct weston_output ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_surfaces, int32_t *, struct ivi_layout_surface ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, get_surfaces_on_layer, struct ivi_layout_layer *, int32_t *, struct ivi_layout_surface ***);
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_add_listener, struct ivi_layout_layer *, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_add_surface, struct ivi_layout_layer *, struct ivi_layout_surface *);
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_destination_rectangle, struct ivi_layout_layer *, int32_t , int32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_fade_info, struct ivi_layout_layer* , uint32_t , double , double );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_opacity, struct ivi_layout_layer *, wl_fixed_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_render_order, struct ivi_layout_layer *, struct ivi_layout_surface **, int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_source_rectangle, struct ivi_layout_layer *, int32_t , int32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_transition, struct ivi_layout_layer *, enum ivi_layout_transition_type , uint32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, layer_set_visibility, struct ivi_layout_layer *, bool );
DEFINE_FAKE_VALUE_FUNC(int32_t, screen_add_layer, struct weston_output *, struct ivi_layout_layer *);
DEFINE_FAKE_VALUE_FUNC(int32_t, screen_remove_layer, struct weston_output *, struct ivi_layout_layer *);
DEFINE_FAKE_VALUE_FUNC(int32_t, screen_set_render_order, struct weston_output *, struct ivi_layout_layer **, const int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_add_listener, struct ivi_layout_surface *, struct wl_listener *);
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_dump, struct weston_surface *, void *, size_t , int32_t , int32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_get_size, struct ivi_layout_surface *, int32_t *, int32_t *, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_destination_rectangle, struct ivi_layout_surface *, int32_t , int32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_id, struct ivi_layout_surface *, uint32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_opacity, struct ivi_layout_surface *, wl_fixed_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_source_rectangle, struct ivi_layout_surface *, int32_t , int32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_transition_duration, struct ivi_layout_surface *, uint32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_transition, struct ivi_layout_surface *, enum ivi_layout_transition_type , uint32_t );
DEFINE_FAKE_VALUE_FUNC(int32_t, surface_set_visibility, struct ivi_layout_surface *, bool );
DEFINE_FAKE_VALUE_FUNC(struct ivi_layout_layer *, get_layer_from_id, uint32_t );
DEFINE_FAKE_VALUE_FUNC(struct ivi_layout_layer *, layer_create_with_dimension, uint32_t , int32_t , int32_t );
DEFINE_FAKE_VALUE_FUNC(struct ivi_layout_surface *, get_surface_from_id, uint32_t );
DEFINE_FAKE_VALUE_FUNC(struct ivi_layout_surface *, get_surface, struct weston_surface *);
DEFINE_FAKE_VALUE_FUNC(struct weston_surface *, surface_get_weston_surface, struct ivi_layout_surface *);
DEFINE_FAKE_VALUE_FUNC(uint32_t, get_id_of_layer, struct ivi_layout_layer *);
DEFINE_FAKE_VALUE_FUNC(uint32_t, get_id_of_surface, struct ivi_layout_surface *);
DEFINE_FAKE_VOID_FUNC(focus, struct weston_pointer_grab *);
DEFINE_FAKE_VOID_FUNC(layer_destroy, struct ivi_layout_layer *);
DEFINE_FAKE_VOID_FUNC(layer_remove_surface, struct ivi_layout_layer *, struct ivi_layout_surface *);
DEFINE_FAKE_VOID_FUNC(transition_move_layer_cancel, struct ivi_layout_layer *);

struct ivi_layout_interface g_iviLayoutInterfaceFake = {
    .commit_changes = commit_changes,
    .add_listener_create_surface = add_listener_create_surface,
    .add_listener_remove_surface = add_listener_remove_surface,
    .add_listener_configure_surface = add_listener_configure_surface,
    .add_listener_configure_desktop_surface = add_listener_configure_desktop_surface,
    .get_surfaces = get_surfaces,
    .get_id_of_surface = get_id_of_surface,
    .get_surface_from_id = get_surface_from_id,
    .get_properties_of_surface = get_properties_of_surface,
    .get_surfaces_on_layer = get_surfaces_on_layer,
    .surface_set_visibility = surface_set_visibility,
    .surface_set_opacity = surface_set_opacity,
    .surface_set_source_rectangle = surface_set_source_rectangle,
    .surface_set_destination_rectangle = surface_set_destination_rectangle,
    .surface_add_listener = surface_add_listener,
    .surface_get_weston_surface = surface_get_weston_surface,
    .surface_set_transition = surface_set_transition,
    .surface_set_transition_duration = surface_set_transition_duration,
    .surface_set_id = surface_set_id,
    .add_listener_create_layer = add_listener_create_layer,
    .add_listener_remove_layer = add_listener_remove_layer,
    .layer_create_with_dimension = layer_create_with_dimension,
    .layer_destroy = layer_destroy,
    .get_layers = get_layers,
    .get_id_of_layer = get_id_of_layer,
    .get_layer_from_id = get_layer_from_id,
    .get_properties_of_layer = get_properties_of_layer,
    .get_layers_under_surface = get_layers_under_surface,
    .get_layers_on_screen = get_layers_on_screen,
    .layer_set_visibility = layer_set_visibility,
    .layer_set_opacity = layer_set_opacity,
    .layer_set_source_rectangle = layer_set_source_rectangle,
    .layer_set_destination_rectangle = layer_set_destination_rectangle,
    .layer_add_surface = layer_add_surface,
    .layer_remove_surface = layer_remove_surface,
    .layer_set_render_order = layer_set_render_order,
    .layer_add_listener = layer_add_listener,
    .layer_set_transition = layer_set_transition,
    .get_screens_under_layer = get_screens_under_layer,
    .screen_add_layer = screen_add_layer,
    .screen_set_render_order = screen_set_render_order,
    .transition_move_layer_cancel = transition_move_layer_cancel,
    .layer_set_fade_info = layer_set_fade_info,
    .surface_get_size = surface_get_size,
    .surface_dump = surface_dump,
    .get_surface = get_surface,
    .screen_remove_layer = screen_remove_layer,
};

struct weston_pointer_grab_interface g_grabInterfaceFake = {
    .focus = focus,
};
