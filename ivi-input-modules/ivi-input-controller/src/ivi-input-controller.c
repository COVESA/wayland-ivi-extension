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

#include <weston.h>
#include <weston/ivi-layout-export.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libweston/plugin-registry.h>
#include "ilm_types.h"

#include "ivi-input-server-protocol.h"
#include "ivi-controller.h"

struct seat_ctx {
    struct input_context *input_ctx;
    struct weston_keyboard_grab keyboard_grab;
    struct weston_pointer_grab pointer_grab;
    struct weston_touch_grab touch_grab;
    struct weston_seat *west_seat;

    /* pointer focus can be forced to specific surfaces
     * when there are no motion events at all. motion
     * event will re-evaulate the focus. A rotary knob
     * is one of the examples, where it is used as pointer
     * axis.*/
    struct ivisurface *forced_ptr_focus_surf;
    int32_t  forced_surf_enabled;

    struct wl_listener updated_caps_listener;
    struct wl_listener destroy_listener;
    struct wl_list seat_node;
};

struct seat_focus {
    struct seat_ctx *seat_ctx;
    ilmInputDevice focus;
    struct wl_list link;
};

struct input_context {
    struct wl_list resource_list;
    struct wl_list seat_list;
    int successful_init_stage;
    struct ivishell *ivishell;

    struct wl_listener surface_created;
    struct wl_listener surface_destroyed;
    struct wl_listener compositor_destroy_listener;
    struct wl_listener seat_create_listener;
};

enum kbd_events {
    KEYBOARD_ENTER,
    KEYBOARD_LEAVE,
    KEYBOARD_KEY,
    KEYBOARD_MODIFIER
};

struct wl_keyboard_data {
    enum kbd_events kbd_evt;
    uint32_t time;
    uint32_t key;
    uint32_t state;
    uint32_t mods_depressed;
    uint32_t mods_latched;
    uint32_t mods_locked;
    uint32_t group;
    uint32_t serial;
};

static struct seat_focus *
get_accepted_seat(struct ivisurface *surface, struct seat_ctx *seat_ctx)
{
    struct seat_focus *st_focus;
    struct seat_focus *ret_focus = NULL;

    wl_list_for_each(st_focus, &surface->accepted_seat_list, link) {
        if (st_focus->seat_ctx == seat_ctx) {
            ret_focus = st_focus;
            break;
        }
    }
    return ret_focus;
}

static int
add_accepted_seat(struct ivisurface *surface, struct seat_ctx *seat_ctx)
{
    const struct ivi_layout_interface *interface =
        surface->shell->interface;
    struct seat_focus *st_focus;
    int ret = 0;

    st_focus = get_accepted_seat(surface, seat_ctx);
    if (st_focus == NULL) {
        st_focus = calloc(1, sizeof(*st_focus));

        if (NULL != st_focus) {
            st_focus->seat_ctx = seat_ctx;
            wl_list_insert(&surface->accepted_seat_list, &st_focus->link);
            ret = 1;
       } else {
            weston_log("%s Failed to allocate memory for seat addition of surface %d",
                    __FUNCTION__, interface->get_id_of_surface(surface->layout_surface));
        }
    } else {
        weston_log("%s: Warning: seat '%s' is already accepted by surface %d\n",
                   __FUNCTION__, seat_ctx->west_seat->seat_name,
                   interface->get_id_of_surface(surface->layout_surface));
        ret = 1;
    }

    return ret;
}

static int
remove_if_seat_accepted(struct ivisurface *surface, struct seat_ctx *seat_ctx)
{
    int ret = 0;

    struct seat_focus *st_focus = get_accepted_seat(surface, seat_ctx);

    if (NULL != st_focus) {
        ret = 1;
        wl_list_remove(&st_focus->link);
        free(st_focus);

    }
    return ret;
}

struct seat_ctx*
input_ctrl_get_seat_ctx(struct input_context *ctx, const char *nm_seat)
{
    struct seat_ctx *ctx_seat;
    struct seat_ctx *ret_ctx = NULL;
    wl_list_for_each(ctx_seat, &ctx->seat_list, seat_node) {
        if (0 == strcmp(ctx_seat->west_seat->seat_name, nm_seat)) {
            ret_ctx = ctx_seat;
            break;
        }
    }
    return ret_ctx;
}

static void
send_input_acceptance(struct input_context *ctx, uint32_t surface_id, const char *seat, int32_t accepted)
{
    struct wl_resource *resource;
    wl_resource_for_each(resource, &ctx->resource_list) {
        ivi_input_send_input_acceptance(resource, surface_id, seat,
                                        accepted);
    }
}

static void
send_input_focus(struct input_context *ctx, struct ivisurface *surf_ctx,
                 ilmInputDevice device, t_ilm_bool enabled)
{
    struct wl_resource *resource;
    const struct ivi_layout_interface *lyt_if = ctx->ivishell->interface;
    t_ilm_surface surface_id = lyt_if->get_id_of_surface(surf_ctx->layout_surface);

    wl_resource_for_each(resource, &ctx->resource_list) {
        ivi_input_send_input_focus(resource, surface_id, device, enabled);
    }
}

static struct ivisurface *
input_ctrl_get_surf_ctx(struct input_context *ctx,
        struct ivi_layout_surface *lyt_surf)
{
    struct ivisurface *surf_ctx = NULL;
    struct ivisurface *ret_ctx = NULL;
    wl_list_for_each(surf_ctx, &ctx->ivishell->list_surface, link) {
        if (lyt_surf == surf_ctx->layout_surface) {
            ret_ctx = surf_ctx;
            break;
        }
    }
    return ret_ctx;
}


static struct ivisurface *
input_ctrl_get_surf_ctx_from_id(struct input_context *ctx,
        uint32_t ivi_surf_id)
{
    const struct ivi_layout_interface *interface = ctx->ivishell->interface;
    struct ivi_layout_surface *lyt_surf;
    lyt_surf = interface->get_surface_from_id(ivi_surf_id);

    return input_ctrl_get_surf_ctx(ctx, lyt_surf);
}


static struct ivisurface *
input_ctrl_get_surf_ctx_from_surf(struct input_context *ctx,
        struct weston_surface *west_surf)
{
    struct weston_surface *main_surface;
    struct ivi_layout_surface *layout_surf;
    struct ivisurface *surf_ctx = NULL;
    const struct ivi_layout_interface *lyt_if = ctx->ivishell->interface;
    main_surface = weston_surface_get_main_surface(west_surf);

    if (NULL != main_surface) {
        layout_surf = lyt_if->get_surface(main_surface);
        surf_ctx = input_ctrl_get_surf_ctx(ctx, layout_surf);
    }
    return surf_ctx;
}

static void
input_ctrl_kbd_snd_event_resource(struct seat_ctx *ctx_seat,
        struct weston_keyboard *keyboard, struct wl_resource *resource,
        struct wl_resource *surf_resource, struct wl_keyboard_data *kbd_data)
{
    struct weston_seat *seat = ctx_seat->west_seat;
    switch(kbd_data->kbd_evt)
    {
    case KEYBOARD_ENTER:
        if (wl_resource_get_version(resource) >=
                WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
            wl_keyboard_send_repeat_info(resource,
                             seat->compositor->kb_repeat_rate,
                             seat->compositor->kb_repeat_delay);
        }

        weston_keyboard_send_keymap(keyboard, resource);

        wl_keyboard_send_modifiers(resource,
                       kbd_data->serial,
                       keyboard->modifiers.mods_depressed,
                       keyboard->modifiers.mods_latched,
                       keyboard->modifiers.mods_locked,
                       keyboard->modifiers.group);
        wl_keyboard_send_enter(resource, kbd_data->serial, surf_resource,
                &keyboard->keys);

        break;
    case KEYBOARD_LEAVE:
        wl_keyboard_send_leave(resource, kbd_data->serial, surf_resource);
        break;
    case KEYBOARD_KEY:
        wl_keyboard_send_key(resource, kbd_data->serial, kbd_data->time,
                kbd_data->key, kbd_data->state);
        break;
    case KEYBOARD_MODIFIER:
        wl_keyboard_send_modifiers(resource,
                       kbd_data->serial,
                       kbd_data->mods_depressed,
                       kbd_data->mods_latched,
                       kbd_data->mods_locked,
                       kbd_data->group);
        break;
    default:
        weston_log("%s: error:  Uknown keyboard event %d", __FUNCTION__,
                kbd_data->kbd_evt);
        break;
    }
}


static void
input_ctrl_kbd_wl_snd_event(struct seat_ctx *ctx_seat,
        struct weston_surface *send_surf, struct weston_keyboard *keyboard,
        struct wl_keyboard_data *kbd_data)
{
    struct wl_resource *resource;
    struct wl_client *surface_client;
    struct wl_client *client;
    struct wl_list *resource_list;

    surface_client = wl_resource_get_client(send_surf->resource);
    resource_list = &keyboard->focus_resource_list;
    wl_resource_for_each(resource, resource_list) {
        client = wl_resource_get_client(resource);
        if (surface_client == client){
            input_ctrl_kbd_snd_event_resource(ctx_seat, keyboard, resource,
                    send_surf->resource, kbd_data);
        }
    }

    resource_list = &keyboard->resource_list;
    wl_resource_for_each(resource, resource_list) {
        client = wl_resource_get_client(resource);
        if (surface_client == client){
            input_ctrl_kbd_snd_event_resource(ctx_seat, keyboard, resource,
                    send_surf->resource, kbd_data);
        }
    }
}


static void
input_ctrl_kbd_leave_surf(struct seat_ctx *ctx_seat,
        struct ivisurface *surf_ctx, struct weston_surface *w_surf)
{
    struct wl_keyboard_data kbd_data;
    struct input_context *ctx = ctx_seat->input_ctx;
    struct seat_focus *st_focus;

    st_focus = get_accepted_seat(surf_ctx, ctx_seat);

    if ((NULL != st_focus)
        && ((st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD))) {

        kbd_data.kbd_evt = KEYBOARD_LEAVE;
        kbd_data.serial = wl_display_next_serial(
                                ctx->ivishell->compositor->wl_display);;
        input_ctrl_kbd_wl_snd_event(ctx_seat, w_surf,
                ctx_seat->keyboard_grab.keyboard, &kbd_data);

        st_focus->focus &= ~ILM_INPUT_DEVICE_KEYBOARD;
        send_input_focus(ctx, surf_ctx,
                ILM_INPUT_DEVICE_KEYBOARD, ILM_FALSE);
    }
}

static void
input_ctrl_kbd_enter_surf(struct seat_ctx *ctx_seat,
        struct ivisurface *surf_ctx, struct weston_surface *w_surf)
{
    struct wl_keyboard_data kbd_data;
    struct input_context *ctx = ctx_seat->input_ctx;
    struct seat_focus *st_focus;
    uint32_t serial;

    st_focus = get_accepted_seat(surf_ctx, ctx_seat);
    if ((NULL != st_focus) &&
        (!(st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD))) {
        serial = wl_display_next_serial(ctx->ivishell->compositor->wl_display);

        kbd_data.kbd_evt = KEYBOARD_ENTER;
        kbd_data.serial = serial;
        input_ctrl_kbd_wl_snd_event(ctx_seat, w_surf,
                ctx_seat->keyboard_grab.keyboard, &kbd_data);

        st_focus->focus |= ILM_INPUT_DEVICE_KEYBOARD;
        send_input_focus(ctx, surf_ctx,
                ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE);

    }
}

static void
input_ctrl_kbd_set_focus_surf(struct seat_ctx *ctx_seat,
        struct ivisurface *surf_ctx, int32_t enabled)
{
    struct input_context *ctx = ctx_seat->input_ctx;
    const struct ivi_layout_interface *interface = ctx->ivishell->interface;
    struct weston_surface *w_surf;

    struct weston_keyboard *keyboard = weston_seat_get_keyboard(
                                            ctx_seat->west_seat);


    if (NULL != keyboard) {
        w_surf = interface->surface_get_weston_surface(
                surf_ctx->layout_surface);

        if (ILM_TRUE == enabled) {
            input_ctrl_kbd_enter_surf(ctx_seat, surf_ctx, w_surf);
        } else {
            input_ctrl_kbd_leave_surf(ctx_seat, surf_ctx, w_surf);
        }
    }
}

static void
keyboard_grab_key(struct weston_keyboard_grab *grab, const struct timespec *time,
                  uint32_t key, uint32_t state)
{
    struct seat_ctx *seat_ctx = wl_container_of(grab, seat_ctx, keyboard_grab);
    struct ivisurface *surf_ctx;
    struct seat_focus *st_focus;
    struct wl_keyboard_data kbd_data;
    struct weston_surface *surface;
    const struct ivi_layout_interface *interface =
        seat_ctx->input_ctx->ivishell->interface;

    kbd_data.kbd_evt = KEYBOARD_KEY;
    kbd_data.time = timespec_to_msec(time);
    kbd_data.key = key;
    kbd_data.state = state;
    kbd_data.serial = wl_display_next_serial(grab->keyboard->seat->
                                            compositor->wl_display);

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->ivishell->list_surface, link) {

        st_focus = get_accepted_seat(surf_ctx, seat_ctx);
        if (NULL == st_focus)
            continue;

        if (!(st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD))
            continue;

        surface = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        input_ctrl_kbd_wl_snd_event(seat_ctx, surface, grab->keyboard, &kbd_data);
    }
}

static void
keyboard_grab_modifiers(struct weston_keyboard_grab *grab, uint32_t serial,
                        uint32_t mods_depressed, uint32_t mods_latched,
                        uint32_t mods_locked, uint32_t group)
{
    struct seat_ctx *seat_ctx = wl_container_of(grab, seat_ctx, keyboard_grab);
    struct ivisurface *surf_ctx;
    struct seat_focus *st_focus;
    struct weston_surface *surface;
    struct wl_keyboard_data kbd_data;
    const struct ivi_layout_interface *interface =
        seat_ctx->input_ctx->ivishell->interface;

    kbd_data.kbd_evt = KEYBOARD_MODIFIER;
    kbd_data.serial = serial;
    kbd_data.mods_depressed = mods_depressed;
    kbd_data.mods_latched = mods_latched;
    kbd_data.mods_locked = mods_locked;
    kbd_data.group = group;

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->ivishell->list_surface, link) {

        st_focus = get_accepted_seat(surf_ctx, seat_ctx);
        if (NULL == st_focus)
            continue;

        /* Keyboard modifiers go to surfaces with pointer focus as well */
        if (!(st_focus->focus
              & (ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_POINTER)))
            continue;

        surface = interface->surface_get_weston_surface(surf_ctx->layout_surface);

        input_ctrl_kbd_wl_snd_event(seat_ctx, surface, grab->keyboard, &kbd_data);
    }
}

static void
keyboard_grab_cancel(struct weston_keyboard_grab *grab)
{
    struct seat_ctx *ctx_seat = wl_container_of(grab, ctx_seat, keyboard_grab);
    struct ivisurface *surf;
    struct weston_surface *w_surf;
    const struct ivi_layout_interface *interface =
                                ctx_seat->input_ctx->ivishell->interface;

    wl_list_for_each(surf, &ctx_seat->input_ctx->ivishell->list_surface,
                     link) {
        w_surf = interface->surface_get_weston_surface(surf->layout_surface);
        input_ctrl_kbd_leave_surf(ctx_seat, surf, w_surf);
    }
}

static struct weston_keyboard_grab_interface keyboard_grab_interface = {
    keyboard_grab_key,
    keyboard_grab_modifiers,
    keyboard_grab_cancel
};

struct seat_focus *
input_ctrl_snd_focus_to_controller(struct ivisurface *surf_ctx,
        struct seat_ctx *ctx_seat, ilmInputDevice device,
        int32_t enabled)
{
    struct input_context *ctx = ctx_seat->input_ctx;
    struct seat_focus *st_focus = NULL;

    if (NULL != surf_ctx) {
        st_focus = get_accepted_seat(surf_ctx, ctx_seat);
        /* Send focus lost event to the surface which has lost the focus*/
        if (NULL != st_focus) {
            if (ILM_TRUE == enabled) {
                st_focus->focus |= device;
            } else {
                st_focus->focus &= ~device;
            }
            send_input_focus(ctx, surf_ctx, device, enabled);
        }
    }
    return st_focus;
}

static void
input_ctrl_ptr_leave_west_focus(struct seat_ctx *ctx_seat,
        struct weston_pointer *pointer)
{
    struct ivisurface *surf_ctx;
    struct input_context *ctx = ctx_seat->input_ctx;

    if (NULL != pointer->focus) {
        surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx,
                pointer->focus->surface);

        input_ctrl_snd_focus_to_controller(surf_ctx, ctx_seat,
                ILM_INPUT_DEVICE_POINTER, ILM_FALSE);

        weston_pointer_clear_focus(pointer);
    }
}


static void
input_ctrl_ptr_set_west_focus(struct seat_ctx *ctx_seat,
        struct weston_pointer *pointer, struct weston_view *w_view)
{
    struct weston_view *view = w_view;
    struct ivisurface *surf_ctx;
    struct input_context *ctx = ctx_seat->input_ctx;
    struct seat_focus *st_focus;
    wl_fixed_t sx, sy;

    if (NULL == view) {
        view = weston_compositor_pick_view(pointer->seat->compositor,
                       pointer->x, pointer->y,
                       &sx, &sy);
    } else {
        weston_view_from_global_fixed(view, pointer->x,
                        pointer->y, &sx, &sy);
    }

    if (pointer->focus != view) {

        if (NULL != pointer->focus) {
            surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx,
                                            pointer->focus->surface);
            /*Leave existing pointer focus*/
            input_ctrl_snd_focus_to_controller(surf_ctx, ctx_seat,
                    ILM_INPUT_DEVICE_POINTER, ILM_FALSE);

        }

        if (NULL != view) {
            surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx, view->surface);

            if (NULL != surf_ctx) {
                /*Enter into new pointer focus is seat accepts*/
                st_focus = input_ctrl_snd_focus_to_controller(surf_ctx,
                        ctx_seat, ILM_INPUT_DEVICE_POINTER, ILM_TRUE);

                if (st_focus != NULL) {
                    weston_pointer_set_focus(pointer, view, sx, sy);
                } else {
                    if (NULL != pointer->focus)
                        weston_pointer_clear_focus(pointer);
                }
            } else {
                weston_pointer_set_focus(pointer, view, sx, sy);
            }
        } else {
            if (NULL != pointer->focus)
                weston_pointer_clear_focus(pointer);
        }
    }
}

static bool
input_ctrl_ptr_is_focus_emtpy(struct seat_ctx *ctx_seat)
{
    return (NULL == ctx_seat->pointer_grab.pointer->focus);
}

static void
input_ctrl_ptr_clear_focus(struct seat_ctx *ctx_seat)
{
    if (!input_ctrl_ptr_is_focus_emtpy(ctx_seat)) {
        input_ctrl_ptr_leave_west_focus(ctx_seat,
                ctx_seat->pointer_grab.pointer);
        ctx_seat->forced_ptr_focus_surf = NULL;
    }
}

static void
input_ctrl_ptr_set_focus_surf(struct seat_ctx *ctx_seat,
        struct ivisurface *surf_ctx, int32_t enabled)
{
    struct weston_pointer *pointer;
    pointer = weston_seat_get_pointer(ctx_seat->west_seat);
    if (NULL != pointer) {
        if (ILM_TRUE == enabled) {
            if (ctx_seat->forced_ptr_focus_surf != surf_ctx) {
                ctx_seat->forced_ptr_focus_surf = surf_ctx;
                ctx_seat->forced_surf_enabled = ILM_TRUE;
                ctx_seat->pointer_grab.interface->focus(
                                    &ctx_seat->pointer_grab);
            }
        } else {
            if (ctx_seat->forced_ptr_focus_surf == surf_ctx) {
                ctx_seat->forced_surf_enabled = ILM_FALSE;
                ctx_seat->pointer_grab.interface->focus(
                                    &ctx_seat->pointer_grab);
            }
        }
    }
}

static void
pointer_grab_focus(struct weston_pointer_grab *grab)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, pointer_grab);
    struct weston_pointer *pointer = grab->pointer;
    struct weston_surface *forced_west_surf = NULL;
    struct weston_view *w_view;
    struct ivi_layout_surface *layout_surf;
    struct input_context *ctx = seat->input_ctx;

    if (pointer->button_count > 0) {
        return;
    }

    if (seat->forced_ptr_focus_surf != NULL) {
        /*When we want to force pointer focus to
         * a certain surface*/
        layout_surf = seat->forced_ptr_focus_surf->layout_surface;
        forced_west_surf = ctx->ivishell->interface->
                            surface_get_weston_surface(layout_surf);

        if (seat->forced_surf_enabled == ILM_TRUE) {
            if ((NULL != forced_west_surf) && !wl_list_empty(&forced_west_surf->views)) {
                w_view = wl_container_of(forced_west_surf->views.next,
                        w_view, surface_link);
                input_ctrl_ptr_set_west_focus(seat, pointer, w_view);
            }
        } else if (NULL != pointer->focus) {
            if(pointer->focus->surface == forced_west_surf) {
                input_ctrl_ptr_leave_west_focus(seat, pointer);
            }
            seat->forced_ptr_focus_surf = NULL;
        }

    } else {
        input_ctrl_ptr_set_west_focus(seat, pointer, NULL);
    }
}

static void
pointer_grab_motion(struct weston_pointer_grab *grab, const struct timespec *time,
                    struct weston_pointer_motion_event *event)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, pointer_grab);
    /*Motion results in re-evaluation of pointer focus*/
    seat->forced_ptr_focus_surf = NULL;
    weston_pointer_send_motion(grab->pointer, time, event);
}

static void
pointer_grab_button(struct weston_pointer_grab *grab, const struct timespec *time,
                    uint32_t button, uint32_t state)
{
    struct weston_pointer *pointer = grab->pointer;

    weston_pointer_send_button(pointer, time, button, state);

    if (pointer->button_count == 0 &&
        state == WL_POINTER_BUTTON_STATE_RELEASED) {
        grab->interface->focus(grab);
    }
}

static void
pointer_grab_axis(struct weston_pointer_grab *grab,
                  const struct timespec *time,
                  struct weston_pointer_axis_event *event)
{
    weston_pointer_send_axis(grab->pointer, time, event);
}

static void
pointer_grab_axis_source(struct weston_pointer_grab *grab,
                          uint32_t source)
{
    weston_pointer_send_axis_source(grab->pointer, source);
}

static void
pointer_grab_frame(struct weston_pointer_grab *grab)
{
    weston_pointer_send_frame(grab->pointer);
}

static void
pointer_grab_cancel(struct weston_pointer_grab *grab)
{
    struct seat_ctx *ctx_seat = wl_container_of(grab, ctx_seat, pointer_grab);
    input_ctrl_ptr_clear_focus(ctx_seat);
}

static struct weston_pointer_grab_interface pointer_grab_interface = {
    pointer_grab_focus,
    pointer_grab_motion,
    pointer_grab_button,
    pointer_grab_axis,
    pointer_grab_axis_source,
    pointer_grab_frame,
    pointer_grab_cancel
};

static void
input_ctrl_touch_set_west_focus(struct seat_ctx *ctx_seat,
        struct weston_touch *touch, const struct timespec *time,
        int touch_id, wl_fixed_t x, wl_fixed_t y)
{
    /*Weston would have set the focus here*/
    struct ivisurface *surf_ctx;
    struct seat_focus *st_focus;

    if (touch->focus == NULL)
        return;

    surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx_seat->input_ctx,
                            touch->focus->surface);

    if (NULL != surf_ctx) {
        if (touch->num_tp == 1) {
            st_focus = input_ctrl_snd_focus_to_controller(surf_ctx, ctx_seat,
                    ILM_INPUT_DEVICE_TOUCH, ILM_TRUE);
        } else {
            st_focus = get_accepted_seat(surf_ctx, ctx_seat);
        }

        if (st_focus != NULL) {
            weston_touch_send_down(touch, time, touch_id, x, y);

        } else {
            weston_touch_set_focus(touch, NULL);
        }
    } else {
        /*Support non ivi-surfaces like input panel*/
        weston_touch_send_down(touch, time, touch_id, x, y);
    }
}

static void
input_ctrl_touch_west_send_cancel(struct weston_touch *touch)
{
    struct wl_resource *resource;

    if (!weston_touch_has_focus_resource(touch))
        return;

    wl_resource_for_each(resource, &touch->focus_resource_list)
        wl_touch_send_cancel(resource);

}

static void
input_ctrl_touch_clear_focus(struct seat_ctx *ctx_seat)
{
    struct input_context *ctx = ctx_seat->input_ctx;
    struct weston_touch *touch = ctx_seat->touch_grab.touch;
    struct ivisurface *surf_ctx;

    if (touch->focus != NULL) {

        surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx,
                                touch->focus->surface);

        input_ctrl_snd_focus_to_controller(surf_ctx, ctx_seat,
                ILM_INPUT_DEVICE_TOUCH, ILM_FALSE);

        input_ctrl_touch_west_send_cancel(touch);

        weston_touch_set_focus(touch, NULL);
    }
}

static void
touch_grab_down(struct weston_touch_grab *grab, const struct timespec *time,
                int touch_id, wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);

    /* if touch device has no focused view, there is nothing to do*/
    if (grab->touch->focus == NULL)
        return;

    input_ctrl_touch_set_west_focus(seat, grab->touch, time, touch_id,
                                    x, y);
}

static void
touch_grab_up(struct weston_touch_grab *grab, const struct timespec *time,
              int touch_id)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct input_context *ctx = seat->input_ctx;
    struct weston_touch *touch = grab->touch;
    struct ivisurface *surf_ctx;

    if (NULL != touch->focus) {
        if (touch->num_tp == 0) {
            surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx,
                                    touch->focus->surface);

            input_ctrl_snd_focus_to_controller(surf_ctx,
                    seat, ILM_INPUT_DEVICE_TOUCH, ILM_FALSE);
        }
        weston_touch_send_up(touch, time, touch_id);
    }
}

static void
touch_grab_motion(struct weston_touch_grab *grab, const struct timespec *time, int touch_id,
                  wl_fixed_t x, wl_fixed_t y)
{
    weston_touch_send_motion(grab->touch, time, touch_id, x, y);
}

static void
touch_grab_frame(struct weston_touch_grab *grab)
{
    weston_touch_send_frame(grab->touch);
}

static void
touch_grab_cancel(struct weston_touch_grab *grab)
{
    struct seat_ctx *ctx_seat = wl_container_of(grab, ctx_seat, touch_grab);
    input_ctrl_touch_clear_focus(ctx_seat);
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
    struct input_context* input_ctx = ctx->input_ctx;
    struct wl_resource *resource;

    if (keyboard && keyboard != ctx->keyboard_grab.keyboard) {
        weston_keyboard_start_grab(keyboard, &ctx->keyboard_grab);
    }
    else if (!keyboard && ctx->keyboard_grab.keyboard) {
        ctx->keyboard_grab.keyboard = NULL;
    }

    if (pointer && pointer != ctx->pointer_grab.pointer) {
        weston_pointer_start_grab(pointer, &ctx->pointer_grab);
    }
    else if (!pointer && ctx->pointer_grab.pointer) {
        ctx->pointer_grab.pointer = NULL;
    }

    if (touch && touch != ctx->touch_grab.touch) {
        weston_touch_start_grab(touch, &ctx->touch_grab);
    }
    else if (!touch && ctx->touch_grab.touch) {
        ctx->touch_grab.touch = NULL;
    }

    wl_resource_for_each(resource, &input_ctx->resource_list) {
        ivi_input_send_seat_capabilities(resource,
                                         seat->seat_name,
                                         get_seat_capabilities(seat));
    }
}

static void
destroy_seat(struct seat_ctx *ctx_seat)
{
    struct ivisurface *surf;
    struct wl_resource *resource;

    /* Remove seat acceptance from surfaces which have input acceptance from
     * this seat */
    wl_list_for_each(surf, &ctx_seat->input_ctx->ivishell->list_surface,
                     link) {
         remove_if_seat_accepted(surf, ctx_seat);
    }

    wl_resource_for_each(resource, &ctx_seat->input_ctx->resource_list) {
        ivi_input_send_seat_destroyed(resource,
                                      ctx_seat->west_seat->seat_name);
    }
    wl_list_remove(&ctx_seat->destroy_listener.link);
    wl_list_remove(&ctx_seat->updated_caps_listener.link);
    wl_list_remove(&ctx_seat->seat_node);
    free(ctx_seat);
}

static void
handle_seat_destroy(struct wl_listener *listener, void *data)
{
    struct seat_ctx *ctx = wl_container_of(listener, ctx, destroy_listener);
    destroy_seat(ctx);
}

static void
handle_seat_create(struct wl_listener *listener, void *data)
{
    struct weston_seat *seat = data;
    struct input_context *input_ctx = wl_container_of(listener, input_ctx,
                                                      seat_create_listener);
    struct wl_resource *resource;
    struct ivisurface *surf;
    const struct ivi_layout_interface *interface =
        input_ctx->ivishell->interface;
    struct seat_ctx *ctx = calloc(1, sizeof *ctx);
    if (ctx == NULL) {
        weston_log("%s: Failed to allocate memory\n", __FUNCTION__);
        return;
    }

    ctx->input_ctx = input_ctx;
    ctx->west_seat = seat;

    ctx->keyboard_grab.interface = &keyboard_grab_interface;
    ctx->pointer_grab.interface = &pointer_grab_interface;
    ctx->touch_grab.interface= &touch_grab_interface;

    wl_list_insert(&input_ctx->seat_list, &ctx->seat_node);
    ctx->destroy_listener.notify = &handle_seat_destroy;
    wl_signal_add(&seat->destroy_signal, &ctx->destroy_listener);

    ctx->updated_caps_listener.notify = &handle_seat_updated_caps;
    wl_signal_add(&seat->updated_caps_signal, &ctx->updated_caps_listener);

    wl_resource_for_each(resource, &input_ctx->resource_list) {
        ivi_input_send_seat_created(resource,
                                    seat->seat_name,
                                    get_seat_capabilities(seat));
    }

    /* If default seat is created, we have to add it to the accepted_seat_list
     * of all surfaces. Also we have to send an acceptance event to all clients */
    if (!strcmp(ctx->west_seat->seat_name, "default")) {
        wl_list_for_each(surf, &input_ctx->ivishell->list_surface, link) {
            add_accepted_seat(surf, ctx);
            send_input_acceptance(input_ctx,
                                 interface->get_id_of_surface(surf->layout_surface),
                                 "default", ILM_TRUE);
        }
    }
}

static void
input_ctrl_free_surf_ctx(struct input_context *ctx, struct ivisurface *surf_ctx)
{
    struct seat_ctx *seat_ctx;
    struct seat_focus *st_focus;
    struct seat_focus *tmp_st_focus;

    wl_list_for_each_safe(st_focus, tmp_st_focus,
            &surf_ctx->accepted_seat_list, link) {
        seat_ctx = st_focus->seat_ctx;

        if (seat_ctx->forced_ptr_focus_surf == surf_ctx)
            seat_ctx->forced_ptr_focus_surf = NULL;

        wl_list_remove(&st_focus->link);
        free(st_focus);
    }
}

static void
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    struct input_context *ctx =
            wl_container_of(listener, ctx, surface_destroyed);
    struct ivisurface *surf = (struct ivisurface *) data;

    if (NULL != surf)
        input_ctrl_free_surf_ctx(ctx, surf);
}

static void
handle_surface_create(struct wl_listener *listener, void *data)
{
    struct input_context *input_ctx =
            wl_container_of(listener, input_ctx, surface_created);
    struct ivisurface *ivisurface = (struct ivisurface *) data;
    const struct ivi_layout_interface *interface =
        input_ctx->ivishell->interface;
    struct seat_ctx *seat_ctx;

    wl_list_init(&ivisurface->accepted_seat_list);

    seat_ctx = input_ctrl_get_seat_ctx(input_ctx, "default");
    if (seat_ctx) {
        add_accepted_seat(ivisurface, seat_ctx);
        send_input_acceptance(input_ctx,
                              interface->get_id_of_surface(ivisurface->layout_surface),
                              "default", ILM_TRUE);
    }
}

static void
unbind_resource_controller(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
setup_input_focus(struct input_context *ctx, uint32_t surface,
        uint32_t device, int32_t enabled)
{
    struct ivisurface *surf = NULL;
    struct seat_focus *st_focus;
    struct seat_ctx *ctx_seat;

    surf = input_ctrl_get_surf_ctx_from_id(ctx, surface);
    if (NULL != surf) {
        wl_list_for_each(st_focus, &surf->accepted_seat_list, link) {
            ctx_seat = st_focus->seat_ctx;
            if (ctx_seat != NULL) {
                if (device & ILM_INPUT_DEVICE_POINTER) {
                    input_ctrl_ptr_set_focus_surf(ctx_seat, surf, enabled);
                }
                if (device & ILM_INPUT_DEVICE_KEYBOARD) {
                    input_ctrl_kbd_set_focus_surf(ctx_seat, surf, enabled);
                }
                if (device & ILM_INPUT_DEVICE_TOUCH) {
                    /*Touch focus cannot be forced to a particular surface.
                     * Preserve the old behaviour by sending it to controller.
                     * TODO: Should we just remove focus setting for touch?*/
                    send_input_focus(ctx, surf, device, enabled);
                }
            }
        }
    }
}

static void
input_set_input_focus(struct wl_client *client,
                                 struct wl_resource *resource,
                                 uint32_t surface, uint32_t device,
                                 int32_t enabled)
{
    struct input_context *ctx = wl_resource_get_user_data(resource);
    setup_input_focus(ctx, surface, device, enabled);
}

static void
setup_input_acceptance(struct input_context *ctx,
                       uint32_t surface, const char *seat,
                       int32_t accepted)
{
    struct ivisurface *ivisurface;
    struct seat_ctx *ctx_seat;
    struct weston_surface *w_surf;
    int found_seat = 0;
    const struct ivi_layout_interface *interface =
        ctx->ivishell->interface;
    struct weston_pointer *pointer;
    struct weston_touch *touch;
    struct weston_keyboard *keyboard;
    struct seat_focus *st_focus;

    ctx_seat = input_ctrl_get_seat_ctx(ctx, seat);

    if (NULL == ctx_seat) {
        weston_log("%s: seat: %s was not found\n", __FUNCTION__, seat);
        return;
    }

    ivisurface = input_ctrl_get_surf_ctx_from_id(ctx, surface);

    if (NULL != ivisurface) {
        if (accepted == ILM_TRUE) {
            found_seat = add_accepted_seat(ivisurface, ctx_seat);

            pointer = weston_seat_get_pointer(ctx_seat->west_seat);
            if (NULL != pointer) {
                /*if seat is having NULL pointer focus, now it may be
                 * possible that this surface can hold the focus as it
                 * accepts events from that seat*/
                if (input_ctrl_ptr_is_focus_emtpy(ctx_seat)) {
                    pointer->grab->interface->focus(pointer->grab);
                }
            }
        } else {
            st_focus = get_accepted_seat(ivisurface, ctx_seat);

            if (NULL != st_focus) {
                w_surf = interface->surface_get_weston_surface(ivisurface->
                                                               layout_surface);

                pointer = weston_seat_get_pointer(ctx_seat->west_seat);
                if (NULL != pointer) {
                    if ((st_focus->focus & ILM_INPUT_DEVICE_POINTER)
                            == ILM_INPUT_DEVICE_POINTER) {
                        input_ctrl_ptr_clear_focus(ctx_seat);
                    }
                }
                touch = weston_seat_get_touch(ctx_seat->west_seat);
                if (NULL != touch) {
                    if ((st_focus->focus & ILM_INPUT_DEVICE_TOUCH)
                            == ILM_INPUT_DEVICE_TOUCH) {
                        input_ctrl_touch_clear_focus(ctx_seat);
                    }
                }
                keyboard = weston_seat_get_keyboard(ctx_seat->west_seat);

                if (NULL != keyboard) {
                    if ((st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD)
                            == ILM_INPUT_DEVICE_KEYBOARD) {
                        input_ctrl_kbd_leave_surf(ctx_seat,
                                ivisurface, w_surf);
                    }
                }

                found_seat = remove_if_seat_accepted(ivisurface, ctx_seat);
            }
        }
    }

    if (found_seat)
        send_input_acceptance(ctx, surface, seat, accepted);
}

static void
input_set_input_acceptance(struct wl_client *client,
                                      struct wl_resource *resource,
                                      uint32_t surface, const char *seat,
                                      int32_t accepted)
{
    struct input_context *ctx = wl_resource_get_user_data(resource);
    setup_input_acceptance(ctx, surface, seat, accepted);
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
    struct weston_seat *seat;
    struct wl_resource *resource;
    struct ivisurface *ivisurface;
    const struct ivi_layout_interface *interface =
        ctx->ivishell->interface;
    struct seat_focus *st_focus;
    uint32_t ivi_surf_id;

    resource = wl_resource_create(client, &ivi_input_interface, 1, id);
    wl_resource_set_implementation(resource, &input_implementation,
                                   ctx, unbind_resource_controller);

    wl_list_insert(&ctx->resource_list, wl_resource_get_link(resource));

    /* Send seat events for all known seats to the client */
    wl_list_for_each(seat, &ctx->ivishell->compositor->seat_list, link) {
        ivi_input_send_seat_created(resource, seat->seat_name,
                                    get_seat_capabilities(seat));
    }
    /* Send focus and acceptance events for all known surfaces to the client */
    wl_list_for_each(ivisurface, &ctx->ivishell->list_surface, link) {
        ivi_surf_id = interface->get_id_of_surface(ivisurface->layout_surface);
        wl_list_for_each(st_focus, &ivisurface->accepted_seat_list, link) {
            ivi_input_send_input_focus(resource, ivi_surf_id,
                                       st_focus->focus, ILM_TRUE);
            ivi_input_send_input_acceptance(resource, ivi_surf_id,
                                            st_focus->seat_ctx->west_seat->seat_name,
                                            ILM_TRUE);
        }
    }
}

static void
destroy_input_context(struct input_context *ctx)
{
    struct seat_ctx *seat;
    struct seat_ctx *tmp;
    struct ivisurface *surf_ctx;
    struct ivisurface *tmp_surf_ctx;
    struct wl_resource *resource, *tmp_resource;

    wl_list_for_each_safe(seat, tmp, &ctx->seat_list, seat_node) {
        destroy_seat(seat);
    }

    wl_list_for_each_safe(surf_ctx, tmp_surf_ctx,
            &ctx->ivishell->list_surface, link) {
        input_ctrl_free_surf_ctx(ctx, surf_ctx);
    }

    wl_list_remove(&ctx->seat_create_listener.link);
    wl_list_remove(&ctx->surface_created.link);
    wl_list_remove(&ctx->surface_destroyed.link);
    wl_list_remove(&ctx->compositor_destroy_listener.link);

    wl_resource_for_each_safe(resource, tmp_resource, &ctx->resource_list) {
        /*We have set destroy function for this resource.
         * The below api will call unbind_resource_controller and
         * free up the controller structure*/
        wl_resource_destroy(resource);
    }
    free(ctx);
}

static void
input_controller_deinit(struct input_context *ctx)
{
    int deinit_stage;

    if (NULL == ctx) {
        return;
    }

    for (deinit_stage = ctx->successful_init_stage; deinit_stage >= 0;
            deinit_stage--) {
        switch (deinit_stage) {
        case 1:
            /*Nothing to free for this stage*/
            break;

        case 0:
            destroy_input_context(ctx);
            break;

        default:
            break;
        }
    }
}

static void
input_controller_destroy(struct wl_listener *listener, void *data)
{
    (void)data;
    if (NULL != listener) {
        struct input_context *ctx = wl_container_of(listener,
                ctx, compositor_destroy_listener);

        input_controller_deinit(ctx);
    }
}


static struct input_context *
create_input_context(struct ivishell *shell)
{
    struct input_context *ctx = NULL;
    struct weston_seat *seat;
    ctx = calloc(1, sizeof *ctx);
    if (ctx == NULL) {
        weston_log("%s: Failed to allocate memory for input context\n",
                   __FUNCTION__);
        return NULL;
    }

    ctx->ivishell = shell;
    wl_list_init(&ctx->resource_list);
    wl_list_init(&ctx->seat_list);

    /* Add signal handlers for ivi surfaces. */
    ctx->surface_created.notify = handle_surface_create;
    ctx->surface_destroyed.notify = handle_surface_destroy;
    ctx->compositor_destroy_listener.notify = input_controller_destroy;

    wl_signal_add(&ctx->ivishell->ivisurface_created_signal, &ctx->surface_created);
    wl_signal_add(&ctx->ivishell->ivisurface_removed_signal, &ctx->surface_destroyed);
    wl_signal_add(&ctx->ivishell->compositor->destroy_signal, &ctx->compositor_destroy_listener);

    ctx->seat_create_listener.notify = &handle_seat_create;
    wl_signal_add(&ctx->ivishell->compositor->seat_created_signal, &ctx->seat_create_listener);

    wl_list_for_each(seat, &ctx->ivishell->compositor->seat_list, link) {
        handle_seat_create(&ctx->seat_create_listener, seat);
        wl_signal_emit(&seat->updated_caps_signal, seat);
    }

    return ctx;
}


static int
input_controller_init(struct ivishell *shell)
{
    int successful_init_stage = 0;
    int init_stage;
    int ret = -1;
    struct input_context *ctx = NULL;
    bool init_success = false;

    for (init_stage = 0; (init_stage == successful_init_stage);
            init_stage++) {
        switch(init_stage)
        {
        case 0:
            ctx = create_input_context(shell);
            if (NULL != ctx)
                successful_init_stage++;
            break;
        case 1:
            if (wl_global_create(shell->compositor->wl_display, &ivi_input_interface, 1,
                                 ctx, bind_ivi_input) != NULL) {
                successful_init_stage++;
            }
            break;
        default:
            init_success = true;
            break;
        }

    }
    if (NULL != ctx) {
         ctx->successful_init_stage = (successful_init_stage - 1);
         ret = 0;
         if (!init_success) {
             weston_log("Initialization failed at stage: %d\n",\
                     successful_init_stage);
             input_controller_deinit(ctx);
             ret = -1;
         }
    }
    return ret;
}

WL_EXPORT int
input_controller_module_init(struct ivishell *shell)
{
    int ret = -1;

    if (NULL != shell->interface) {
        ret = input_controller_init(shell);

        if (ret >= 0)
            weston_log("ivi-input-controller module loaded successfully!\n");
    }
    return ret;
}

