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

#ifndef IVI_CONTROLLER_SERVER_PROTOCOL_H
#define IVI_CONTROLLER_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

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
 * @set_visibility: Set Visibility
 * @set_opacity: Set Opacity
 * @set_source_rectangle: Set the area of wl_surface which should be used
 *	for the rendering
 * @set_destination_rectangle: Set the destination area of a surface
 *	within a layer for rendering
 * @set_configuration: request new buffer size for application content
 * @set_orientation: Set Orientation
 * @screenshot: Take screenshot
 * @send_stats: (none)
 * @destroy: destroy ivi_controller_surface
 *
 *
 */
struct ivi_controller_surface_interface {
	/**
	 * set_visibility - Set Visibility
	 * @visibility: (none)
	 *
	 *
	 */
	void (*set_visibility)(struct wl_client *client,
			       struct wl_resource *resource,
			       uint32_t visibility);
	/**
	 * set_opacity - Set Opacity
	 * @opacity: (none)
	 *
	 *
	 */
	void (*set_opacity)(struct wl_client *client,
			    struct wl_resource *resource,
			    wl_fixed_t opacity);
	/**
	 * set_source_rectangle - Set the area of wl_surface which should
	 *	be used for the rendering
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * x: horizontal start position of the used area y: vertical
	 * start position of the used area width : width of the area
	 * height: height of the area
	 */
	void (*set_source_rectangle)(struct wl_client *client,
				     struct wl_resource *resource,
				     int32_t x,
				     int32_t y,
				     int32_t width,
				     int32_t height);
	/**
	 * set_destination_rectangle - Set the destination area of a
	 *	surface within a layer for rendering
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * Set the destination area of a wl_surface within a layer for
	 * rendering. The surface will be scaled to this rectangle for
	 * rendering. x: horizontal start position of the used area y:
	 * vertical start position of the used area width : width of the
	 * area height: height of the area
	 */
	void (*set_destination_rectangle)(struct wl_client *client,
					  struct wl_resource *resource,
					  int32_t x,
					  int32_t y,
					  int32_t width,
					  int32_t height);
	/**
	 * set_configuration - request new buffer size for application
	 *	content
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*set_configuration)(struct wl_client *client,
				  struct wl_resource *resource,
				  int32_t width,
				  int32_t height);
	/**
	 * set_orientation - Set Orientation
	 * @orientation: (none)
	 *
	 *
	 */
	void (*set_orientation)(struct wl_client *client,
				struct wl_resource *resource,
				int32_t orientation);
	/**
	 * screenshot - Take screenshot
	 * @filename: (none)
	 *
	 *
	 */
	void (*screenshot)(struct wl_client *client,
			   struct wl_resource *resource,
			   const char *filename);
	/**
	 * send_stats - (none)
	 */
	void (*send_stats)(struct wl_client *client,
			   struct wl_resource *resource);
	/**
	 * destroy - destroy ivi_controller_surface
	 * @destroy_scene_object: (none)
	 *
	 *
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource,
			int32_t destroy_scene_object);
};

#define IVI_CONTROLLER_SURFACE_VISIBILITY	0
#define IVI_CONTROLLER_SURFACE_OPACITY	1
#define IVI_CONTROLLER_SURFACE_SOURCE_RECTANGLE	2
#define IVI_CONTROLLER_SURFACE_DESTINATION_RECTANGLE	3
#define IVI_CONTROLLER_SURFACE_CONFIGURATION	4
#define IVI_CONTROLLER_SURFACE_ORIENTATION	5
#define IVI_CONTROLLER_SURFACE_PIXELFORMAT	6
#define IVI_CONTROLLER_SURFACE_LAYER	7
#define IVI_CONTROLLER_SURFACE_STATS	8
#define IVI_CONTROLLER_SURFACE_DESTROYED	9
#define IVI_CONTROLLER_SURFACE_CONTENT	10

static inline void
ivi_controller_surface_send_visibility(struct wl_resource *resource_, int32_t visibility)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_VISIBILITY, visibility);
}

static inline void
ivi_controller_surface_send_opacity(struct wl_resource *resource_, wl_fixed_t opacity)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_OPACITY, opacity);
}

static inline void
ivi_controller_surface_send_source_rectangle(struct wl_resource *resource_, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_SOURCE_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_surface_send_destination_rectangle(struct wl_resource *resource_, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_DESTINATION_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_surface_send_configuration(struct wl_resource *resource_, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_CONFIGURATION, width, height);
}

static inline void
ivi_controller_surface_send_orientation(struct wl_resource *resource_, int32_t orientation)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_ORIENTATION, orientation);
}

static inline void
ivi_controller_surface_send_pixelformat(struct wl_resource *resource_, int32_t pixelformat)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_PIXELFORMAT, pixelformat);
}

static inline void
ivi_controller_surface_send_layer(struct wl_resource *resource_, struct wl_resource *layer)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_LAYER, layer);
}

static inline void
ivi_controller_surface_send_stats(struct wl_resource *resource_, uint32_t redraw_count, uint32_t frame_count, uint32_t update_count, uint32_t pid)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_STATS, redraw_count, frame_count, update_count, pid);
}

static inline void
ivi_controller_surface_send_destroyed(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_DESTROYED);
}

static inline void
ivi_controller_surface_send_content(struct wl_resource *resource_, int32_t content_state)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE_CONTENT, content_state);
}

/**
 * ivi_controller_layer - Request property change of ivi_layer and
 *	add/remove ivi_surface from ivi_layer to server
 * @set_visibility: Set Visibility
 * @set_opacity: Set Opacity
 * @set_source_rectangle: Set the area of layer which should be used for
 *	the rendering
 * @set_destination_rectangle: Set the destination area on the display
 *	for a layer
 * @set_configuration: Set new configuration for layer
 * @set_orientation: Set Orientation
 * @screenshot: Take screenshot
 * @clear_surfaces: remove all ivi_surfaces from a layer
 * @add_surface: add a ivi_surface to top order of a ivi_layer
 * @remove_surface: remove a ivi_surface from a layer
 * @set_render_order: Set Render Order
 * @destroy: destroy ivi_controller_layer
 *
 *
 */
struct ivi_controller_layer_interface {
	/**
	 * set_visibility - Set Visibility
	 * @visibility: (none)
	 *
	 *
	 */
	void (*set_visibility)(struct wl_client *client,
			       struct wl_resource *resource,
			       uint32_t visibility);
	/**
	 * set_opacity - Set Opacity
	 * @opacity: (none)
	 *
	 *
	 */
	void (*set_opacity)(struct wl_client *client,
			    struct wl_resource *resource,
			    wl_fixed_t opacity);
	/**
	 * set_source_rectangle - Set the area of layer which should be
	 *	used for the rendering
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * x: horizontal start position of the used area y: vertical
	 * start position of the used area width : width of the area
	 * height: height of the area
	 */
	void (*set_source_rectangle)(struct wl_client *client,
				     struct wl_resource *resource,
				     int32_t x,
				     int32_t y,
				     int32_t width,
				     int32_t height);
	/**
	 * set_destination_rectangle - Set the destination area on the
	 *	display for a layer
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * Set the destination area on the display for a layer. The layer
	 * will be scaled and positioned to this rectangle for rendering x:
	 * horizontal start position of the used area y: vertical start
	 * position of the used area width : width of the area height:
	 * height of the area
	 */
	void (*set_destination_rectangle)(struct wl_client *client,
					  struct wl_resource *resource,
					  int32_t x,
					  int32_t y,
					  int32_t width,
					  int32_t height);
	/**
	 * set_configuration - Set new configuration for layer
	 * @width: (none)
	 * @height: (none)
	 *
	 *
	 */
	void (*set_configuration)(struct wl_client *client,
				  struct wl_resource *resource,
				  int32_t width,
				  int32_t height);
	/**
	 * set_orientation - Set Orientation
	 * @orientation: (none)
	 *
	 *
	 */
	void (*set_orientation)(struct wl_client *client,
				struct wl_resource *resource,
				int32_t orientation);
	/**
	 * screenshot - Take screenshot
	 * @filename: (none)
	 *
	 *
	 */
	void (*screenshot)(struct wl_client *client,
			   struct wl_resource *resource,
			   const char *filename);
	/**
	 * clear_surfaces - remove all ivi_surfaces from a layer
	 *
	 *
	 */
	void (*clear_surfaces)(struct wl_client *client,
			       struct wl_resource *resource);
	/**
	 * add_surface - add a ivi_surface to top order of a ivi_layer
	 * @surface: (none)
	 *
	 *
	 */
	void (*add_surface)(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *surface);
	/**
	 * remove_surface - remove a ivi_surface from a layer
	 * @surface: (none)
	 *
	 *
	 */
	void (*remove_surface)(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *surface);
	/**
	 * set_render_order - Set Render Order
	 * @id_surfaces: (none)
	 *
	 *
	 */
	void (*set_render_order)(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_array *id_surfaces);
	/**
	 * destroy - destroy ivi_controller_layer
	 * @destroy_scene_object: (none)
	 *
	 *
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource,
			int32_t destroy_scene_object);
};

#define IVI_CONTROLLER_LAYER_VISIBILITY	0
#define IVI_CONTROLLER_LAYER_OPACITY	1
#define IVI_CONTROLLER_LAYER_SOURCE_RECTANGLE	2
#define IVI_CONTROLLER_LAYER_DESTINATION_RECTANGLE	3
#define IVI_CONTROLLER_LAYER_CONFIGURATION	4
#define IVI_CONTROLLER_LAYER_ORIENTATION	5
#define IVI_CONTROLLER_LAYER_SCREEN	6
#define IVI_CONTROLLER_LAYER_DESTROYED	7

static inline void
ivi_controller_layer_send_visibility(struct wl_resource *resource_, int32_t visibility)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_VISIBILITY, visibility);
}

static inline void
ivi_controller_layer_send_opacity(struct wl_resource *resource_, wl_fixed_t opacity)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_OPACITY, opacity);
}

static inline void
ivi_controller_layer_send_source_rectangle(struct wl_resource *resource_, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_SOURCE_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_layer_send_destination_rectangle(struct wl_resource *resource_, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_DESTINATION_RECTANGLE, x, y, width, height);
}

static inline void
ivi_controller_layer_send_configuration(struct wl_resource *resource_, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_CONFIGURATION, width, height);
}

static inline void
ivi_controller_layer_send_orientation(struct wl_resource *resource_, int32_t orientation)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_ORIENTATION, orientation);
}

static inline void
ivi_controller_layer_send_screen(struct wl_resource *resource_, struct wl_resource *screen)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_SCREEN, screen);
}

static inline void
ivi_controller_layer_send_destroyed(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER_DESTROYED);
}

/**
 * ivi_controller_screen - Request add/remove layer from ivi_layer to
 *	server
 * @destroy: destroy ivi_controller_screen
 * @clear: remove all ivi_layers from wl_output
 * @add_layer: add a ivi_layer to top order of a wl_output
 * @screenshot: Take screenshot
 * @set_render_order: Set Render Order
 *
 *
 */
struct ivi_controller_screen_interface {
	/**
	 * destroy - destroy ivi_controller_screen
	 *
	 *
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * clear - remove all ivi_layers from wl_output
	 *
	 *
	 */
	void (*clear)(struct wl_client *client,
		      struct wl_resource *resource);
	/**
	 * add_layer - add a ivi_layer to top order of a wl_output
	 * @layer: (none)
	 *
	 *
	 */
	void (*add_layer)(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *layer);
	/**
	 * screenshot - Take screenshot
	 * @filename: (none)
	 *
	 *
	 */
	void (*screenshot)(struct wl_client *client,
			   struct wl_resource *resource,
			   const char *filename);
	/**
	 * set_render_order - Set Render Order
	 * @id_layers: (none)
	 *
	 *
	 */
	void (*set_render_order)(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_array *id_layers);
};

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
 * @commit_changes: commit changes and request done by client
 * @layer_create: ilm_layerCreateWithDimension
 * @surface_create: create surface controller
 *
 *
 */
struct ivi_controller_interface {
	/**
	 * commit_changes - commit changes and request done by client
	 *
	 *
	 */
	void (*commit_changes)(struct wl_client *client,
			       struct wl_resource *resource);
	/**
	 * layer_create - ilm_layerCreateWithDimension
	 * @id_layer: (none)
	 * @width: (none)
	 * @height: (none)
	 * @id: (none)
	 *
	 *
	 */
	void (*layer_create)(struct wl_client *client,
			     struct wl_resource *resource,
			     uint32_t id_layer,
			     int32_t width,
			     int32_t height,
			     uint32_t id);
	/**
	 * surface_create - create surface controller
	 * @id_surface: (none)
	 * @id: (none)
	 *
	 *
	 */
	void (*surface_create)(struct wl_client *client,
			       struct wl_resource *resource,
			       uint32_t id_surface,
			       uint32_t id);
};

#define IVI_CONTROLLER_SCREEN	0
#define IVI_CONTROLLER_LAYER	1
#define IVI_CONTROLLER_SURFACE	2
#define IVI_CONTROLLER_ERROR	3

static inline void
ivi_controller_send_screen(struct wl_resource *resource_, uint32_t id_screen, struct wl_resource *screen)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SCREEN, id_screen, screen);
}

static inline void
ivi_controller_send_layer(struct wl_resource *resource_, uint32_t id_layer)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_LAYER, id_layer);
}

static inline void
ivi_controller_send_surface(struct wl_resource *resource_, uint32_t id_surface)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_SURFACE, id_surface);
}

static inline void
ivi_controller_send_error(struct wl_resource *resource_, int32_t object_id, int32_t object_type, int32_t error_code, const char *error_text)
{
	wl_resource_post_event(resource_, IVI_CONTROLLER_ERROR, object_id, object_type, error_code, error_text);
}

#ifdef  __cplusplus
}
#endif

#endif
