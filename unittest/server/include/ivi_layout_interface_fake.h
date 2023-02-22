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

#ifndef IVI_LAYOUT_INTERFACE_FAKE
#define IVI_LAYOUT_INTERFACE_FAKE
#include "ivi-wm-server-protocol.h"
#include "ivi-layout-export.h"
#include "libweston-desktop/libweston-desktop.h"
#include "weston.h"
#include "fff.h"

#ifdef __cplusplus
extern "C" {
#endif
 // create the fake interface
const struct ivi_layout_layer_properties *get_properties_of_layer(struct ivi_layout_layer *ivilayer);
const struct ivi_layout_surface_properties *get_properties_of_surface(struct ivi_layout_surface *ivisurf);
int32_t add_listener_configure_desktop_surface(struct wl_listener *listener);
int32_t add_listener_configure_surface(struct wl_listener *listener);
int32_t add_listener_create_layer(struct wl_listener *listener);
int32_t add_listener_create_surface(struct wl_listener *listener);
int32_t add_listener_remove_layer(struct wl_listener *listener);
int32_t add_listener_remove_surface(struct wl_listener *listener);
int32_t commit_changes(void);
int32_t get_layers(int32_t *pLength, struct ivi_layout_layer ***ppArray);
int32_t get_layers_on_screen(struct weston_output *output, int32_t *pLength, struct ivi_layout_layer ***ppArray);
int32_t get_layers_under_surface(struct ivi_layout_surface *ivisurf, int32_t *pLength, struct ivi_layout_layer ***ppArray);
int32_t get_screens_under_layer(struct ivi_layout_layer *ivilayer, int32_t *pLength, struct weston_output ***ppArray);
int32_t get_surfaces(int32_t *pLength, struct ivi_layout_surface ***ppArray);
int32_t get_surfaces_on_layer(struct ivi_layout_layer *ivilayer, int32_t *pLength, struct ivi_layout_surface ***ppArray);
int32_t layer_add_listener(struct ivi_layout_layer *ivilayer, struct wl_listener *listener);
int32_t layer_add_surface(struct ivi_layout_layer *ivilayer, struct ivi_layout_surface *addsurf);
int32_t layer_set_destination_rectangle(struct ivi_layout_layer *ivilayer, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t layer_set_fade_info(struct ivi_layout_layer* ivilayer, uint32_t is_fade_in, double start_alpha, double end_alpha);
int32_t layer_set_opacity(struct ivi_layout_layer *ivilayer, wl_fixed_t opacity);
int32_t layer_set_render_order(struct ivi_layout_layer *ivilayer, struct ivi_layout_surface **pSurface, int32_t number);
int32_t layer_set_source_rectangle(struct ivi_layout_layer *ivilayer, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t layer_set_transition(struct ivi_layout_layer *ivilayer, enum ivi_layout_transition_type type, uint32_t duration);
int32_t layer_set_visibility(struct ivi_layout_layer *ivilayer, bool newVisibility);
int32_t screen_add_layer(struct weston_output *output, struct ivi_layout_layer *addlayer);
int32_t screen_remove_layer(struct weston_output *output, struct ivi_layout_layer *removelayer);
int32_t screen_set_render_order(struct weston_output *output, struct ivi_layout_layer **pLayer, const int32_t number);
int32_t surface_add_listener(struct ivi_layout_surface *ivisurf, struct wl_listener *listener);
int32_t surface_dump(struct weston_surface *surface, void *target, size_t size, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t surface_get_size(struct ivi_layout_surface *ivisurf, int32_t *width, int32_t *height, int32_t *stride);
int32_t surface_set_destination_rectangle(struct ivi_layout_surface *ivisurf, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t surface_set_id(struct ivi_layout_surface *ivisurf, uint32_t id_surface);
int32_t surface_set_opacity(struct ivi_layout_surface *ivisurf, wl_fixed_t opacity);
int32_t surface_set_source_rectangle(struct ivi_layout_surface *ivisurf, int32_t x, int32_t y, int32_t width, int32_t height);
int32_t surface_set_transition_duration(struct ivi_layout_surface *ivisurf, uint32_t duration);
int32_t surface_set_transition(struct ivi_layout_surface *ivisurf, enum ivi_layout_transition_type type, uint32_t duration);
int32_t surface_set_visibility(struct ivi_layout_surface *ivisurf, bool newVisibility);
struct ivi_layout_layer * get_layer_from_id(uint32_t id_layer);
struct ivi_layout_layer *layer_create_with_dimension(uint32_t id_layer, int32_t width, int32_t height);
struct ivi_layout_surface *get_surface_from_id(uint32_t id_surface);
struct ivi_layout_surface *get_surface(struct weston_surface *surface);
struct weston_surface *surface_get_weston_surface(struct ivi_layout_surface *ivisurf);
uint32_t get_id_of_layer(struct ivi_layout_layer *ivilayer);
uint32_t get_id_of_surface(struct ivi_layout_surface *ivisurf);
void focus(struct weston_pointer_grab *grab);
void layer_destroy(struct ivi_layout_layer *ivilayer);
void layer_remove_surface(struct ivi_layout_layer *ivilayer, struct ivi_layout_surface *remsurf);
void transition_move_layer_cancel(struct ivi_layout_layer *layer);

//Stub all functions
DECLARE_FAKE_VALUE_FUNC(const struct ivi_layout_layer_properties *, get_properties_of_layer, struct ivi_layout_layer *);
DECLARE_FAKE_VALUE_FUNC(const struct ivi_layout_surface_properties *, get_properties_of_surface, struct ivi_layout_surface *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_configure_desktop_surface, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_configure_surface, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_create_layer, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_create_surface, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_remove_layer, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, add_listener_remove_surface, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, commit_changes);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_layers, int32_t *, struct ivi_layout_layer ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_layers_on_screen, struct weston_output *, int32_t *, struct ivi_layout_layer ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_layers_under_surface, struct ivi_layout_surface *, int32_t *, struct ivi_layout_layer ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_screens_under_layer, struct ivi_layout_layer *, int32_t *, struct weston_output ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_surfaces, int32_t *, struct ivi_layout_surface ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, get_surfaces_on_layer, struct ivi_layout_layer *, int32_t *, struct ivi_layout_surface ***);
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_add_listener, struct ivi_layout_layer *, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_add_surface, struct ivi_layout_layer *, struct ivi_layout_surface *);
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_destination_rectangle, struct ivi_layout_layer *, int32_t , int32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_fade_info, struct ivi_layout_layer* , uint32_t , double , double );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_opacity, struct ivi_layout_layer *, wl_fixed_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_render_order, struct ivi_layout_layer *, struct ivi_layout_surface **, int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_source_rectangle, struct ivi_layout_layer *, int32_t , int32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_transition, struct ivi_layout_layer *, enum ivi_layout_transition_type , uint32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, layer_set_visibility, struct ivi_layout_layer *, bool );
DECLARE_FAKE_VALUE_FUNC(int32_t, screen_add_layer, struct weston_output *, struct ivi_layout_layer *);
DECLARE_FAKE_VALUE_FUNC(int32_t, screen_remove_layer, struct weston_output *, struct ivi_layout_layer *);
DECLARE_FAKE_VALUE_FUNC(int32_t, screen_set_render_order, struct weston_output *, struct ivi_layout_layer **, const int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_add_listener, struct ivi_layout_surface *, struct wl_listener *);
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_dump, struct weston_surface *, void *, size_t , int32_t , int32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_get_size, struct ivi_layout_surface *, int32_t *, int32_t *, int32_t *);
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_destination_rectangle, struct ivi_layout_surface *, int32_t , int32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_id, struct ivi_layout_surface *, uint32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_opacity, struct ivi_layout_surface *, wl_fixed_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_source_rectangle, struct ivi_layout_surface *, int32_t , int32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_transition_duration, struct ivi_layout_surface *, uint32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_transition, struct ivi_layout_surface *, enum ivi_layout_transition_type , uint32_t );
DECLARE_FAKE_VALUE_FUNC(int32_t, surface_set_visibility, struct ivi_layout_surface *, bool );
DECLARE_FAKE_VALUE_FUNC(struct ivi_layout_layer *, get_layer_from_id, uint32_t );
DECLARE_FAKE_VALUE_FUNC(struct ivi_layout_layer *, layer_create_with_dimension, uint32_t , int32_t , int32_t );
DECLARE_FAKE_VALUE_FUNC(struct ivi_layout_surface *, get_surface_from_id, uint32_t );
DECLARE_FAKE_VALUE_FUNC(struct ivi_layout_surface *, get_surface, struct weston_surface *);
DECLARE_FAKE_VALUE_FUNC(struct weston_surface *, surface_get_weston_surface, struct ivi_layout_surface *);
DECLARE_FAKE_VALUE_FUNC(uint32_t, get_id_of_layer, struct ivi_layout_layer *);
DECLARE_FAKE_VALUE_FUNC(uint32_t, get_id_of_surface, struct ivi_layout_surface *);
DECLARE_FAKE_VOID_FUNC(focus, struct weston_pointer_grab *);
DECLARE_FAKE_VOID_FUNC(layer_destroy, struct ivi_layout_layer *);
DECLARE_FAKE_VOID_FUNC(layer_remove_surface, struct ivi_layout_layer *, struct ivi_layout_surface *);
DECLARE_FAKE_VOID_FUNC(transition_move_layer_cancel, struct ivi_layout_layer *);

#define IVI_LAYOUT_FAKE_LIST(FAKE) \
    FAKE(get_properties_of_layer) \
    FAKE(get_properties_of_surface) \
    FAKE(add_listener_configure_desktop_surface) \
    FAKE(add_listener_configure_surface) \
    FAKE(add_listener_create_layer) \
    FAKE(add_listener_create_surface) \
    FAKE(add_listener_remove_layer) \
    FAKE(add_listener_remove_surface) \
    FAKE(commit_changes) \
    FAKE(get_layers) \
    FAKE(get_layers_on_screen) \
    FAKE(get_layers_under_surface) \
    FAKE(get_screens_under_layer) \
    FAKE(get_surfaces) \
    FAKE(get_surfaces_on_layer) \
    FAKE(layer_add_listener) \
    FAKE(layer_add_surface) \
    FAKE(layer_set_destination_rectangle) \
    FAKE(layer_set_fade_info) \
    FAKE(layer_set_opacity) \
    FAKE(layer_set_render_order) \
    FAKE(layer_set_source_rectangle) \
    FAKE(layer_set_transition) \
    FAKE(layer_set_visibility) \
    FAKE(screen_add_layer) \
    FAKE(screen_remove_layer) \
    FAKE(screen_set_render_order) \
    FAKE(surface_add_listener) \
    FAKE(surface_dump) \
    FAKE(surface_get_size) \
    FAKE(surface_set_destination_rectangle) \
    FAKE(surface_set_id) \
    FAKE(surface_set_opacity) \
    FAKE(surface_set_source_rectangle) \
    FAKE(surface_set_transition_duration) \
    FAKE(surface_set_transition) \
    FAKE(surface_set_visibility) \
    FAKE(get_layer_from_id) \
    FAKE(layer_create_with_dimension) \
    FAKE(get_surface_from_id) \
    FAKE(get_surface) \
    FAKE(surface_get_weston_surface) \
    FAKE(get_id_of_layer) \
    FAKE(get_id_of_surface) \
    FAKE(focus) \
    FAKE(layer_destroy) \
    FAKE(layer_remove_surface) \
    FAKE(transition_move_layer_cancel)

extern struct ivi_layout_interface g_iviLayoutInterfaceFake;
extern struct weston_pointer_grab_interface g_grabInterfaceFake;

#ifdef __cplusplus
}
#endif
#endif  // IVI_LAYOUT_INTERFACE_FAKE