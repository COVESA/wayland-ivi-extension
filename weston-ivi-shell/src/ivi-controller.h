/*
 * Copyright (C) 2017 Advanced Driver Information Technology Joint Venture GmbH
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

#ifndef WESTON_IVI_SHELL_SRC_IVI_CONTROLLER_H_
#define WESTON_IVI_SHELL_SRC_IVI_CONTROLLER_H_

#include "ivi-wm-server-protocol.h"
#include <weston/ivi-layout-export.h>

struct ivisurface {
    struct wl_list link;
    struct ivishell *shell;
    uint32_t update_count;
    struct ivi_layout_surface *layout_surface;
    const struct ivi_layout_surface_properties *prop;
    struct wl_listener property_changed;
    struct wl_listener surface_destroy_listener;
    struct wl_listener committed;
    struct wl_list notification_list;
    enum ivi_wm_surface_type type;
    uint32_t frame_count;
    struct wl_list accepted_seat_list;
};

struct ivishell {
    struct weston_compositor *compositor;
    const struct ivi_layout_interface *interface;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;

    struct wl_list list_controller;

    struct wl_listener surface_created;
    struct wl_listener surface_removed;
    struct wl_listener surface_configured;

    struct wl_listener layer_created;
    struct wl_listener layer_removed;

    struct wl_listener output_created;
    struct wl_listener output_destroyed;

    struct wl_listener destroy_listener;

    struct wl_array screen_ids;
    uint32_t screen_id_offset;
};

#endif /* WESTON_IVI_SHELL_SRC_IVI_CONTROLLER_H_ */
