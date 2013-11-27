/*
 * Copyright (C) 2013 DENSO CORPORATION
 * Copyright (c) 2013 BMW Car IT GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IVI_CONTROLLER_CLIENT_PROTOCOL_H
#define IVI_CONTROLLER_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct ivi_controller_surface;
struct ivi_controller_layer;
struct ivi_controller_screen;
struct ivi_controller;

extern const struct wl_interface ivi_controller_surface_interface;
extern const struct wl_interface ivi_controller_layer_interface;
extern const struct wl_interface ivi_controller_screen_interface;
extern const struct wl_interface ivi_controller_interface;

#ifndef IVI_CONTROLLER_SURFACE_ORIENTATION_ENUM
#define IVI_CONTROLLER_SURFACE_ORIENTATION_ENUM
/**
 * ivi_controller_surface_orientation - orientation presets in degrees
 * @IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES: not rotated
 * @IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES: rotated 90 degrees
 *	clockwise
 * @IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES: rotated 180 degrees
 *	clockwise
 * @IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES: rotated 270 degrees
 *	clockwise
 *
 *
 */
enum ivi_controller_surface_orientation {
	IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES = 0,
	IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES = 1,
	IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES = 2,
	IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES = 3,
};
#endif /* IVI_CONTROLLER_SURFACE_ORIENTATION_ENUM */

#ifndef IVI_CONTROLLER_SURFACE_PIXELFORMAT_ENUM
#define IVI_CONTROLLER_SURFACE_PIXELFORMAT_ENUM
/**
 * ivi_controller_surface_pixelformat - pixel format values
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_R_8: 8 bit luminance surface
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGB_888: 24 bit rgb surface
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_8888: 24 bit rgb surface with
 *	8 bit alpha
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGB_565: 16 bit rgb surface
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_5551: 16 bit rgb surface with
 *	binary mask
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_6661: 18 bit rgb surface with
 *	binary mask
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_4444: 12 bit rgb surface with
 *	4 bit alpha
 * @IVI_CONTROLLER_SURFACE_PIXELFORMAT_UNKNOWN: unknown
 *
 *
 */
enum ivi_controller_surface_pixelformat {
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_R_8 = 0,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGB_888 = 1,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_8888 = 2,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGB_565 = 3,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_5551 = 4,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_6661 = 5,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_4444 = 6,
	IVI_CONTROLLER_SURFACE_PIXELFORMAT_UNKNOWN = 7,
};
#endif /* IVI_CONTROLLER_SURFACE_PIXELFORMAT_ENUM */

#ifndef IVI_CONTROLLER_SURFACE_CONTENT_STATE_ENUM
#define IVI_CONTROLLER_SURFACE_CONTENT_STATE_ENUM
/**
 * ivi_controller_surface_content_state - all possible states of content
 *	for a surface
 * @IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_AVAILABLE: application
 *	provided wl_surface for this surface
 * @IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED: wl_surface was
 *	removed for this surface
 *
 *
 */
enum ivi_controller_surface_content_state {
	IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_AVAILABLE = 1,
	IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED = 2,
};
#endif /* IVI_CONTROLLER_SURFACE_CONTENT_STATE_ENUM */

/**
 * ivi_controller_surface - Request property change of ivi_surface to
 *	server
 * @visibility: sent in response to set_visibility
 * @opacity: sent in response to set_opacity
 * @source_rectangle: sent in response to set_source_rectangle
 * @destination_rectangle: sent in response to set_destination_rectangle
 * @configuration: sent in response to set_configuration
 * @orientation: sent in response to set_orientation
 * @pixelformat: pixelformat
 * @layer: Receive a ivi_layer this ivi_surface belongs
 * @stats: sent in response to send_stats
 * @destroyed: destroyed surface event
 * @content: content state for surface has changed
 *
 *
 */
struct ivi_controller_surface_listener {
	/**
	 * visibility - sent in response to set_visibility
	 * @visibility: (none)
	 *
	 *
	 */
	void (*visibility)(void *data,
			   struct ivi_controller_surface *ivi_controller_surface,
			   int32_t visibility);
	/**
	 * opacity - sent in response to set_opacity
	 * @opacity: (none)
	 *
	 *
	 */
	void (*opacity)(void *data,
			struct ivi_controller_surface *ivi_controller_surface,
			wl_fixed_t opacity);
	/**
	 * source_rectangle - sent in response to set_source_rectangle
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*source_rectangle)(void *data,
				 struct ivi_controller_surface *ivi_controller_surface,
				 int32_t x,
				 int32_t y,
				 int32_t width,
				 int32_t height);
	/**
	 * destination_rectangle - sent in response to
	 *	set_destination_rectangle
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*destination_rectangle)(void *data,
				      struct ivi_controller_surface *ivi_controller_surface,
				      int32_t x,
				      int32_t y,
				      int32_t width,
				      int32_t height);
	/**
	 * configuration - sent in response to set_configuration
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*configuration)(void *data,
			      struct ivi_controller_surface *ivi_controller_surface,
			      int32_t width,
			      int32_t height);
	/**
	 * orientation - sent in response to set_orientation
	 * @orientation: (none)
	 *
	 *
	 */
	void (*orientation)(void *data,
			    struct ivi_controller_surface *ivi_controller_surface,
			    int32_t orientation);
	/**
	 * pixelformat - pixelformat
	 * @pixelformat: (none)
	 *
	 *
	 */
	void (*pixelformat)(void *data,
			    struct ivi_controller_surface *ivi_controller_surface,
			    int32_t pixelformat);
	/**
	 * layer - Receive a ivi_layer this ivi_surface belongs
	 * @layer: (none)
	 *
	 *
	 */
	void (*layer)(void *data,
		      struct ivi_controller_surface *ivi_controller_surface,
		      struct ivi_controller_layer *layer);
	/**
	 * stats - sent in response to send_stats
	 * @redraw_count: (none)
	 * @frame_count: (none)
	 * @update_count: (none)
	 * @pid: (none)
	 *
	 *
	 */
	void (*stats)(void *data,
		      struct ivi_controller_surface *ivi_controller_surface,
		      uint32_t redraw_count,
		      uint32_t frame_count,
		      uint32_t update_count,
		      uint32_t pid);
	/**
	 * destroyed - destroyed surface event
	 *
	 *
	 */
	void (*destroyed)(void *data,
			  struct ivi_controller_surface *ivi_controller_surface);
	/**
	 * content - content state for surface has changed
	 * @content_state: (none)
	 *
	 *
	 */
	void (*content)(void *data,
			struct ivi_controller_surface *ivi_controller_surface,
			int32_t content_state);
};

static inline int
ivi_controller_surface_add_listener(struct ivi_controller_surface *ivi_controller_surface,
				    const struct ivi_controller_surface_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ivi_controller_surface,
				     (void (**)(void)) listener, data);
}

#define IVI_CONTROLLER_SURFACE_SET_VISIBILITY	0
#define IVI_CONTROLLER_SURFACE_SET_OPACITY	1
#define IVI_CONTROLLER_SURFACE_SET_SOURCE_RECTANGLE	2
#define IVI_CONTROLLER_SURFACE_SET_DESTINATION_RECTANGLE	3
#define IVI_CONTROLLER_SURFACE_SET_CONFIGURATION	4
#define IVI_CONTROLLER_SURFACE_SET_ORIENTATION	5
#define IVI_CONTROLLER_SURFACE_SCREENSHOT	6
#define IVI_CONTROLLER_SURFACE_SEND_STATS	7
#define IVI_CONTROLLER_SURFACE_DESTROY	8

static inline void
ivi_controller_surface_set_user_data(struct ivi_controller_surface *ivi_controller_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_controller_surface, user_data);
}

static inline void *
ivi_controller_surface_get_user_data(struct ivi_controller_surface *ivi_controller_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_controller_surface);
}

static inline void
ivi_controller_surface_set_visibility(struct ivi_controller_surface *ivi_controller_surface, uint32_t visibility)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_VISIBILITY, visibility);
}

static inline void
ivi_controller_surface_set_opacity(struct ivi_controller_surface *ivi_controller_surface, wl_fixed_t opacity)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_OPACITY, opacity);
}

static inline void
ivi_controller_surface_set_source_rectangle(struct ivi_controller_surface *ivi_controller_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_SOURCE_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_surface_set_destination_rectangle(struct ivi_controller_surface *ivi_controller_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_DESTINATION_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_surface_set_configuration(struct ivi_controller_surface *ivi_controller_surface, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_CONFIGURATION, width, height);
}

static inline void
ivi_controller_surface_set_orientation(struct ivi_controller_surface *ivi_controller_surface, int32_t orientation)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SET_ORIENTATION, orientation);
}

static inline void
ivi_controller_surface_screenshot(struct ivi_controller_surface *ivi_controller_surface, const char *filename)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SCREENSHOT, filename);
}

static inline void
ivi_controller_surface_send_stats(struct ivi_controller_surface *ivi_controller_surface)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_SEND_STATS);
}

static inline void
ivi_controller_surface_destroy(struct ivi_controller_surface *ivi_controller_surface, int32_t destroy_scene_object)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_surface,
			 IVI_CONTROLLER_SURFACE_DESTROY, destroy_scene_object);

	wl_proxy_destroy((struct wl_proxy *) ivi_controller_surface);
}

/**
 * ivi_controller_layer - Request property change of ivi_layer and
 *	add/remove ivi_surface from ivi_layer to server
 * @visibility: sent in response to set_visibility
 * @opacity: sent in response to set_opacity
 * @source_rectangle: sent in response to set_source_rectangle
 * @destination_rectangle: sent in response to set_destination_rectangle
 * @configuration: sent in response to set_configuration
 * @orientation: sent in response to set_orientation
 * @screen: Receive a wl_output this ivi_layer belongs
 * @destroyed: destroyed layer event
 *
 *
 */
struct ivi_controller_layer_listener {
	/**
	 * visibility - sent in response to set_visibility
	 * @visibility: (none)
	 *
	 *
	 */
	void (*visibility)(void *data,
			   struct ivi_controller_layer *ivi_controller_layer,
			   int32_t visibility);
	/**
	 * opacity - sent in response to set_opacity
	 * @opacity: (none)
	 *
	 *
	 */
	void (*opacity)(void *data,
			struct ivi_controller_layer *ivi_controller_layer,
			wl_fixed_t opacity);
	/**
	 * source_rectangle - sent in response to set_source_rectangle
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*source_rectangle)(void *data,
				 struct ivi_controller_layer *ivi_controller_layer,
				 int32_t x,
				 int32_t y,
				 int32_t width,
				 int32_t height);
	/**
	 * destination_rectangle - sent in response to
	 *	set_destination_rectangle
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*destination_rectangle)(void *data,
				      struct ivi_controller_layer *ivi_controller_layer,
				      int32_t x,
				      int32_t y,
				      int32_t width,
				      int32_t height);
	/**
	 * configuration - sent in response to set_configuration
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*configuration)(void *data,
			      struct ivi_controller_layer *ivi_controller_layer,
			      int32_t width,
			      int32_t height);
	/**
	 * orientation - sent in response to set_orientation
	 * @orientation: (none)
	 *
	 *
	 */
	void (*orientation)(void *data,
			    struct ivi_controller_layer *ivi_controller_layer,
			    int32_t orientation);
	/**
	 * screen - Receive a wl_output this ivi_layer belongs
	 * @screen: (none)
	 *
	 *
	 */
	void (*screen)(void *data,
		       struct ivi_controller_layer *ivi_controller_layer,
		       struct wl_output *screen);
	/**
	 * destroyed - destroyed layer event
	 *
	 *
	 */
	void (*destroyed)(void *data,
			  struct ivi_controller_layer *ivi_controller_layer);
};

static inline int
ivi_controller_layer_add_listener(struct ivi_controller_layer *ivi_controller_layer,
				  const struct ivi_controller_layer_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ivi_controller_layer,
				     (void (**)(void)) listener, data);
}

#define IVI_CONTROLLER_LAYER_SET_VISIBILITY	0
#define IVI_CONTROLLER_LAYER_SET_OPACITY	1
#define IVI_CONTROLLER_LAYER_SET_SOURCE_RECTANGLE	2
#define IVI_CONTROLLER_LAYER_SET_DESTINATION_RECTANGLE	3
#define IVI_CONTROLLER_LAYER_SET_CONFIGURATION	4
#define IVI_CONTROLLER_LAYER_SET_ORIENTATION	5
#define IVI_CONTROLLER_LAYER_SCREENSHOT	6
#define IVI_CONTROLLER_LAYER_CLEAR_SURFACES	7
#define IVI_CONTROLLER_LAYER_ADD_SURFACE	8
#define IVI_CONTROLLER_LAYER_REMOVE_SURFACE	9
#define IVI_CONTROLLER_LAYER_SET_RENDER_ORDER	10
#define IVI_CONTROLLER_LAYER_DESTROY	11

static inline void
ivi_controller_layer_set_user_data(struct ivi_controller_layer *ivi_controller_layer, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_controller_layer, user_data);
}

static inline void *
ivi_controller_layer_get_user_data(struct ivi_controller_layer *ivi_controller_layer)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_controller_layer);
}

static inline void
ivi_controller_layer_set_visibility(struct ivi_controller_layer *ivi_controller_layer, uint32_t visibility)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_VISIBILITY, visibility);
}

static inline void
ivi_controller_layer_set_opacity(struct ivi_controller_layer *ivi_controller_layer, wl_fixed_t opacity)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_OPACITY, opacity);
}

static inline void
ivi_controller_layer_set_source_rectangle(struct ivi_controller_layer *ivi_controller_layer, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_SOURCE_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_layer_set_destination_rectangle(struct ivi_controller_layer *ivi_controller_layer, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_DESTINATION_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_layer_set_configuration(struct ivi_controller_layer *ivi_controller_layer, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_CONFIGURATION, width, height);
}

static inline void
ivi_controller_layer_set_orientation(struct ivi_controller_layer *ivi_controller_layer, int32_t orientation)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_ORIENTATION, orientation);
}

static inline void
ivi_controller_layer_screenshot(struct ivi_controller_layer *ivi_controller_layer, const char *filename)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SCREENSHOT, filename);
}

static inline void
ivi_controller_layer_clear_surfaces(struct ivi_controller_layer *ivi_controller_layer)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_CLEAR_SURFACES);
}

static inline void
ivi_controller_layer_add_surface(struct ivi_controller_layer *ivi_controller_layer, struct ivi_controller_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_ADD_SURFACE, surface);
}

static inline void
ivi_controller_layer_remove_surface(struct ivi_controller_layer *ivi_controller_layer, struct ivi_controller_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_REMOVE_SURFACE, surface);
}

static inline void
ivi_controller_layer_set_render_order(struct ivi_controller_layer *ivi_controller_layer, struct wl_array *id_surfaces)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_SET_RENDER_ORDER, id_surfaces);
}

static inline void
ivi_controller_layer_destroy(struct ivi_controller_layer *ivi_controller_layer, int32_t destroy_scene_object)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_layer,
			 IVI_CONTROLLER_LAYER_DESTROY, destroy_scene_object);

	wl_proxy_destroy((struct wl_proxy *) ivi_controller_layer);
}

#define IVI_CONTROLLER_SCREEN_DESTROY	0
#define IVI_CONTROLLER_SCREEN_CLEAR	1
#define IVI_CONTROLLER_SCREEN_ADD_LAYER	2
#define IVI_CONTROLLER_SCREEN_SCREENSHOT	3
#define IVI_CONTROLLER_SCREEN_SET_RENDER_ORDER	4

static inline void
ivi_controller_screen_set_user_data(struct ivi_controller_screen *ivi_controller_screen, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_controller_screen, user_data);
}

static inline void *
ivi_controller_screen_get_user_data(struct ivi_controller_screen *ivi_controller_screen)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_controller_screen);
}

static inline void
ivi_controller_screen_destroy(struct ivi_controller_screen *ivi_controller_screen)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_screen,
			 IVI_CONTROLLER_SCREEN_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) ivi_controller_screen);
}

static inline void
ivi_controller_screen_clear(struct ivi_controller_screen *ivi_controller_screen)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_screen,
			 IVI_CONTROLLER_SCREEN_CLEAR);
}

static inline void
ivi_controller_screen_add_layer(struct ivi_controller_screen *ivi_controller_screen, struct ivi_controller_layer *layer)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_screen,
			 IVI_CONTROLLER_SCREEN_ADD_LAYER, layer);
}

static inline void
ivi_controller_screen_screenshot(struct ivi_controller_screen *ivi_controller_screen, const char *filename)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_screen,
			 IVI_CONTROLLER_SCREEN_SCREENSHOT, filename);
}

static inline void
ivi_controller_screen_set_render_order(struct ivi_controller_screen *ivi_controller_screen, struct wl_array *id_layers)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller_screen,
			 IVI_CONTROLLER_SCREEN_SET_RENDER_ORDER, id_layers);
}

#ifndef IVI_CONTROLLER_OBJECT_TYPE_ENUM
#define IVI_CONTROLLER_OBJECT_TYPE_ENUM
/**
 * ivi_controller_object_type - available object types in ivi compositor
 *	scene
 * @IVI_CONTROLLER_OBJECT_TYPE_SURFACE: surface object type
 * @IVI_CONTROLLER_OBJECT_TYPE_LAYER: layer object type
 * @IVI_CONTROLLER_OBJECT_TYPE_SCREEN: screen object type
 *
 *
 */
enum ivi_controller_object_type {
	IVI_CONTROLLER_OBJECT_TYPE_SURFACE = 1,
	IVI_CONTROLLER_OBJECT_TYPE_LAYER = 2,
	IVI_CONTROLLER_OBJECT_TYPE_SCREEN = 3,
};
#endif /* IVI_CONTROLLER_OBJECT_TYPE_ENUM */

#ifndef IVI_CONTROLLER_ERROR_CODE_ENUM
#define IVI_CONTROLLER_ERROR_CODE_ENUM
/**
 * ivi_controller_error_code - possible error codes returned in error
 *	event
 * @IVI_CONTROLLER_ERROR_CODE_UNKNOWN_ERROR: unknown error encountered
 * @IVI_CONTROLLER_ERROR_CODE_FILE_ERROR: file i/o error encountered
 *
 *
 */
enum ivi_controller_error_code {
	IVI_CONTROLLER_ERROR_CODE_UNKNOWN_ERROR = 1,
	IVI_CONTROLLER_ERROR_CODE_FILE_ERROR = 2,
};
#endif /* IVI_CONTROLLER_ERROR_CODE_ENUM */

/**
 * ivi_controller - Interface for central controller of layers and
 *	surfaces
 * @screen: Receive new additional screen controller
 * @layer: Receive id_layer and a layer controller
 * @surface: Receive id_surface and a controller to control ivi_surface
 * @error: request resulted in server-side error
 *
 *
 */
struct ivi_controller_listener {
	/**
	 * screen - Receive new additional screen controller
	 * @id_screen: (none)
	 * @screen: (none)
	 *
	 *
	 */
	void (*screen)(void *data,
		       struct ivi_controller *ivi_controller,
		       uint32_t id_screen,
		       struct ivi_controller_screen *screen);
	/**
	 * layer - Receive id_layer and a layer controller
	 * @id_layer: (none)
	 *
	 *
	 */
	void (*layer)(void *data,
		      struct ivi_controller *ivi_controller,
		      uint32_t id_layer);
	/**
	 * surface - Receive id_surface and a controller to control
	 *	ivi_surface
	 * @id_surface: (none)
	 *
	 *
	 */
	void (*surface)(void *data,
			struct ivi_controller *ivi_controller,
			uint32_t id_surface);
	/**
	 * error - request resulted in server-side error
	 * @object_id: (none)
	 * @object_type: (none)
	 * @error_code: (none)
	 * @error_text: (none)
	 *
	 *
	 */
	void (*error)(void *data,
		      struct ivi_controller *ivi_controller,
		      int32_t object_id,
		      int32_t object_type,
		      int32_t error_code,
		      const char *error_text);
};

static inline int
ivi_controller_add_listener(struct ivi_controller *ivi_controller,
			    const struct ivi_controller_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ivi_controller,
				     (void (**)(void)) listener, data);
}

#define IVI_CONTROLLER_COMMIT_CHANGES	0
#define IVI_CONTROLLER_LAYER_CREATE	1
#define IVI_CONTROLLER_SURFACE_CREATE	2

static inline void
ivi_controller_set_user_data(struct ivi_controller *ivi_controller, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_controller, user_data);
}

static inline void *
ivi_controller_get_user_data(struct ivi_controller *ivi_controller)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_controller);
}

static inline void
ivi_controller_destroy(struct ivi_controller *ivi_controller)
{
	wl_proxy_destroy((struct wl_proxy *) ivi_controller);
}

static inline void
ivi_controller_commit_changes(struct ivi_controller *ivi_controller)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_controller,
			 IVI_CONTROLLER_COMMIT_CHANGES);
}

static inline struct ivi_controller_layer *
ivi_controller_layer_create(struct ivi_controller *ivi_controller, uint32_t id_layer, int32_t width, int32_t height)
{
	struct wl_proxy *id;

	id = wl_proxy_create((struct wl_proxy *) ivi_controller,
			     &ivi_controller_layer_interface);
	if (!id)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) ivi_controller,
			 IVI_CONTROLLER_LAYER_CREATE, id_layer, width, height, id);

	return (struct ivi_controller_layer *) id;
}

static inline struct ivi_controller_surface *
ivi_controller_surface_create(struct ivi_controller *ivi_controller, uint32_t id_surface)
{
	struct wl_proxy *id;

	id = wl_proxy_create((struct wl_proxy *) ivi_controller,
			     &ivi_controller_surface_interface);
	if (!id)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) ivi_controller,
			 IVI_CONTROLLER_SURFACE_CREATE, id_surface, id);

	return (struct ivi_controller_surface *) id;
}

#ifdef  __cplusplus
}
#endif

#endif
