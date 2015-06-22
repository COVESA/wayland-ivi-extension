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

#ifndef _IVI_LAYOUT_EXPORT_H_
#define _IVI_LAYOUT_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdbool.h"
#include <weston/compositor.h>

#define IVI_SUCCEEDED (0)
#define IVI_FAILED (-1)

struct ivi_layout_layer;
struct ivi_layout_screen;
struct ivi_layout_surface;

struct ivi_layout_surface_properties
{
	wl_fixed_t opacity;
	int32_t source_x;
	int32_t source_y;
	int32_t source_width;
	int32_t source_height;
	int32_t start_x;
	int32_t start_y;
	int32_t start_width;
	int32_t start_height;
	int32_t dest_x;
	int32_t dest_y;
	int32_t dest_width;
	int32_t dest_height;
	enum wl_output_transform orientation;
	bool visibility;
	int32_t transition_type;
	uint32_t transition_duration;
};

struct ivi_layout_layer_properties
{
	wl_fixed_t opacity;
	int32_t source_x;
	int32_t source_y;
	int32_t source_width;
	int32_t source_height;
	int32_t dest_x;
	int32_t dest_y;
	int32_t dest_width;
	int32_t dest_height;
	enum wl_output_transform orientation;
	uint32_t visibility;
	int32_t transition_type;
	uint32_t transition_duration;
	double start_alpha;
	double end_alpha;
	uint32_t is_fade_in;
};

enum ivi_layout_notification_mask {
	IVI_NOTIFICATION_NONE        = 0,
	IVI_NOTIFICATION_OPACITY     = (1 << 1),
	IVI_NOTIFICATION_SOURCE_RECT = (1 << 2),
	IVI_NOTIFICATION_DEST_RECT   = (1 << 3),
	IVI_NOTIFICATION_DIMENSION   = (1 << 4),
	IVI_NOTIFICATION_POSITION    = (1 << 5),
	IVI_NOTIFICATION_ORIENTATION = (1 << 6),
	IVI_NOTIFICATION_VISIBILITY  = (1 << 7),
	IVI_NOTIFICATION_PIXELFORMAT = (1 << 8),
	IVI_NOTIFICATION_ADD         = (1 << 9),
	IVI_NOTIFICATION_REMOVE      = (1 << 10),
	IVI_NOTIFICATION_CONFIGURE   = (1 << 11),
	IVI_NOTIFICATION_ALL         = 0xFFFF
};

enum ivi_layout_transition_type{
	IVI_LAYOUT_TRANSITION_NONE,
	IVI_LAYOUT_TRANSITION_VIEW_DEFAULT,
	IVI_LAYOUT_TRANSITION_VIEW_DEST_RECT_ONLY,
	IVI_LAYOUT_TRANSITION_VIEW_FADE_ONLY,
	IVI_LAYOUT_TRANSITION_LAYER_FADE,
	IVI_LAYOUT_TRANSITION_LAYER_MOVE,
	IVI_LAYOUT_TRANSITION_LAYER_VIEW_ORDER,
	IVI_LAYOUT_TRANSITION_VIEW_MOVE_RESIZE,
	IVI_LAYOUT_TRANSITION_VIEW_RESIZE,
	IVI_LAYOUT_TRANSITION_VIEW_FADE,
	IVI_LAYOUT_TRANSITION_MAX,
};

typedef void (*layer_property_notification_func)(
			struct ivi_layout_layer *ivilayer,
			const struct ivi_layout_layer_properties *,
			enum ivi_layout_notification_mask mask,
			void *userdata);

typedef void (*surface_property_notification_func)(
			struct ivi_layout_surface *ivisurf,
			const struct ivi_layout_surface_properties *,
			enum ivi_layout_notification_mask mask,
			void *userdata);

typedef void (*layer_create_notification_func)(
			struct ivi_layout_layer *ivilayer,
			void *userdata);

typedef void (*layer_remove_notification_func)(
			struct ivi_layout_layer *ivilayer,
			void *userdata);

typedef void (*surface_create_notification_func)(
			struct ivi_layout_surface *ivisurf,
			void *userdata);

typedef void (*surface_remove_notification_func)(
			struct ivi_layout_surface *ivisurf,
			void *userdata);

typedef void (*surface_configure_notification_func)(
			struct ivi_layout_surface *ivisurf,
			void *userdata);

typedef void (*ivi_controller_surface_content_callback)(
			struct ivi_layout_surface *ivisurf,
			int32_t content,
			void *userdata);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IVI_LAYOUT_EXPORT_H_ */
