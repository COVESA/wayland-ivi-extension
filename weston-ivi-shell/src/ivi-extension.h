/*
 * Copyright (C) 2013 DENSO CORPORATION
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef IVI_EXTENSION_H
#define IVI_EXTENSION_H

#include <stdbool.h>
#include <weston/compositor.h>

struct ivishell {
    struct weston_compositor *compositor;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;

    struct wl_list list_controller;
};

int32_t
ivi_extension_commit_changes(struct ivishell *shell);

int
ivi_extension_add_notification_create_surface(struct ivishell *shell,
					       surface_create_notification_func callback,
					       void *userdata);

int
ivi_extension_add_notification_remove_surface(struct ivishell *shell,
					       surface_remove_notification_func callback,
					       void *userdata);

int
ivi_extension_add_notification_create_layer(struct ivishell *shell,
					     layer_create_notification_func callback,
					     void *userdata);

int
ivi_extension_add_notification_remove_layer(struct ivishell *shell,
					     layer_remove_notification_func callback,
					     void *userdata);

int
ivi_extension_add_notification_configure_surface(struct ivishell *shell,
						  surface_configure_notification_func callback,
						  void *userdata);

int32_t
ivi_extension_get_surfaces(struct ivishell *shell,
			    int32_t *pLength,
			    struct ivi_layout_surface ***ppArray);

uint32_t
ivi_extension_get_id_of_surface(struct ivishell *shell,
				 struct ivi_layout_surface *ivisurf);

struct ivi_layout_surface *
ivi_extension_get_surface_from_id(struct ivishell *shell, uint32_t id_surface);

const struct ivi_layout_surface_properties *
ivi_extension_get_properties_of_surface(struct ivishell *shell,
					 struct ivi_layout_surface *ivisurf);

int32_t
ivi_extension_get_surfaces_on_layer(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer,
				     int32_t *pLength,
				     struct ivi_layout_surface ***ppArray);

int32_t
ivi_extension_surface_set_visibility(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf,
				      bool newVisibility);

bool
ivi_extension_surface_get_visibility(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf);

int32_t
ivi_extension_surface_set_opacity(struct ivishell *shell,
				   struct ivi_layout_surface *ivisurf,
				   wl_fixed_t opacity);

wl_fixed_t
ivi_extension_surface_get_opacity(struct ivishell *shell,
				   struct ivi_layout_surface *ivisurf);

int32_t
ivi_extension_surface_set_source_rectangle(struct ivishell *shell,
					    struct ivi_layout_surface *ivisurf,
					    int32_t x, int32_t y,
					    int32_t width, int32_t height);

int32_t
ivi_extension_surface_set_destination_rectangle(struct ivishell *shell,
						 struct ivi_layout_surface *ivisurf,
						 int32_t x, int32_t y,
						 int32_t width, int32_t height);

int32_t
ivi_extension_surface_set_position(struct ivishell *shell,
				    struct ivi_layout_surface *ivisurf,
				    int32_t dest_x, int32_t dest_y);

int32_t
ivi_extension_surface_get_position(struct ivishell *shell,
				    struct ivi_layout_surface *ivisurf,
				    int32_t *dest_x, int32_t *dest_y);

int32_t
ivi_extension_surface_set_dimension(struct ivishell *shell,
				     struct ivi_layout_surface *ivisurf,
				     int32_t dest_width, int32_t dest_height);

int32_t
ivi_extension_surface_get_dimension(struct ivishell *shell,
				     struct ivi_layout_surface *ivisurf,
				     int32_t *dest_width, int32_t *dest_height);

int32_t
ivi_extension_surface_set_orientation(struct ivishell *shell,
				       struct ivi_layout_surface *ivisurf,
				       enum wl_output_transform orientation);

enum wl_output_transform
ivi_extension_surface_get_orientation(struct ivishell *shell,
				       struct ivi_layout_surface *ivisurf);

int32_t
ivi_extension_surface_set_content_observer(struct ivishell *shell,
					    struct ivi_layout_surface *ivisurf,
					    ivi_controller_surface_content_callback callback,
					    void* userdata);

int32_t
ivi_extension_surface_add_notification(struct ivishell *shell,
					struct ivi_layout_surface *ivisurf,
					surface_property_notification_func callback,
					void *userdata);

void
ivi_extension_surface_remove_notification(struct ivishell *shell,
					   struct ivi_layout_surface *ivisurf);

struct weston_surface *
ivi_extension_surface_get_weston_surface(struct ivishell *shell,
					  struct ivi_layout_surface *ivisurf);

int32_t
ivi_extension_surface_set_transition(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf,
				      enum ivi_layout_transition_type type,
				      uint32_t duration);

int32_t
ivi_extension_surface_set_transition_duration(struct ivishell *shell,
					       struct ivi_layout_surface *ivisurf,
					       uint32_t duration);

int32_t
ivi_extension_surface_dump(struct ivishell *ivishell,
			   struct weston_surface *surface,
			   void *target,
			   size_t size,
			   int32_t x,
			   int32_t y,
			   int32_t width,
			   int32_t height);

int32_t
ivi_extension_surface_get_size(struct ivishell *shell,
				struct ivi_layout_surface *ivisurf,
				int32_t *width,
				int32_t *height,
				int32_t *stride);

struct ivi_layout_layer *
ivi_extension_layer_create_with_dimension(struct ivishell *shell,
					   uint32_t id_layer, int32_t width, int32_t height);

void
ivi_extension_layer_remove(struct ivishell *shell,
			    struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_get_layers(struct ivishell *shell,
			  int32_t *pLength, struct ivi_layout_layer ***ppArray);

uint32_t
ivi_extension_get_id_of_layer(struct ivishell *shell,
			       struct ivi_layout_layer *ivilayer);

struct ivi_layout_layer *
ivi_extension_get_layer_from_id(struct ivishell *shell, uint32_t id_layer);

const struct ivi_layout_layer_properties *
ivi_extension_get_properties_of_layer(struct ivishell *shell,
				       struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_get_layers_under_surface(struct ivishell *shell,
					struct ivi_layout_surface *ivisurf,
					int32_t *pLength,
					struct ivi_layout_layer ***ppArray);

int32_t
ivi_extension_get_layers_on_screen(struct ivishell *shell,
				    struct ivi_layout_screen *iviscrn,
				    int32_t *pLength,
				    struct ivi_layout_layer ***ppArray);

int32_t
ivi_extension_layer_set_visibility(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    bool newVisibility);

bool
ivi_extension_layer_get_visibility(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_layer_set_opacity(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer,
				 wl_fixed_t opacity);

wl_fixed_t
ivi_extension_layer_get_opacity(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_layer_set_source_rectangle(struct ivishell *shell,
					  struct ivi_layout_layer *ivilayer,
					  int32_t x, int32_t y,
					  int32_t width, int32_t height);

int32_t
ivi_extension_layer_set_destination_rectangle(struct ivishell *shell,
					       struct ivi_layout_layer *ivilayer,
					       int32_t x, int32_t y,
					       int32_t width, int32_t height);

int32_t
ivi_extension_layer_set_position(struct ivishell *shell,
				  struct ivi_layout_layer *ivilayer,
				  int32_t dest_x, int32_t dest_y);

int32_t
ivi_extension_layer_get_position(struct ivishell *shell,
				  struct ivi_layout_layer *ivilayer,
				  int32_t *dest_x, int32_t *dest_y);

int32_t
ivi_extension_layer_set_dimension(struct ivishell *shell,
				   struct ivi_layout_layer *ivilayer,
				   int32_t dest_width, int32_t dest_height);

int32_t
ivi_extension_layer_get_dimension(struct ivishell *shell,
				   struct ivi_layout_layer *ivilayer,
				   int32_t *dest_width, int32_t *dest_height);

int32_t
ivi_extension_layer_set_orientation(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer,
				     enum wl_output_transform orientation);

enum wl_output_transform
ivi_extension_layer_get_orientation(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_layer_add_surface(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer,
				 struct ivi_layout_surface *addsurf);

void
ivi_extension_layer_remove_surface(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    struct ivi_layout_surface *remsurf);

int32_t
ivi_extension_layer_set_render_order(struct ivishell *shell,
				      struct ivi_layout_layer *ivilayer,
				      struct ivi_layout_surface **pSurface,
				      int32_t number);

int32_t
ivi_extension_layer_add_notification(struct ivishell *shell,
				      struct ivi_layout_layer *ivilayer,
				      layer_property_notification_func callback,
				      void *userdata);

void
ivi_extension_layer_remove_notification(struct ivishell *shell,
					 struct ivi_layout_layer *ivilayer);

int32_t
ivi_extension_layer_set_transition(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    enum ivi_layout_transition_type type,
				    uint32_t duration);

struct ivi_layout_screen *
ivi_extension_get_screen_from_id(struct ivishell *shell,
				  uint32_t id_screen);

int32_t
ivi_extension_get_screen_resolution(struct ivishell *shell,
				     struct ivi_layout_screen *iviscrn,
				     int32_t *pWidth,
				     int32_t *pHeight);

int32_t
ivi_extension_get_screens(struct ivishell *shell,
			   int32_t *pLength, struct ivi_layout_screen ***ppArray);

int32_t
ivi_extension_get_screens_under_layer(struct ivishell *shell,
				       struct ivi_layout_layer *ivilayer,
				       int32_t *pLength,
				       struct ivi_layout_screen ***ppArray);

int32_t
ivi_extension_screen_add_layer(struct ivishell *shell,
				struct ivi_layout_screen *iviscrn,
				struct ivi_layout_layer *addlayer);

int32_t
ivi_extension_screen_set_render_order(struct ivishell *shell,
				       struct ivi_layout_screen *iviscrn,
				       struct ivi_layout_layer **pLayer,
				       const int32_t number);

struct weston_output *
ivi_extension_screen_get_output(struct ivishell *shell,
				 struct ivi_layout_screen *);


void
ivi_extension_transition_move_layer_cancel(struct ivishell *shell,
					    struct ivi_layout_layer *layer);

int32_t
ivi_extension_layer_set_fade_info(struct ivishell *shell,
				   struct ivi_layout_layer* ivilayer,
				   uint32_t is_fade_in,
				   double start_alpha, double end_alpha);

int32_t
ivi_extension_surface_set_is_forced_configure_event(struct ivishell *shell,
						     struct weston_surface *surface,
						     bool is_force);
#endif
