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
 * Management of surfaces and layers grouping these surfaces are common way in
 * In-Vehicle Infotainment system, which integrate several domains in one system.
 * A layer is allocated to a domain in order to control application surfaces
 * grouped to the layer all together.
 */

#ifndef _IVI_LAYOUT_H_
#define _IVI_LAYOUT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "compositor.h"

struct ivi_layout_surface;

struct ivi_layout_interface {
	struct weston_view* (*get_weston_view)(struct ivi_layout_surface *surface);
	void (*surfaceConfigure)(struct ivi_layout_surface *ivisurf,
				 uint32_t width, uint32_t height);
	int32_t (*surfaceSetNativeContent)(struct weston_surface *wl_surface,
					   uint32_t width,
                                           uint32_t height,
                                           uint32_t id_surface);
	struct ivi_layout_surface* (*surfaceCreate)(struct weston_surface *wl_surface,
							      uint32_t id_surface);
	void (*initWithCompositor)(struct weston_compositor *ec);
};

WL_EXPORT struct ivi_layout_interface ivi_layout_interface;

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _IVI_LAYOUT_H_ */
