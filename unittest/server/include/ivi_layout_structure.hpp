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

#ifndef IVI_LAYOUT_STRUCTURE
#define IVI_LAYOUT_STRUCTURE

#include <stdint.h>
#include "libweston/libweston.h"
#include "ivi-layout-export.h"
#include "libweston-desktop/libweston-desktop.h"

struct ivi_layout_view {
	struct wl_list link;	/* ivi_layout::view_list */
	struct wl_list surf_link;	/*ivi_layout_surface::view_list */
	struct wl_list pending_link;	/* ivi_layout_layer::pending.view_list */
	struct wl_list order_link;	/* ivi_layout_layer::order.view_list */

	struct weston_view *view;
	struct weston_transform transform;

	struct ivi_layout_surface *ivisurf;
	struct ivi_layout_layer *on_layer;
};

struct ivi_layout_surface {
	struct wl_list link;	/* ivi_layout::surface_list */
	struct wl_signal property_changed;
	int32_t update_count;
	uint32_t id_surface;

	struct ivi_layout *layout;
	struct weston_surface *surface;
	struct weston_desktop_surface *weston_desktop_surface;

	struct ivi_layout_surface_properties prop;

	struct {
		struct ivi_layout_surface_properties prop;
	} pending;

	struct wl_list view_list;	/* ivi_layout_view::surf_link */
};

struct ivi_layout_layer {
	struct wl_list link;	/* ivi_layout::layer_list */
	struct wl_signal property_changed;
	uint32_t id_layer;

	struct ivi_layout *layout;
	struct ivi_layout_screen *on_screen;

	struct ivi_layout_layer_properties prop;

	struct {
		struct ivi_layout_layer_properties prop;
		struct wl_list view_list;	/* ivi_layout_view::pending_link */
		struct wl_list link;	/* ivi_layout_screen::pending.layer_list */
	} pending;

	struct {
		int dirty;
		struct wl_list view_list;	/* ivi_layout_view::order_link */
		struct wl_list link;	/* ivi_layout_screen::order.layer_list */
	} order;

	int32_t ref_count;
};

struct ivi_layout {
	struct weston_compositor *compositor;

	struct wl_list surface_list;	/* ivi_layout_surface::link */
	struct wl_list layer_list;	/* ivi_layout_layer::link */
	struct wl_list screen_list;	/* ivi_layout_screen::link */
	struct wl_list view_list;	/* ivi_layout_view::link */

	struct {
		struct wl_signal created;
		struct wl_signal removed;
	} layer_notification;

	struct {
		struct wl_signal created;
		struct wl_signal removed;
		struct wl_signal configure_changed;
		struct wl_signal configure_desktop_changed;
	} surface_notification;

	struct weston_layer layout_layer;

	struct ivi_layout_transition_set *transitions;
	struct wl_list pending_transition_list;	/* transition_node::link */
};

#endif // IVI_LAYOUT_STRUCTURE
