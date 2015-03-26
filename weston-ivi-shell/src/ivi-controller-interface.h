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

/**
 * The ivi-layout library supports API set of controlling properties of
 * surface and layer which groups surfaces. An unique ID whose type is integer
 * is required to create surface and layer. With the unique ID, surface and
 * layer are identified to control them. The API set consists of APIs to control
 * properties of surface and layers about followings,
 * - visibility.
 * - opacity.
 * - clipping (x,y,width,height).
 * - position and size of it to be displayed.
 * - orientation per 90 degree.
 * - add or remove surfaces to a layer.
 * - order of surfaces/layers in layer/screen to be displayed.
 * - commit to apply property changes.
 * - notifications of property change.
 *
 * Management of surfaces and layers grouping these surfaces are common
 * way in In-Vehicle Infotainment system, which integrate several domains
 * in one system. A layer is allocated to a domain in order to control
 * application surfaces grouped to the layer all together.
 *
 * This API and ABI follow following specifications.
 * http://projects.genivi.org/wayland-ivi-extension/layer-manager-apis
 */

#ifndef _IVI_CONTROLLER_INTERFACE_H_
#define _IVI_CONTROLLER_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ivi-layout-export.h"

struct ivi_controller_interface {

	/**
	 * \brief Commit all changes and execute all enqueued commands since
	 * last commit.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*commit_changes)(void);

	/**
	 * surface controller interface
	 */

	/**
	 * \brief register/unregister for notification when ivi_surface is created
	 */
	int32_t (*add_notification_create_surface)(
				surface_create_notification_func callback,
				void *userdata);

	void (*remove_notification_create_surface)(
				surface_create_notification_func callback,
				void *userdata);

	/**
	 * \brief register/unregister for notification when ivi_surface is removed
	 */
	int32_t (*add_notification_remove_surface)(
				surface_remove_notification_func callback,
				void *userdata);

	void (*remove_notification_remove_surface)(
				surface_remove_notification_func callback,
				void *userdata);

	/**
	 * \brief register/unregister for notification when ivi_surface is configured
	 */
	int32_t (*add_notification_configure_surface)(
				surface_configure_notification_func callback,
				void *userdata);

	void (*remove_notification_configure_surface)(
				surface_configure_notification_func callback,
				void *userdata);

	/**
	 * \brief Get all ivi_surfaces which are currently registered and managed
	 * by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_surfaces)(int32_t *pLength, struct ivi_layout_surface ***ppArray);

	/**
	 * \brief get id of ivi_surface from ivi_layout_surface
	 *
	 * \return id of ivi_surface
	 */
	uint32_t (*get_id_of_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief get ivi_layout_surface from id of ivi_surface
	 *
	 * \return (struct ivi_layout_surface *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_surface *
		(*get_surface_from_id)(uint32_t id_surface);

	/**
	 * \brief get ivi_layout_surface_properties from ivisurf
	 *
	 * \return (struct ivi_layout_surface_properties *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	const struct ivi_layout_surface_properties *
		(*get_properties_of_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief Get all Surfaces which are currently registered to a given
	 * layer and are managed by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_surfaces_on_layer)(struct ivi_layout_layer *ivilayer,
					 int32_t *pLength,
					 struct ivi_layout_surface ***ppArray);

	/**
	 * \brief Set the visibility of a ivi_surface.
	 *
	 * If a surface is not visible it will not be rendered.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_visibility)(struct ivi_layout_surface *ivisurf,
					  bool newVisibility);

	/**
	 * \brief Get the visibility of a surface.
	 *
	 * If a surface is not visible it will not be rendered.
	 *
	 * \return true if surface is visible
	 * \return false if surface is invisible or the method call was failed
	 */
	bool (*surface_get_visibility)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief Set the opacity of a surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_opacity)(struct ivi_layout_surface *ivisurf,
				       wl_fixed_t opacity);

	/**
	 * \brief Get the opacity of a ivi_surface.
	 *
	 * \return opacity if the method call was successful
	 * \return wl_fixed_from_double(0.0) if the method call was failed
	 */
	wl_fixed_t (*surface_get_opacity)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief Set the area of a ivi_surface which should be used for the rendering.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_source_rectangle)(struct ivi_layout_surface *ivisurf,
						int32_t x, int32_t y,
						int32_t width, int32_t height);

	/**
	 * \brief Set the destination area of a ivi_surface within a ivi_layer
	 * for rendering.
	 *
	 * The surface will be scaled to this rectangle for rendering.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_destination_rectangle)(struct ivi_layout_surface *ivisurf,
						     int32_t x, int32_t y,
						     int32_t width, int32_t height);

	/**
	 * \brief Sets the horizontal and vertical position of the surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_position)(struct ivi_layout_surface *ivisurf,
					int32_t dest_x, int32_t dest_y);

	/**
	 * \brief Get the horizontal and vertical position of the surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_get_position)(struct ivi_layout_surface *ivisurf,
					int32_t *dest_x, int32_t *dest_y);

	/**
	 * \brief Set the horizontal and vertical dimension of the surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_dimension)(struct ivi_layout_surface *ivisurf,
					 int32_t dest_width, int32_t dest_height);

	/**
	 * \brief Get the horizontal and vertical dimension of the surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_get_dimension)(struct ivi_layout_surface *ivisurf,
					 int32_t *dest_width, int32_t *dest_height);

	/**
	 * \brief Sets the orientation of a ivi_surface.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_orientation)(struct ivi_layout_surface *ivisurf,
					   enum wl_output_transform orientation);

	/**
	 * \brief Gets the orientation of a surface.
	 *
	 * \return (enum wl_output_transform)
	 *              if the method call was successful
	 * \return WL_OUTPUT_TRANSFORM_NORMAL if the method call was failed
	 */
	enum wl_output_transform
		(*surface_get_orientation)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief Set an observer callback for ivi_surface content status change.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_set_content_observer)(
				struct ivi_layout_surface *ivisurf,
				ivi_controller_surface_content_callback callback,
				void* userdata);

	/**
	 * \brief register for notification on property changes of ivi_surface
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*surface_add_notification)(struct ivi_layout_surface *ivisurf,
					    surface_property_notification_func callback,
					    void *userdata);

	/**
	 * \brief remove notification on property changes of ivi_surface
	 */
	void (*surface_remove_notification)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief get weston_surface of ivi_surface
	 */
	struct weston_surface *
		(*surface_get_weston_surface)(struct ivi_layout_surface *ivisurf);

	/**
	 * \brief get size and stride of ivi_surface
	 */
	int32_t (*surface_get_size)
		(struct ivi_layout_surface *ivisurf, int32_t *width, int32_t *height, int32_t *stride);

	/**
	 * \brief set type of transition animation
	 */
	int32_t (*surface_set_transition)(struct ivi_layout_surface *ivisurf,
					  enum ivi_layout_transition_type type,
					  uint32_t duration);

	/**
	 * \brief set duration of transition animation
	 */
	int32_t (*surface_set_transition_duration)(
					struct ivi_layout_surface *ivisurf,
					uint32_t duration);

	/**
	 * layer controller interface
	 */

	/**
	 * \brief register/unregister for notification when ivi_layer is created
	 */
	int32_t (*add_notification_create_layer)(
				layer_create_notification_func callback,
				void *userdata);

	void (*remove_notification_create_layer)(
				layer_create_notification_func callback,
				void *userdata);

	/**
	 * \brief register/unregister for notification when ivi_layer is removed
	 */
	int32_t (*add_notification_remove_layer)(
				layer_remove_notification_func callback,
				void *userdata);

	void (*remove_notification_remove_layer)(
				layer_remove_notification_func callback,
				void *userdata);

	/**
	 * \brief Create a ivi_layer which should be managed by the service
	 *
	 * \return (struct ivi_layout_layer *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_layer *
		(*layer_create_with_dimension)(uint32_t id_layer,
					       int32_t width, int32_t height);

	/**
	 * \brief Removes a ivi_layer which is currently managed by the service
	 */
	void (*layer_remove)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Get all ivi_layers which are currently registered and managed
	 * by the services
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers)(int32_t *pLength, struct ivi_layout_layer ***ppArray);

	/**
	 * \brief get id of ivi_layer from ivi_layout_layer
	 *
	 *
	 * \return id of ivi_layer
	 */
	uint32_t (*get_id_of_layer)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief get ivi_layout_layer from id of layer
	 *
	 * \return (struct ivi_layout_layer *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_layer * (*get_layer_from_id)(uint32_t id_layer);

	/**
	 * \brief  Get the ivi_layer properties
	 *
	 * \return (const struct ivi_layout_layer_properties *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	const struct ivi_layout_layer_properties *
		(*get_properties_of_layer)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Get all ivi_ayers under the given ivi_surface
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers_under_surface)(struct ivi_layout_surface *ivisurf,
					    int32_t *pLength,
					    struct ivi_layout_layer ***ppArray);

	/**
	 * \brief Get all Layers of the given screen
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_layers_on_screen)(struct ivi_layout_screen *iviscrn,
					int32_t *pLength,
					struct ivi_layout_layer ***ppArray);

	/**
	 * \brief Set the visibility of a ivi_layer. If a ivi_layer is not visible,
	 * the ivi_layer and its ivi_surfaces will not be rendered.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_visibility)(struct ivi_layout_layer *ivilayer,
					bool newVisibility);

	/**
	 * \brief Get the visibility of a layer. If a layer is not visible,
	 * the layer and its surfaces will not be rendered.
	 *
	 * \return true if layer is visible
	 * \return false if layer is invisible or the method call was failed
	 */
	bool (*layer_get_visibility)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Set the opacity of a ivi_layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_opacity)(struct ivi_layout_layer *ivilayer,
				     wl_fixed_t opacity);

	/**
	 * \brief Get the opacity of a ivi_layer.
	 *
	 * \return opacity if the method call was successful
	 * \return wl_fixed_from_double(0.0) if the method call was failed
	 */
	wl_fixed_t (*layer_get_opacity)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Set the area of a ivi_layer which should be used for the rendering.
	 *
	 * Only this part will be visible.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_source_rectangle)(struct ivi_layout_layer *ivilayer,
					      int32_t x, int32_t y,
					      int32_t width, int32_t height);

	/**
	 * \brief Set the destination area on the display for a ivi_layer.
	 *
	 * The ivi_layer will be scaled and positioned to this rectangle
	 * for rendering
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_destination_rectangle)(struct ivi_layout_layer *ivilayer,
						   int32_t x, int32_t y,
						   int32_t width, int32_t height);

	/**
	 * \brief Sets the horizontal and vertical position of the ivi_layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_position)(struct ivi_layout_layer *ivilayer,
				      int32_t dest_x, int32_t dest_y);

	/**
	 * \brief Get the horizontal and vertical position of the ivi_layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_get_position)(struct ivi_layout_layer *ivilayer,
				      int32_t *dest_x, int32_t *dest_y);

	/**
	 * \brief Set the horizontal and vertical dimension of the layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_dimension)(struct ivi_layout_layer *ivilayer,
				       int32_t dest_width, int32_t dest_height);

	/**
	 * \brief Get the horizontal and vertical dimension of the layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_get_dimension)(struct ivi_layout_layer *ivilayer,
				       int32_t *dest_width, int32_t *dest_height);

	/**
	 * \brief Sets the orientation of a ivi_layer.
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_orientation)(struct ivi_layout_layer *ivilayer,
					 enum wl_output_transform orientation);

	/**
	 * \brief Gets the orientation of a layer.
	 *
	 * \return (enum wl_output_transform)
	 *              if the method call was successful
	 * \return WL_OUTPUT_TRANSFORM_NORMAL if the method call was failed
	 */
	enum wl_output_transform
		(*layer_get_orientation)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief Add a ivi_surface to a ivi_layer which is currently managed by the service
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_add_surface)(struct ivi_layout_layer *ivilayer,
				     struct ivi_layout_surface *addsurf);

	/**
	 * \brief Removes a surface from a layer which is currently managed by the service
	 */
	void (*layer_remove_surface)(struct ivi_layout_layer *ivilayer,
				     struct ivi_layout_surface *remsurf);

	/**
	 * \brief Sets render order of ivi_surfaces within a ivi_layer
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_set_render_order)(struct ivi_layout_layer *ivilayer,
					  struct ivi_layout_surface **pSurface,
					  int32_t number);

	/**
	 * \brief register for notification on property changes of ivi_layer
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*layer_add_notification)(struct ivi_layout_layer *ivilayer,
					  layer_property_notification_func callback,
					  void *userdata);

	/**
	 * \brief remove notification on property changes of ivi_layer
	 */
	void (*layer_remove_notification)(struct ivi_layout_layer *ivilayer);

	/**
	 * \brief set type of transition animation
	 */
	int32_t (*layer_set_transition)(struct ivi_layout_layer *ivilayer,
					enum ivi_layout_transition_type type,
					uint32_t duration);

	/**
	 * screen controller interface
	 */

	/**
	 * \brief get ivi_layout_screen from id of ivi_screen
	 *
	 * \return (struct ivi_layout_screen *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct ivi_layout_screen *
		(*get_screen_from_id)(uint32_t id_screen);

	/**
	 * \brief Get the screen resolution of a specific ivi_screen
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_screen_resolution)(struct ivi_layout_screen *iviscrn,
					 int32_t *pWidth,
					 int32_t *pHeight);

	/**
	 * \brief Get the ivi_screens
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_screens)(int32_t *pLength, struct ivi_layout_screen ***ppArray);

	/**
	 * \brief Get the ivi_screens under the given ivi_layer
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*get_screens_under_layer)(struct ivi_layout_layer *ivilayer,
					   int32_t *pLength,
					   struct ivi_layout_screen ***ppArray);

	/**
	 * \brief Add a ivi_layer to a ivi_screen which is currently managed
	 * by the service
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*screen_add_layer)(struct ivi_layout_screen *iviscrn,
				    struct ivi_layout_layer *addlayer);

	/**
	 * \brief Sets render order of ivi_layers on a ivi_screen
	 *
	 * \return IVI_SUCCEEDED if the method call was successful
	 * \return IVI_FAILED if the method call was failed
	 */
	int32_t (*screen_set_render_order)(struct ivi_layout_screen *iviscrn,
					   struct ivi_layout_layer **pLayer,
					   const int32_t number);

	/**
	 * \brief get weston_output from ivi_layout_screen.
	 *
	 * \return (struct weston_output *)
	 *              if the method call was successful
	 * \return NULL if the method call was failed
	 */
	struct weston_output *(*screen_get_output)(struct ivi_layout_screen *);


	/**
	 * transision animation for layer
	 */
	void (*transition_move_layer_cancel)(struct ivi_layout_layer *layer);
	int32_t (*layer_set_fade_info)(struct ivi_layout_layer* ivilayer,
				       uint32_t is_fade_in,
				       double start_alpha, double end_alpha);

};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IVI_CONTROLLER_INTERFACE_H_ */
