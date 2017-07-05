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

#include "plugin-registry.h"
#include "ilm_types.h"

#include "ivi-input-server-protocol.h"

struct seat_ctx {
    const char *name_seat;
    struct input_context *input_ctx;
    struct weston_keyboard_grab keyboard_grab;
    struct weston_pointer_grab pointer_grab;
    struct weston_touch_grab touch_grab;
    struct weston_seat *west_seat;
    struct wl_listener updated_caps_listener;
    struct wl_listener destroy_listener;
    struct wl_list seat_node;
};

struct seat_focus {
    char *seat_name;
    ilmInputDevice focus;
    struct wl_list link;
};

struct surface_ctx {
    struct wl_list link;
    ilmInputDevice focus;
    struct ivi_layout_surface *layout_surface;
    struct wl_list accepted_seat_list;
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
    struct wl_list seat_list;
    struct weston_compositor *compositor;
    const struct ivi_layout_interface *ivi_layout_interface;
    int successful_init_stage;

    struct wl_listener surface_created;
    struct wl_listener surface_destroyed;
    struct wl_listener compositor_destroy_listener;
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
get_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    struct seat_focus *st_focus;
    struct seat_focus *ret_focus = NULL;

    wl_list_for_each(st_focus, &surface->accepted_seat_list, link) {

        if (strcmp(st_focus->seat_name, seat) == 0) {
            ret_focus = st_focus;
            break;
        }
    }
    return ret_focus;
}

static int
add_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    const struct ivi_layout_interface *interface =
        surface->input_context->ivi_layout_interface;
    struct seat_focus *st_focus;
    int ret = 0;

    st_focus = get_accepted_seat(surface, seat);
    if (st_focus == NULL) {
        st_focus = calloc(1, sizeof(*st_focus));

        if (NULL != st_focus) {
            st_focus->seat_name = strdup(seat);
            if (NULL != st_focus->seat_name) {
                wl_list_insert(&surface->accepted_seat_list,
                        &st_focus->link);
                ret = 1;
            } else {
                free(st_focus);
                weston_log("%s Failed to allocate memory for seat addition of surface %d",
                        __FUNCTION__, interface->get_id_of_surface(surface->layout_surface));
            }
       } else {
            weston_log("%s Failed to allocate memory for seat addition of surface %d",
                    __FUNCTION__, interface->get_id_of_surface(surface->layout_surface));
        }

    }else {
        weston_log("%s: Warning: seat '%s' is already accepted by surface %d\n",
                   __FUNCTION__, seat,
                   interface->get_id_of_surface(surface->layout_surface));
        ret = 1;
    }

    return ret;
}

static int
remove_accepted_seat(struct surface_ctx *surface, const char *seat)
{
    int ret = 0;
    const struct ivi_layout_interface *interface =
            surface->input_context->ivi_layout_interface;

    struct seat_focus *st_focus = get_accepted_seat(surface, seat);

    if (NULL != st_focus) {
        ret = 1;
        wl_list_remove(&st_focus->link);
        free(st_focus);

    } else {
        weston_log("%s: Warning: seat '%s' not found for surface %u\n",
                  __FUNCTION__, seat,
                  interface->get_id_of_surface(surface->layout_surface));
    }
    return ret;
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
pointer_move(struct weston_pointer *pointer, wl_fixed_t x, wl_fixed_t y)
{
    struct weston_pointer_motion_event event;

    event.x = x;
    event.y = y;
    event.dx = 0;
    event.dy = 0;
    event.mask = WESTON_POINTER_MOTION_ABS;

    weston_pointer_move(pointer, &event);
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
    const struct ivi_layout_interface *interface =
              ctx->ivi_layout_interface;

    /* Assume one view per surface */
    w_surface = interface->surface_get_weston_surface(surface_ctx->layout_surface);
    view = wl_container_of(w_surface->views.next, view, surface_link);
    if ((focus & ILM_INPUT_DEVICE_POINTER) && (pointer != NULL)){
        if ( (view != NULL) && enabled) {
            weston_view_to_global_fixed(view, wl_fixed_from_int(0),
            wl_fixed_from_int(0), &x, &y);
            // move pointer to local (0,0) of the view
            pointer_move(pointer, x, y);
            weston_pointer_set_focus(pointer, view,
            wl_fixed_from_int(0), wl_fixed_from_int(0));
        } else if (pointer->focus == view) {
            weston_pointer_set_focus(pointer, NULL,
            wl_fixed_from_int(0), wl_fixed_from_int(0));
        }
    }

    if ((focus & ILM_INPUT_DEVICE_KEYBOARD) && (keyboard != NULL)) {
        surface_client = wl_resource_get_client(w_surface->resource);
        serial = wl_display_next_serial(ctx->compositor->wl_display);
        resource = wl_resource_find_for_client(&keyboard->resource_list,
                                               surface_client);

        if (!resource)
            return;

        if (!enabled)
            wl_keyboard_send_leave(resource, serial, w_surface->resource);
        else
            wl_keyboard_send_enter(resource, serial, w_surface->resource,
                                   &keyboard->keys);
    }
}

static struct surface_ctx *
input_ctrl_get_surf_ctx(struct input_context *ctx,
        struct ivi_layout_surface *lyt_surf)
{
    struct surface_ctx *surf_ctx = NULL;
    struct surface_ctx *ret_ctx = NULL;
    wl_list_for_each(surf_ctx, &ctx->surface_list, link) {
        if (lyt_surf == surf_ctx->layout_surface) {
            ret_ctx = surf_ctx;
            break;
        }
    }
    return ret_ctx;
}


static struct surface_ctx *
input_ctrl_get_surf_ctx_from_id(struct input_context *ctx,
        uint32_t ivi_surf_id)
{
    const struct ivi_layout_interface *interface = ctx->ivi_layout_interface;
    struct ivi_layout_surface *lyt_surf;
    lyt_surf = interface->get_surface_from_id(ivi_surf_id);

    return input_ctrl_get_surf_ctx(ctx, lyt_surf);
}


static struct surface_ctx *
input_ctrl_get_surf_ctx_from_surf(struct input_context *ctx,
        struct weston_surface *west_surf)
{
    struct weston_surface *main_surface;
    struct ivi_layout_surface *layout_surf;
    struct surface_ctx *surf_ctx = NULL;
    const struct ivi_layout_interface *lyt_if = ctx->ivi_layout_interface;
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

        if (seat->compositor->use_xkbcommon) {
            wl_keyboard_send_keymap(resource,
                        WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                        keyboard->xkb_info->keymap_fd,
                        keyboard->xkb_info->keymap_size);
        } else {
            int null_fd = open("/dev/null", O_RDONLY);
            wl_keyboard_send_keymap(resource,
                        WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP,
                        null_fd,
                        0);
            close(null_fd);
        }
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
    struct input_context *ctx = ctx_seat->input_ctx;
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
        struct surface_ctx *surf_ctx, struct weston_surface *w_surf)
{
    struct wl_keyboard_data kbd_data;
    struct input_context *ctx = ctx_seat->input_ctx;
    const struct ivi_layout_interface *interface = ctx->ivi_layout_interface;
    struct seat_focus *st_focus;


    st_focus = get_accepted_seat(surf_ctx, ctx_seat->name_seat);

    if ((NULL != st_focus)
        && ((st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD))) {

        kbd_data.kbd_evt = KEYBOARD_LEAVE;
        kbd_data.serial = wl_display_next_serial(
                                ctx->compositor->wl_display);;
        input_ctrl_kbd_wl_snd_event(ctx_seat, w_surf,
                ctx_seat->keyboard_grab.keyboard, &kbd_data);

        st_focus->focus &= ~ILM_INPUT_DEVICE_KEYBOARD;
        send_input_focus(ctx,
                interface->get_id_of_surface(surf_ctx->layout_surface),
                ILM_INPUT_DEVICE_KEYBOARD, ILM_FALSE);
    }
}

static void
input_ctrl_kbd_enter_surf(struct seat_ctx *ctx_seat,
        struct surface_ctx *surf_ctx, struct weston_surface *w_surf)
{
    struct wl_keyboard_data kbd_data;
    struct input_context *ctx = ctx_seat->input_ctx;
    struct seat_focus *st_focus;
    const struct ivi_layout_interface *interface = ctx->ivi_layout_interface;
    uint32_t serial;

    st_focus = get_accepted_seat(surf_ctx, ctx_seat->name_seat);
    if ((NULL != st_focus) &&
        (!(st_focus->focus & ILM_INPUT_DEVICE_KEYBOARD))) {
        serial = wl_display_next_serial(ctx->compositor->wl_display);

        kbd_data.kbd_evt = KEYBOARD_ENTER;
        kbd_data.serial = serial;
        input_ctrl_kbd_wl_snd_event(ctx_seat, w_surf,
                ctx_seat->keyboard_grab.keyboard, &kbd_data);

        st_focus->focus |= ILM_INPUT_DEVICE_KEYBOARD;
        send_input_focus(ctx,
                interface->get_id_of_surface(surf_ctx->layout_surface),
                ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE);

    }
}

static void
input_ctrl_kbd_set_focus_surf(struct seat_ctx *ctx_seat,
        uint32_t ivi_surf_id, int32_t enabled)
{
    struct input_context *ctx = ctx_seat->input_ctx;
    struct surface_ctx *surf_ctx;
    const struct ivi_layout_interface *interface = ctx->ivi_layout_interface;
    struct weston_surface *w_surf;

    struct weston_keyboard *keyboard = weston_seat_get_keyboard(
                                            ctx_seat->west_seat);


    if (NULL != keyboard) {
        surf_ctx = input_ctrl_get_surf_ctx_from_id(ctx, ivi_surf_id);
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
keyboard_grab_key(struct weston_keyboard_grab *grab, uint32_t time,
                  uint32_t key, uint32_t state)
{
    struct seat_ctx *seat_ctx = wl_container_of(grab, seat_ctx, keyboard_grab);
    struct surface_ctx *surf_ctx;
    struct seat_focus *st_focus;
    struct wl_keyboard_data kbd_data;
    struct weston_surface *surface;
    const struct ivi_layout_interface *interface =
        seat_ctx->input_ctx->ivi_layout_interface;

    kbd_data.kbd_evt = KEYBOARD_KEY;
    kbd_data.time = time;
    kbd_data.key = key;
    kbd_data.state = state;
    kbd_data.serial = wl_display_next_serial(grab->keyboard->seat->
                                            compositor->wl_display);

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->surface_list, link) {

        st_focus = get_accepted_seat(surf_ctx, grab->keyboard->seat->seat_name);
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
    struct surface_ctx *surf_ctx;
    struct seat_focus *st_focus;
    struct weston_surface *surface;
    struct wl_keyboard_data kbd_data;
    const struct ivi_layout_interface *interface =
        seat_ctx->input_ctx->ivi_layout_interface;

    kbd_data.kbd_evt = KEYBOARD_MODIFIER;
    kbd_data.serial = serial;
    kbd_data.mods_depressed = mods_depressed;
    kbd_data.mods_latched = mods_latched;
    kbd_data.mods_locked = mods_locked;
    kbd_data.group = group;

    wl_list_for_each(surf_ctx, &seat_ctx->input_ctx->surface_list, link) {

        st_focus = get_accepted_seat(surf_ctx, grab->keyboard->seat->seat_name);
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
                    struct weston_pointer_motion_event *event)
{
    struct weston_pointer *pointer = grab->pointer;
    struct weston_compositor *compositor = pointer->seat->compositor;
    struct wl_list *resource_list;
    struct wl_resource *resource;
    struct weston_view *picked_view;
    wl_fixed_t x, y;
    wl_fixed_t sx, sy;
    wl_fixed_t old_sx = pointer->sx;
    wl_fixed_t old_sy = pointer->sy;

    weston_pointer_move(pointer, event);

    if (pointer->focus) {
        weston_pointer_motion_to_abs(pointer, event, &x, &y);
        picked_view = weston_compositor_pick_view(compositor, x, y,
                                           &sx, &sy);

       if (picked_view != pointer->focus)
           return;

        pointer->sx = sx;
        pointer->sy = sy;
    }

    if (pointer->focus_client &&
        (old_sx != pointer->sx || old_sy != pointer->sy)) {
        resource_list = &pointer->focus_client->pointer_resources;
        wl_resource_for_each(resource, resource_list) {
            wl_pointer_send_motion(resource, time,
                            pointer->sx, pointer->sy);
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
    struct weston_surface *w_surf, *send_surf;
    struct weston_subsurface *sub;
    struct wl_resource *resource;
    struct wl_list *resource_list = NULL;
    uint32_t serial;
    const struct ivi_layout_interface *interface =
        seat->input_ctx->ivi_layout_interface;

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

            /* Find a focused surface from subsurface list */
            send_surf = w_surf;
            if (!wl_list_empty(&w_surf->subsurface_list)) {
                wl_list_for_each(sub, &w_surf->subsurface_list, parent_link) {
                    if (sub->surface == picked_view->surface) {
                        send_surf = sub->surface;
                        break;
                    }
                }
            }

            w_view = wl_container_of(send_surf->views.next, w_view, surface_link);

            if (get_accepted_seat(surf_ctx, grab->pointer->seat->seat_name) == NULL)
                continue;

            if (picked_view->surface == send_surf) {
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

    if (pointer->focus_client)
        resource_list = &pointer->focus_client->pointer_resources;
    if (resource_list && !wl_list_empty(resource_list)) {
        resource_list = &pointer->focus_client->pointer_resources;
        serial = wl_display_next_serial(display);
        wl_resource_for_each(resource, resource_list)
            wl_pointer_send_button(resource,
                           serial,
                           time,
                           button,
                           state);
    }
}

static void
pointer_grab_axis(struct weston_pointer_grab *grab,
                  uint32_t time,
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
        struct weston_touch *touch, uint32_t time, int touch_id,
        wl_fixed_t x, wl_fixed_t y)
{
    /*Weston would have set the focus here*/
    struct surface_ctx *surf_ctx;
    const struct ivi_layout_interface *interface =
        ctx_seat->input_ctx->ivi_layout_interface;
    struct seat_focus *st_focus;

    if (touch->focus == NULL)
        return;

    surf_ctx = input_ctrl_get_surf_ctx_from_surf(ctx_seat->input_ctx,
                            touch->focus->surface);

    if (NULL != surf_ctx) {
        st_focus = get_accepted_seat(surf_ctx, touch->seat->seat_name);
        if (st_focus != NULL) {

            if (touch->num_tp == 1) {
                st_focus->focus |= ILM_INPUT_DEVICE_TOUCH;
                send_input_focus(ctx_seat->input_ctx,
                     interface->get_id_of_surface(surf_ctx->layout_surface),
                     ILM_INPUT_DEVICE_TOUCH, ILM_TRUE);
            }
            weston_touch_send_down(touch, time, touch_id, x, y);
        } else {
            weston_touch_set_focus(touch, NULL);
        }
    }
}

static void
touch_grab_down(struct weston_touch_grab *grab, uint32_t time, int touch_id,
                wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);

    /* if touch device has no focused view, there is nothing to do*/
    if (grab->touch->focus == NULL)
        return;

    input_ctrl_touch_set_west_focus(seat, grab->touch, time, touch_id,
                                    x, y);
}

static void
touch_grab_up(struct weston_touch_grab *grab, uint32_t time, int touch_id)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct wl_display *display = grab->touch->seat->compositor->wl_display;
    struct surface_ctx *surf_ctx;
    const struct ivi_layout_interface *interface =
        seat->input_ctx->ivi_layout_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf, *send_surf;
        struct weston_subsurface *sub;
        struct wl_resource *resource;
        struct wl_client *surface_client;
        uint32_t serial;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) == NULL)
            continue;

        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        serial = wl_display_next_serial(display);

        /* Find a focused surface from subsurface list */
        send_surf = surf;
        if (!wl_list_empty(&surf->subsurface_list)) {
            wl_list_for_each(sub, &surf->subsurface_list, parent_link) {
                if (sub->surface == grab->touch->focus->surface) {
                    send_surf = sub->surface;
                    break;
                }
            }
        }

        /* Touches unset touch focus */
        if (grab->touch->num_tp == 0) {
            if (send_surf == grab->touch->focus->surface)
                surf_ctx->focus &= ~ILM_INPUT_DEVICE_TOUCH;
                send_input_focus(seat->input_ctx,
                                 interface->get_id_of_surface(surf_ctx->layout_surface),
                                 ILM_INPUT_DEVICE_TOUCH, ILM_FALSE);
        }

        surface_client = wl_resource_get_client(send_surf->resource);

        resource = wl_resource_find_for_client(&grab->touch->resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_up(resource, serial, time, touch_id);

        resource = wl_resource_find_for_client(&grab->touch->focus_resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_up(resource, serial, time, touch_id);

    }
}

static void
touch_grab_motion(struct weston_touch_grab *grab, uint32_t time, int touch_id,
                  wl_fixed_t x, wl_fixed_t y)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct surface_ctx *surf_ctx;
    wl_fixed_t sx, sy;
    const struct ivi_layout_interface *interface =
        seat->input_ctx->ivi_layout_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf, *send_surf;
        struct weston_subsurface *sub;
        struct weston_view *view;
        struct wl_resource *resource;
        struct wl_client *surface_client;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) == NULL)
            continue;

        /* Assume one view per surface */
        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);

        /* Find a focused surface from subsurface list */
        send_surf = surf;
        if (!wl_list_empty(&surf->subsurface_list)) {
            wl_list_for_each(sub, &surf->subsurface_list, parent_link) {
                if (sub->surface == grab->touch->focus->surface) {
                    send_surf = sub->surface;
                    break;
                }
            }
        }

        view = wl_container_of(send_surf->views.next, view, surface_link);
        weston_view_from_global_fixed(view, x, y, &sx, &sy);

        surface_client = wl_resource_get_client(send_surf->resource);

        resource = wl_resource_find_for_client(&grab->touch->resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_motion(resource, time, touch_id, sx, sy);

        resource = wl_resource_find_for_client(&grab->touch->focus_resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_motion(resource, time, touch_id, sx, sy);
    }
}

static void
touch_grab_frame(struct weston_touch_grab *grab)
{
    struct seat_ctx *seat = wl_container_of(grab, seat, touch_grab);
    struct surface_ctx *surf_ctx;
    const struct ivi_layout_interface *interface =
        seat->input_ctx->ivi_layout_interface;

    /* For each surface_ctx, check for focus and send */
    wl_list_for_each(surf_ctx, &seat->input_ctx->surface_list, link) {
        struct weston_surface *surf;
        struct wl_resource *resource;
        struct wl_client *surface_client;

        if (!(surf_ctx->focus & ILM_INPUT_DEVICE_TOUCH))
            continue;

        if (get_accepted_seat(surf_ctx, grab->touch->seat->seat_name) == NULL)
            continue;

        surf = interface->surface_get_weston_surface(surf_ctx->layout_surface);
        surface_client = wl_resource_get_client(surf->resource);

        resource = wl_resource_find_for_client(&grab->touch->resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_frame(resource);

        resource = wl_resource_find_for_client(&grab->touch->focus_resource_list,
                                               surface_client);
        if (resource)
            wl_touch_send_frame(resource);
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
    wl_list_remove(&ctx->seat_node);
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
    ctx->name_seat = strdup(seat->seat_name);
    ctx->west_seat = seat;

    ctx->keyboard_grab.interface = &keyboard_grab_interface;
    ctx->pointer_grab.interface = &pointer_grab_interface;
    ctx->touch_grab.interface= &touch_grab_interface;

    wl_list_insert(&input_ctx->seat_list, &ctx->seat_node);
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
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    struct input_context *ctx =
            wl_container_of(listener, ctx, surface_destroyed);
    struct surface_ctx *surf;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    int surface_removed = 0;
    const struct ivi_layout_interface *interface =
        ctx->ivi_layout_interface;
    struct seat_focus *st_focus;
    struct seat_focus *st_focus_tmp;

    surf = input_ctrl_get_surf_ctx(ctx, layout_surface);

    if (NULL != surf) {

        wl_list_remove(&surf->link);

        wl_list_for_each_safe(st_focus, st_focus_tmp,
                &surf->accepted_seat_list, link) {
            wl_list_remove(&st_focus->link);
            free(st_focus->seat_name);
            free(st_focus);
        }

        free(surf);
        surface_removed = 1;
    }

    if (!surface_removed) {
        weston_log("%s: Warning! surface %d already destroyed\n", __FUNCTION__,
                   interface->get_id_of_surface((layout_surface)));
    }
}

static void
handle_surface_create(struct wl_listener *listener, void *data)
{
    struct input_context *input_ctx =
            wl_container_of(listener, input_ctx, surface_created);
    struct surface_ctx *ctx;
    struct ivi_layout_surface *layout_surface =
           (struct ivi_layout_surface *) data;
    const struct ivi_layout_interface *interface =
        input_ctx->ivi_layout_interface;

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
    wl_list_init(&ctx->accepted_seat_list);
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
    const struct ivi_layout_interface *interface =
	ctx->ivi_layout_interface;
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
                if (get_accepted_seat(surf, seat->seat_name) == NULL)
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
                if ((get_accepted_seat(surf, seat->seat_name) == NULL) ||
                   (get_accepted_seat(current_surf, seat->seat_name) == NULL))
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
    const struct ivi_layout_interface *interface =
        ctx->ivi_layout_interface;

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
    const struct ivi_layout_interface *interface =
        ctx->ivi_layout_interface;
    struct seat_focus *st_focus;
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
        wl_list_for_each(st_focus, &surface_ctx->accepted_seat_list, link) {
            ivi_input_send_input_acceptance(controller->resource,
                    interface->get_id_of_surface(surface_ctx->layout_surface),
                    st_focus->seat_name, ILM_TRUE);

        }
    }
}

static void
destroy_input_context(struct input_context *ctx)
{
    struct seat_ctx *seat;
    struct seat_ctx *tmp;

    wl_list_for_each_safe(seat, tmp, &ctx->seat_list, seat_node) {
        wl_list_remove(&seat->seat_node);
        free(seat);
    }
    free(ctx);
}

static void
input_controller_deinit(struct input_context *ctx)
{
    int deinit_stage;
    int ret = 0;
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
create_input_context(struct weston_compositor *ec,
                     const struct ivi_layout_interface *interface)
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
    ctx->ivi_layout_interface = interface;
    wl_list_init(&ctx->controller_list);
    wl_list_init(&ctx->surface_list);
    wl_list_init(&ctx->seat_list);

    /* Add signal handlers for ivi surfaces. */
    ctx->surface_created.notify = handle_surface_create;
    ctx->surface_destroyed.notify = handle_surface_destroy;
    ctx->compositor_destroy_listener.notify = input_controller_destroy;

    interface->add_listener_create_surface(&ctx->surface_created);
    interface->add_listener_remove_surface(&ctx->surface_destroyed);
    wl_signal_add(&ec->destroy_signal, &ctx->compositor_destroy_listener);

    ctx->seat_create_listener.notify = &handle_seat_create;
    wl_signal_add(&ec->seat_created_signal, &ctx->seat_create_listener);

    wl_list_for_each(seat, &ec->seat_list, link) {
        handle_seat_create(&ctx->seat_create_listener, seat);
        wl_signal_emit(&seat->updated_caps_signal, seat);
    }

    return ctx;
}


static int
input_controller_init(struct weston_compositor *ec,
		const struct ivi_layout_interface *layout_if)
{
    int successful_init_stage = 0;
    int init_stage;
    int ret = -1;
    struct input_context *ctx;
    bool init_success = false;

    for (init_stage = 0; (init_stage == successful_init_stage);
            init_stage++) {
        switch(init_stage)
        {
        case 0:
            ctx = create_input_context(ec, layout_if);
            if (NULL != ctx)
                successful_init_stage++;
            break;
        case 1:
            if (wl_global_create(ec->wl_display, &ivi_input_interface, 1,
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
input_controller_module_init(struct weston_compositor *ec,
                             const struct ivi_layout_interface *interface,
                             size_t interface_version)
{
    int ret = -1;

    if (NULL != interface) {
        ret = input_controller_init(ec, interface);

        if (ret >= 0)
            weston_log("ivi-input-controller module loaded successfully!\n");
    }
    return ret;
}
