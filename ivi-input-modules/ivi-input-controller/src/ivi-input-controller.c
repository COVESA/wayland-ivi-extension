/*
 * Copyright 2015 Codethink Ltd
 * Copyright (C) 2015 Advanced Driver Information Technology Joint Venture GmbH
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

#include <stdlib.h>
#include <string.h>

#include <weston/compositor.h>
#include "ilm_types.h"

#include "ivi-input-server-protocol.h"
#include "ivi-controller-interface.h"

struct seat_ctx {
    struct input_context *input_ctx;
    struct weston_keyboard_grab keyboard_grab;
    struct weston_pointer_grab pointer_grab;
    struct weston_touch_grab touch_grab;
    struct wl_listener updated_caps_listener;
    struct wl_listener destroy_listener;
};

struct surface_ctx {
    struct wl_list link;
    ilmInputDevice focus;
    struct ivi_layout_surface *layout_surface;
    struct wl_array accepted_devices;
    struct input_context *input_context;
};

struct input_controller {
    struct wl_list link;
    struct wl_resource *resource;
    struct wl_client *client;
    uint32_t id;
    struct input_context *input_context;
};

struct input_context {
    struct wl_listener seat_create_listener;
    struct wl_list controller_list;
    struct wl_list surface_list;
    struct weston_compositor *compositor;
    const struct ivi_controller_interface *ivi_controller_interface;
};

static int
get_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    int i;
    char **arr = surface->accepted_devices.data;
    for (i = 0; i < (surface->accepted_devices.size / sizeof(char**)); i++) {
        if (strcmp(arr[i], seat) == 0)
            return i;
    }
    return -1;
}

static int
add_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    char **added_entry;
    const struct ivi_controller_interface *interface =
        surface->input_context->ivi_controller_interface;
    if (get_accepted_seat(surface, seat) >= 0) {
        weston_log("%s: Warning: seat '%s' is already accepted by surface %d\n",
                   __FUNCTION__, seat,
                   interface->get_id_of_surface(surface->layout_surface));
        return 1;
    }
    added_entry = wl_array_add(&surface->accepted_devices, sizeof *added_entry);
    if (added_entry == NULL) {
        weston_log("%s: Failed to expand accepted devices array for "
                   "surface %d\n", __FUNCTION__,
                   interface->get_id_of_surface(surface->layout_surface));
        return 0;
    }
    *added_entry = strdup(seat);
    if (*added_entry == NULL) {
        weston_log("%s: Failed to duplicate seat name '%s'\n",
                   __FUNCTION__, seat);
        return 0;
    }
    return 1;
}

static int
remove_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    int seat_index = get_accepted_seat(surface, seat);
    int i;
    struct wl_array *array = &surface->accepted_devices;
    char **data = array->data;
    const struct ivi_controller_interface *interface =
        surface->input_context->ivi_controller_interface;
    if (seat_index < 0) {
        weston_log("%s: Warning: seat '%s' not found for surface %d\n",
                  __FUNCTION__, seat,
                  interface->get_id_of_surface(surface->layout_surface));
        return 0;
    }
    free(data[seat_index]);
    for (i = seat_index + 1; i < array->size / sizeof(char **); i++)
        data[i - 1] = data[i];
    array->size-= sizeof(char**);

    return 1;
}

static void
send_input_acceptance(struct input_context *ctx, uint32_t surface_id, const char *seat, int32_t accepted)
{
    struct input_controller *controller;
    wl_list_for_each(controller, &ctx->controller_list, link) {
        ivi_input_send_input_acceptance(controller->resource,
                                        surface_id, seat,
                                        accepted);
    }
}

static void
send_input_focus(struct input_context *ctx, t_ilm_surface surface_id,
                 ilmInputDevice device, t_ilm_bool enabled)
{
    struct input_controller *controller;
    wl_list_for_each(controller, &ctx->controller_list, link) {
        ivi_input_send_input_focus(controller->resource, surface_id,
                                   device, enabled);
    }
}

static void
set_weston_focus(struct input_context *ctx, struct surface_ctx *surface_ctx,
                 ilmInputDevice focus, struct weston_seat *seat, t_ilm_bool enabled)
{
    struct weston_surface *w_surface;
    struct weston_view *view;
    struct wl_resource *resource;
    struct wl_client *surface_client;
    uint32_t serial;
    wl_fixed_t x, y;
    struct weston_pointer *pointer = weston_seat_get_pointer(seat);
    struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
    const struct ivi_controller_interface *interface =
              ctx->ivi_controller_interface;

    /* Assume one view per surface */
	w_surface = interface->surface_get_weston_surface(surface_ctx->layout_surface);
    view = wl_container_of(w_surface->views.next, view, surface_link);
	if ((focus & ILM_INPUT_DEVICE_POINTER) &&
		 (pointer != NULL)){
		if ( (view != NULL) && enabled) {
	        weston_view_to_global_fixed(view, wl_fixed_from_int(0),
	            wl_fixed_from_int(0), &x, &y);
	        // move pointer to local (0,0) of the view
	        weston_pointer_move(pointer, x, y);
		    weston_pointer_set_focus(pointer, view,
		        wl_fixed_from_int(0), wl_fixed_from_int(0));
		} else if (pointer->focus == view){
            weston_pointer_set_focus(pointer, NULL,
                wl_fixed_from_int(0), wl_fixed_from_int(0));
		}
	}

	if ((focus & ILM_INPUT_DEVICE_KEYBOARD) &&
		 (keyboard != NULL)) {
		if (w_surface) {
	        surface_client = wl_resource_get_client(w_surface->resource);
	        serial = wl_display_next_serial(ctx->compositor->wl_display);
	        wl_resource_for_each(resource, &keyboard->resource_list) {
	            if (wl_resource_get_client(resource) != surface_client)
	                continue;

	            if (!enabled) {
	                wl_keyboard_send_leave(resource, serial,
	                         w_surface->resource);
	            } else {
                    wl_keyboard_send_enter(resource, serial, w_surface->resource,
                            &keyboard->keys);
	            }
	        }
		}
	}
}

static void
keyboard_grab_key(struct weston_keyboard_grab *grab, uint32_t time,
                  uint32_t key, uint32_t state)
{
    struct seat_ctx *seat_ctx = wl_container_of(grab, seat_ctx, keyboard_grab);
    struct surface_ctx *surf_ctx;
    struct wl_display *display = grab->keyboard->seat->compositor->wl_display;
    const struct ivi_controller_interface *interface =
        seat_ctx->input_ctx->ivi_controller_interface;

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->surface_list, link) {
        struct weston_surface *surface;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        uint32_t serial;
        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_KEYBOARD))
            continue;

        if (get_accepted_seat(surf_ctx, grab->keyboard->seat->seat_name) < 0)
            continue;

        surface = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        surface_client = wl_resource_get_client(surface->resource);
        serial = wl_display_next_serial(display);

        wl_resource_for_each(resource, &grab->keyboard->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_keyboard_send_key(resource, serial, time, key, state);
        }

        wl_resource_for_each(resource, &grab->keyboard->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_keyboard_send_key(resource, serial, time, key, state);
        }
    }
}

static void
keyboard_grab_modifiers(struct weston_keyboard_grab *grab, uint32_t serial,
                        uint32_t mods_depressed, uint32_t mods_latched,
                        uint32_t mods_locked, uint32_t group)
{
    struct seat_ctx *seat_ctx = wl_container_of(grab, seat_ctx, keyboard_grab);
    struct surface_ctx *surf_ctx;
    struct wl_display *display = grab->keyboard->seat->compositor->wl_display;
    const struct ivi_controller_interface *interface =
        seat_ctx->input_ctx->ivi_controller_interface;

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->surface_list, link) {
        struct weston_surface *surface;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        uint32_t serial;

        /* Keyboard modifiers go to surfaces with pointer focus as well */
        if (!(surf_ctx->focus
              & (ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_POINTER)))
            continue;

        if (get_accepted_seat(surf_ctx, grab->keyboard->seat->seat_name) < 0)
            continue;

        surface = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        surface_client = wl_resource_get_client(surface->resource);
        serial = wl_display_next_serial(display);

        wl_resource_for_each(resource, &grab->keyboard->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_keyboard_send_modifiers(resource, serial, mods_depressed,
			               mods_latched, mods_locked, group);
        }

        wl_resource_for_each(resource, &grab->keyboard->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_keyboard_send_modifiers(resource, serial, mods_depressed,
			               mods_latched, mods_locked, group);
        }
    }
}

static void
keyboard_grab_cancel(struct weston_keyboard_grab *grab)
{
}

static struct weston_keyboard_grab_interface keyboard_grab_interface = {
    keyboard_grab_key,
    keyboard_grab_modifiers,
    keyboard_grab_cancel
};

static void
pointer_grab_focus(struct weston_pointer_grab *grab)
{
}

static void
pointer_grab_motion(struct weston_pointer_grab *grab, uint32_t time,
                    wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, pointer_grab);
    struct surface_ctx *surf_ctx;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    weston_pointer_move(grab->pointer, x, y);

    /* Get coordinates relative to the surface the pointer is in.
     * This might cause weirdness if there are multiple surfaces
     * that are accepted by this pointer's seat and have focus */

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        struct weston_view *view;
        wl_fixed_t sx, sy;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_POINTER))
            continue;

        if (get_accepted_seat(surf_ctx, grab->pointer->seat->seat_name) < 0)
            continue;

        /* Assume one view per surface */
        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        view = wl_container_of(surf->views.next, view, surface_link);

        if (view == grab->pointer->focus) {

            /* Do not send motion events for coordinates outside the surface */
            weston_view_from_global_fixed(view, x, y, &sx, &sy);
            if ((!pixman_region32_contains_point(&surf->input, wl_fixed_to_int(sx),
                                                wl_fixed_to_int(sy), NULL))
                 && (grab->pointer->button_count == 0))
                continue;

            surface_client = wl_resource_get_client(surf->resource);

            wl_resource_for_each(resource, &grab->pointer->focus_resource_list) {
                if (wl_resource_get_client(resource) != surface_client)
                    continue;

                wl_pointer_send_motion(resource, time, sx, sy);
            }
            return;
        }
    }
}

static void
pointer_grab_button(struct weston_pointer_grab *grab, uint32_t time,
                    uint32_t button, uint32_t state)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, pointer_grab);
    struct weston_pointer *pointer = grab->pointer;
    struct weston_compositor *compositor = pointer->seat->compositor;
    struct wl_display *display = compositor->wl_display;
    struct surface_ctx *surf_ctx;
    wl_fixed_t sx, sy;
    struct weston_view *picked_view, *w_view, *old_focus;
    struct weston_surface *w_surf;
    struct wl_resource *resource;
    struct wl_client *surface_client;
    uint32_t serial;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    picked_view = weston_compositor_pick_view(compositor, pointer->x, pointer->y,
                                       &sx, &sy);
    if (picked_view == NULL)
        return;

    /* If a button press, set pointer focus to this surface */
    if ((grab->pointer->focus != picked_view) &&
        (state == WL_POINTER_BUTTON_STATE_PRESSED)){
        old_focus = grab->pointer->focus;
        /* search for the picked view in layout surfaces */
        wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
            w_surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
            w_view = wl_container_of(w_surf->views.next, w_view, surface_link);

            if (get_accepted_seat(surf_ctx, grab->pointer->seat->seat_name) < 0)
                continue;

            if (picked_view->surface == w_surf) {
                /* Correct layout surface is found*/
                surf_ctx->focus |= ILM_INPUT_DEVICE_POINTER;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_POINTER, ILM_TRUE);

                weston_pointer_set_focus(grab->pointer, picked_view, sx, sy);

            } else if (old_focus == w_view){
                /* Send focus lost event to the surface which has lost the focus*/
                surf_ctx->focus &= ~ILM_INPUT_DEVICE_POINTER;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_POINTER, ILM_FALSE);
            }
        }
    }

    /* Send to surfaces that have pointer focus */
    if (grab->pointer->focus == picked_view) {
        surface_client = wl_resource_get_client(grab->pointer->focus->surface->resource);
        serial = wl_display_next_serial(display);

        wl_resource_for_each(resource, &grab->pointer->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_pointer_send_button(resource, serial, time, button, state);
        }
    }
}

static void
pointer_grab_cancel(struct weston_pointer_grab *grab)
{
}

static struct weston_pointer_grab_interface pointer_grab_interface = {
    pointer_grab_focus,
    pointer_grab_motion,
    pointer_grab_button,
    pointer_grab_cancel
};

static void
touch_grab_down(struct weston_touch_grab *grab, uint32_t time, int touch_id,
                wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct wl_display *display = grab->touch->seat->compositor->wl_display;
    struct surface_ctx *surf_ctx;
    wl_fixed_t sx, sy;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    /* if touch device has no focused view, there is nothing to do*/
    if (grab->touch->focus == NULL)
        return;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct weston_view *view;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        uint32_t serial;

        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) < 0)
            continue;

        /* Touches set touch focus */
        if (grab->touch->num_tp == 1) {
            if (surf == grab->touch->focus->surface) {
                surf_ctx->focus |= ILM_INPUT_DEVICE_TOUCH;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_TOUCH, ILM_TRUE);
            } else {
                surf_ctx->focus &= ~ILM_INPUT_DEVICE_TOUCH;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_TOUCH, ILM_FALSE);
            }
        }

        /* This code below is slightly redundant, since we have already
         * decided only one surface has touch focus */
        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        /* Assume one view per surface */
        view = wl_container_of(surf->views.next, view, surface_link);
        weston_view_from_global_fixed(view, x, y, &sx, &sy);

        surface_client = wl_resource_get_client(surf->resource);
        serial = wl_display_next_serial(display);
        wl_resource_for_each(resource, &grab->touch->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_down(resource, serial, time, surf->resource,
                               touch_id, sx, sy);
        }
        wl_resource_for_each(resource, &grab->touch->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_down(resource, serial, time, surf->resource,
                               touch_id, sx, sy);
        }
    }


}

static void
touch_grab_up(struct weston_touch_grab *grab, uint32_t time, int touch_id)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct wl_display *display = grab->touch->seat->compositor->wl_display;
    struct surface_ctx *surf_ctx;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        uint32_t serial;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) < 0)
            continue;

        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        surface_client = wl_resource_get_client(surf->resource);
        serial = wl_display_next_serial(display);
        wl_resource_for_each(resource, &grab->touch->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_up(resource, serial, time, touch_id);
        }

        wl_resource_for_each(resource, &grab->touch->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_up(resource, serial, time, touch_id);
        }

        /* Touches unset touch focus */
        if (grab->touch->num_tp == 0) {
            if (surf == grab->touch->focus->surface)
                surf_ctx->focus &= ~ILM_INPUT_DEVICE_TOUCH;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_TOUCH, ILM_FALSE);
        }
    }
}

static void
touch_grab_motion(struct weston_touch_grab *grab, uint32_t time, int touch_id,
                  wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct surface_ctx *surf_ctx;
    wl_fixed_t sx, sy;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct weston_view *view;
        struct wl_resource *resource;
        struct wl_client *surface_client;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) < 0)
            continue;

        /* Assume one view per surface */
        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        view = wl_container_of(surf->views.next, view, surface_link);
        weston_view_from_global_fixed(view, x, y, &sx, &sy);

        surface_client = wl_resource_get_client(surf->resource);
        wl_resource_for_each(resource, &grab->touch->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_motion(resource, time, touch_id, sx, sy);
        }

        wl_resource_for_each(resource, &grab->touch->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_motion(resource, time, touch_id, sx, sy);
        }
    }
}

static void
touch_grab_frame(struct weston_touch_grab *grab)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct surface_ctx *surf_ctx;
    const struct ivi_controller_interface *interface =
        seat->input_ctx->ivi_controller_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct wl_resource *resource;
        struct wl_client *surface_client;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) < 0)
            continue;

        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        surface_client = wl_resource_get_client(surf->resource);
        wl_resource_for_each(resource, &grab->touch->resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_frame(resource);
        }

        wl_resource_for_each(resource, &grab->touch->focus_resource_list) {
            if (wl_resource_get_client(resource) != surface_client)
                continue;

            wl_touch_send_frame(resource);
        }
    }
}

static void
touch_grab_cancel(struct weston_touch_grab *grab)
{
}

static struct weston_touch_grab_interface touch_grab_interface = {
    touch_grab_down,
    touch_grab_up,
    touch_grab_motion,
    touch_grab_frame,
    touch_grab_cancel
};

static uint32_t
get_seat_capabilities(struct weston_seat *seat)
{
    struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
    struct weston_pointer *pointer = weston_seat_get_pointer(seat);
    struct weston_touch *touch = weston_seat_get_touch(seat);
    uint32_t caps = 0;

    if (keyboard != NULL)
        caps |= ILM_INPUT_DEVICE_KEYBOARD;
    if (pointer != NULL)
        caps |= ILM_INPUT_DEVICE_POINTER;
    if (touch != NULL)
        caps |= ILM_INPUT_DEVICE_TOUCH;
    return caps;
}

static void
handle_seat_updated_caps(struct wl_listener *listener, void *data)
{
    struct weston_seat *seat = data;
    struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
    struct weston_pointer *pointer = weston_seat_get_pointer(seat);
    struct weston_touch *touch = weston_seat_get_touch(seat);
    struct seat_ctx *ctx = wl_container_of(listener, ctx,
                                           updated_caps_listener);
    struct input_controller *controller;

    if (keyboard && keyboard != ctx->keyboard_grab.keyboard) {
        weston_keyboard_start_grab(keyboard, &ctx->keyboard_grab);
    }
    if (pointer && pointer != ctx->pointer_grab.pointer) {
        weston_pointer_start_grab(pointer, &ctx->pointer_grab);
    }
    if (touch && touch != ctx->touch_grab.touch) {
        weston_touch_start_grab(touch, &ctx->touch_grab);
    }

    wl_list_for_each(controller, &ctx->input_ctx->controller_list, link) {
        ivi_input_send_seat_capabilities(controller->resource,
                                         seat->seat_name,
                                         get_seat_capabilities(seat));
    }
}

static void
handle_seat_destroy(struct wl_listener *listener, void *data)
{
    struct seat_ctx *ctx = wl_container_of(listener, ctx, destroy_listener);
    struct weston_seat *seat = data;
    struct input_controller *controller;

    if (ctx->keyboard_grab.keyboard)
        keyboard_grab_cancel(&ctx->keyboard_grab);
    if (ctx->pointer_grab.pointer)
        pointer_grab_cancel(&ctx->pointer_grab);
    if (ctx->touch_grab.touch)
        touch_grab_cancel(&ctx->touch_grab);

    wl_list_for_each(controller, &ctx->input_ctx->controller_list, link) {
        ivi_input_send_seat_destroyed(controller->resource,
                                      seat->seat_name);
    }

    free(ctx);
}

static void
handle_seat_create(struct wl_listener *listener, void *data)
{
    struct weston_seat *seat = data;
    struct input_context *input_ctx = wl_container_of(listener, input_ctx,
                                                      seat_create_listener);
    struct input_controller *controller;
    struct seat_ctx *ctx = calloc(1, sizeof *ctx);
    if (ctx == NULL) {
        weston_log("%s: Failed to allocate memory\n", __FUNCTION__);
        return;
    }

    ctx->input_ctx = input_ctx;

    ctx->keyboard_grab.interface = &keyboard_grab_interface;
    ctx->pointer_grab.interface = &pointer_grab_interface;
    ctx->touch_grab.interface= &touch_grab_interface;

    ctx->destroy_listener.notify = &handle_seat_destroy;
    wl_signal_add(&seat->destroy_signal, &ctx->destroy_listener);

    ctx->updated_caps_listener.notify = &handle_seat_updated_caps;
    wl_signal_add(&seat->updated_caps_signal, &ctx->updated_caps_listener);

    wl_list_for_each(controller, &input_ctx->controller_list, link) {
        ivi_input_send_seat_created(controller->resource,
                                    seat->seat_name,
                                    get_seat_capabilities(seat));
    }
}

static void
handle_surface_destroy(struct ivi_layout_surface *layout_surface, void *data)
{
    struct input_context *ctx = data;
    struct surface_ctx *surf, *next;
    int surface_removed = 0;
    const struct ivi_controller_interface *interface =
        ctx->ivi_controller_interface;

    wl_list_for_each_safe(surf, next, &ctx->surface_list, link) {
        if (surf->layout_surface == layout_surface) {
            uint32_t i;
            char **data = surf->accepted_devices.data;
            wl_list_remove(&surf->link);
            for (i = 0; i < surf->accepted_devices.size / sizeof(char**); i++) {
                free(data[i]);
	    }
            wl_array_release(&surf->accepted_devices);
            free(surf);
            surface_removed = 1;
            break;
        }
    }

    if (!surface_removed) {
        weston_log("%s: Warning! surface %d already destroyed\n", __FUNCTION__,
                   interface->get_id_of_surface((layout_surface)));
    }
}

static void
handle_surface_create(struct ivi_layout_surface *layout_surface, void *data)
{
    struct input_context *input_ctx = data;
    struct surface_ctx *ctx;
    const struct ivi_controller_interface *interface =
        input_ctx->ivi_controller_interface;

    wl_list_for_each(ctx, &input_ctx->surface_list, link) {
        if (ctx->layout_surface == layout_surface) {
            weston_log("%s: Warning! surface context already created for"
                       " surface %d\n", __FUNCTION__,
                       interface->get_id_of_surface((layout_surface)));
            break;
        }
    }

    ctx = calloc(1, sizeof *ctx);
    if (ctx == NULL) {
        weston_log("%s: Failed to allocate memory\n", __FUNCTION__);
        return;
    }
    ctx->layout_surface = layout_surface;
    ctx->input_context = input_ctx;
    wl_array_init(&ctx->accepted_devices);
    add_accepted_seat(ctx, "default");
    send_input_acceptance(input_ctx,
                          interface->get_id_of_surface(layout_surface),
                          "default", ILM_TRUE);

    wl_list_insert(&input_ctx->surface_list, &ctx->link);
}

static void
unbind_resource_controller(struct wl_resource *resource)
{
    struct input_controller *controller = wl_resource_get_user_data(resource);

    wl_list_remove(&controller->link);

    free(controller);
}

static void
input_set_input_focus(struct wl_client *client,
                                 struct wl_resource *resource,
                                 uint32_t surface, uint32_t device,
                                 int32_t enabled)
{
    struct input_controller *controller = wl_resource_get_user_data(resource);
    struct input_context *ctx = controller->input_context;
    struct surface_ctx *surf, *current_surf = NULL;
    struct weston_seat *seat;
    const struct ivi_controller_interface *interface =
	ctx->ivi_controller_interface;
    uint32_t caps;
    struct ivi_layout_surface *current_layout_surface;

    current_layout_surface = interface->get_surface_from_id(surface);

    if (!current_layout_surface) {
        weston_log("%s: surface %d was not found\n", __FUNCTION__, surface);
        return;
    }

    wl_list_for_each(surf, &ctx->surface_list, link) {
        if (current_layout_surface == surf->layout_surface) {
            current_surf = surf;
            if (enabled == ILM_TRUE) {
                surf->focus |= device;
            } else {
                surf->focus &= ~device;
            }

            send_input_focus(ctx, surface, device, enabled);

            wl_list_for_each(seat, &ctx->compositor->seat_list, link) {
                if (get_accepted_seat(surf, seat->seat_name) < 0)
                    continue;

                caps = get_seat_capabilities(seat);

                if (!(caps | device))
                    continue;

                set_weston_focus(ctx, surf, device, seat, enabled);
            }

            break;
        }
    }

    /* If focus is enabled for one of these devices, every other surface
     * must have focus unset */
    if ((device != ILM_INPUT_DEVICE_KEYBOARD) && enabled) {
        wl_list_for_each(surf, &ctx->surface_list, link) {
            if (surf == current_surf)
                continue;

            /* We do not need to unset the focus, if the surface does not have it*/
            if (!(surf->focus | device))
                continue;

            wl_list_for_each(seat, &ctx->compositor->seat_list, link) {
                /*if both of surfaces have acceptance to same seat */
                if ((get_accepted_seat(surf, seat->seat_name) < 0) ||
                   (get_accepted_seat(current_surf, seat->seat_name) < 0))
                    continue;

                caps = get_seat_capabilities(seat);

                if (!(caps | device))
                    continue;

                surf->focus &= ~(device);
                send_input_focus(ctx, interface->get_id_of_surface(surf->layout_surface),
                     device, ILM_FALSE);
                set_weston_focus(ctx, surf, device, seat, ILM_FALSE);
            }
        }
    }
}

static void
input_set_input_acceptance(struct wl_client *client,
                                      struct wl_resource *resource,
                                      uint32_t surface, const char *seat,
                                      int32_t accepted)
{
    struct input_controller *controller = wl_resource_get_user_data(resource);
    struct input_context *ctx = controller->input_context;
    struct surface_ctx *surface_ctx;
    int found_seat = 0;
    int found_weston_seat = 0;
    struct weston_seat *w_seat = NULL;
    const struct ivi_controller_interface *interface =
        ctx->ivi_controller_interface;

    wl_list_for_each(w_seat, &ctx->compositor->seat_list, link) {
        if(strcmp(seat,w_seat->seat_name) == 0) {
            found_weston_seat = 1;
            break;
        }
    }

    if (!found_weston_seat) {
        weston_log("%s: seat: %s was not found\n", __FUNCTION__, seat);
        return;
    }

    wl_list_for_each(surface_ctx, &ctx->surface_list, link) {
        if (interface->get_id_of_surface(surface_ctx->layout_surface) == surface) {
            if (accepted == ILM_TRUE) {
                found_seat = add_accepted_seat(surface_ctx, seat);
                if (found_weston_seat)
                    set_weston_focus(ctx, surface_ctx, surface_ctx->focus, w_seat, ILM_TRUE);
            } else {
                if (found_weston_seat)
                    set_weston_focus(ctx, surface_ctx, surface_ctx->focus, w_seat, ILM_FALSE);
                found_seat = remove_accepted_seat(surface_ctx, seat);
            }
            break;
        }
    }

    if (found_seat)
        send_input_acceptance(ctx, surface, seat, accepted);
}

static const struct ivi_input_interface input_implementation = {
    input_set_input_focus,
    input_set_input_acceptance
};

static void
bind_ivi_input(struct wl_client *client, void *data,
               uint32_t version, uint32_t id)
{
    struct input_context *ctx = data;
    struct input_controller *controller;
    struct weston_seat *seat;
    struct surface_ctx *surface_ctx;
    const struct ivi_controller_interface *interface =
        ctx->ivi_controller_interface;
    controller = calloc(1, sizeof *controller);
    if (controller == NULL) {
        weston_log("%s: Failed to allocate memory for controller\n",
                   __FUNCTION__);
        return;
    }

    controller->input_context = ctx;
    controller->resource =
        wl_resource_create(client, &ivi_input_interface, 1, id);
    wl_resource_set_implementation(controller->resource, &input_implementation,
                                   controller, unbind_resource_controller);

    controller->client = client;
    controller->id = id;

    wl_list_insert(&ctx->controller_list, &controller->link);

    /* Send seat events for all known seats to the client */
    wl_list_for_each(seat, &ctx->compositor->seat_list, link) {
        ivi_input_send_seat_created(controller->resource,
                                               seat->seat_name,
                                               get_seat_capabilities(seat));
    }
    /* Send focus events for all known surfaces to the client */
    wl_list_for_each(surface_ctx, &ctx->surface_list, link) {
        ivi_input_send_input_focus(controller->resource,
            interface->get_id_of_surface(surface_ctx->layout_surface),
            surface_ctx->focus, ILM_TRUE);
    }
    /* Send acceptance events for all known surfaces to the client */
    wl_list_for_each(surface_ctx, &ctx->surface_list, link) {
        char **name;
        wl_array_for_each(name, &surface_ctx->accepted_devices) {
            ivi_input_send_input_acceptance(controller->resource,
                    interface->get_id_of_surface(surface_ctx->layout_surface),
                    *name, ILM_TRUE);
        }
    }
}

static struct input_context *
create_input_context(struct weston_compositor *ec,
                     const struct ivi_controller_interface *interface)
{
    struct input_context *ctx = NULL;
    struct weston_seat *seat;
    ctx = calloc(1, sizeof *ctx);
    if (ctx == NULL) {
        weston_log("%s: Failed to allocate memory for input context\n",
                   __FUNCTION__);
        return NULL;
    }

    ctx->compositor = ec;
    ctx->ivi_controller_interface = interface;
    wl_list_init(&ctx->controller_list);
    wl_list_init(&ctx->surface_list);

    /* Add signal handlers for ivi surfaces. Warning: these functions leak
     * memory. */
    interface->add_notification_create_surface(handle_surface_create, ctx);
    interface->add_notification_remove_surface(handle_surface_destroy, ctx);

    ctx->seat_create_listener.notify = &handle_seat_create;
    wl_signal_add(&ec->seat_created_signal, &ctx->seat_create_listener);

    wl_list_for_each(seat, &ec->seat_list, link) {
        handle_seat_create(&ctx->seat_create_listener, seat);
        wl_signal_emit(&seat->updated_caps_signal, seat);
    }

    return ctx;
}

WL_EXPORT int
input_controller_module_init(struct weston_compositor *ec,
                             const struct ivi_controller_interface *interface,
                             size_t interface_version)
{
    struct input_context *ctx = create_input_context(ec, interface);
    if (ctx == NULL) {
        weston_log("%s: Failed to create input context\n", __FUNCTION__);
        return -1;
    }

    if (wl_global_create(ec->wl_display, &ivi_input_interface, 1,
                         ctx, bind_ivi_input) == NULL) {
        return -1;
    }
    weston_log("ivi-input-controller module loaded successfully!\n");
    return 0;
}
