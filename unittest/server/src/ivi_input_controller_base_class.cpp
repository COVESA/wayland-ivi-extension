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

#include "ivi_input_controller_base_class.hpp"
#include "ivi-controller.h"
#include "ivi-input-server-protocol.h"

struct testInputController
{
    bool m_isInitialized = false;
    struct weston_keyboard_grab_interface *mp_keyboard_grab_interface = nullptr;
    struct weston_pointer_grab_interface *mp_pointer_grab_interface = nullptr;
    struct weston_touch_grab_interface *mp_touch_grab_interface = nullptr;
    struct ivi_input_interface *mp_input_implementation = nullptr;
    wl_global_bind_func_t mp_bind_ivi_input = nullptr;
    wl_resource_destroy_func_t mp_unbind_resource_controller = nullptr;
    wl_notify_func_t mp_input_controller_destroy = nullptr;
    wl_notify_func_t mp_handle_surface_create = nullptr;
    wl_notify_func_t mp_handle_surface_destroy = nullptr;
    wl_notify_func_t mp_handle_seat_create = nullptr;
    wl_notify_func_t mp_handle_seat_destroy = nullptr;
    wl_notify_func_t mp_handle_seat_updated_caps = nullptr;
};

static struct testInputController g_testInputController = {};

/**
 * \brief: Getting 8 callback functions and 4 implementation handlers, then
 *         setting them to g_testInputController
 */
static bool setupForGetFuncCallbacks()
{
    // If setupForGetFuncCallbacks successed, don't need to do it again
    if(g_testInputController.m_isInitialized)
    {
        return true;
    }
    struct weston_compositor l_westonCompositor = {};
    struct ivishell l_iviShell = {};
    l_iviShell.compositor = &l_westonCompositor;
    l_iviShell.interface = &g_iviLayoutInterfaceFake;
    custom_wl_list_init(&l_westonCompositor.seat_list);
    custom_wl_list_init(&l_iviShell.list_surface);
    weston_log_fake.custom_fake = custom_weston_log;

    // First call the input_controller_module_init to get the input_context pointer and bind_ivi_input address
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    uint8_t l_fakePointer = 0;
    struct wl_global *lpp_wlGlobal[] = {(struct wl_global *)&l_fakePointer};
    SET_RETURN_SEQ(wl_global_create, lpp_wlGlobal, 1);
    if(input_controller_module_init(&l_iviShell) < 0)
    {
        return false;
    }
    struct input_context *lp_ctxInput = (struct input_context *)((uintptr_t)wl_list_init_fake.arg0_history[0] - offsetof(struct input_context, resource_list));
    if(wl_list_insert_fake.call_count != 4 || wl_global_create_fake.call_count != 1)
    {
        printf("wl_list_insert_fake should call 4 times, but got %d\n", wl_list_insert_fake.call_count);
        free(lp_ctxInput);
        return false;
    }
    g_testInputController.mp_input_controller_destroy = lp_ctxInput->compositor_destroy_listener.notify;
    g_testInputController.mp_handle_surface_create = lp_ctxInput->surface_created.notify;
    g_testInputController.mp_handle_surface_destroy = lp_ctxInput->surface_destroyed.notify;
    g_testInputController.mp_handle_seat_create = lp_ctxInput->seat_create_listener.notify;
    g_testInputController.mp_bind_ivi_input = wl_global_create_fake.arg4_history[0];

    // Second call the handle_seat_create to get the keyboard, pointer and touch grab pointer, seat destroy and seat update caps address
    SERVER_API_FAKE_LIST(RESET_FAKE);
    struct weston_seat l_westonSeat = {};
    l_westonSeat.seat_name = (char*)"default";
    custom_wl_list_init(&lp_ctxInput->resource_list);
    g_testInputController.mp_handle_seat_create(&lp_ctxInput->seat_create_listener, &l_westonSeat);
    struct seat_ctx *lp_ctxSeat = (struct seat_ctx *)((uintptr_t)wl_list_insert_fake.arg1_history[0] - offsetof(struct seat_ctx, seat_node));
    if(wl_list_insert_fake.call_count != 3)
    {
        printf("wl_list_insert_fake should call 3 times, but got %d\n", wl_list_insert_fake.call_count);
        free(lp_ctxSeat);
        free(lp_ctxInput);
        return false;
    }
    g_testInputController.mp_keyboard_grab_interface = (weston_keyboard_grab_interface*)lp_ctxSeat->keyboard_grab.interface;
    g_testInputController.mp_pointer_grab_interface = (weston_pointer_grab_interface*)lp_ctxSeat->pointer_grab.interface;
    g_testInputController.mp_touch_grab_interface = (weston_touch_grab_interface*)lp_ctxSeat->touch_grab.interface;
    g_testInputController.mp_handle_seat_destroy = lp_ctxSeat->destroy_listener.notify;
    g_testInputController.mp_handle_seat_updated_caps = lp_ctxSeat->updated_caps_listener.notify;

    // Third call the bind_ivi_input to get the input_implementation pointer, unbind_resource_controller address
    SERVER_API_FAKE_LIST(RESET_FAKE);
    g_testInputController.mp_bind_ivi_input(nullptr, lp_ctxInput, 1, 1);
    if(wl_resource_set_implementation_fake.call_count != 1)
    {
        printf("wl_resource_set_implementation_fake should call 1 times, but got %d\n", wl_resource_set_implementation_fake.call_count);
        free(lp_ctxSeat);
        free(lp_ctxInput);
        return false;
    }
    g_testInputController.mp_unbind_resource_controller = wl_resource_set_implementation_fake.arg3_history[0];
    g_testInputController.mp_input_implementation = (struct ivi_input_interface*)wl_resource_set_implementation_fake.arg1_history[0];
    free(lp_ctxSeat);
    free(lp_ctxInput);

    g_testInputController.m_isInitialized = true;
    return true;
}

bool InputControllerBase::initBaseModule()
{
    return setupForGetFuncCallbacks();
}

void InputControllerBase::keyboard_grab_key(struct weston_keyboard_grab *grab, const struct timespec *time, uint32_t key, uint32_t state)
{
    if(g_testInputController.mp_keyboard_grab_interface != nullptr)
    {
        g_testInputController.mp_keyboard_grab_interface->key(grab, time, key, state);
    }
}
void InputControllerBase::keyboard_grab_modifiers(struct weston_keyboard_grab *grab, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    if(g_testInputController.mp_keyboard_grab_interface != nullptr)
    {
        g_testInputController.mp_keyboard_grab_interface->modifiers(grab, serial, mods_depressed, mods_latched, mods_locked, group);
    }
}
void InputControllerBase::keyboard_grab_cancel(struct weston_keyboard_grab *grab)
{
    if(g_testInputController.mp_keyboard_grab_interface != nullptr)
    {
        g_testInputController.mp_keyboard_grab_interface->cancel(grab);
    }
}
void InputControllerBase::pointer_grab_focus(struct weston_pointer_grab *grab)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->focus(grab);
    }
}
void InputControllerBase::pointer_grab_motion(struct weston_pointer_grab *grab, const struct timespec *time, struct weston_pointer_motion_event *event)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->motion(grab, time, event);
    }
}
void InputControllerBase::pointer_grab_button(struct weston_pointer_grab *grab, const struct timespec *time, uint32_t button, uint32_t state)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->button(grab, time, button, state);
    }
}
void InputControllerBase::pointer_grab_axis(struct weston_pointer_grab *grab, const struct timespec *time, struct weston_pointer_axis_event *event)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->axis(grab, time, event);
    }
}
void InputControllerBase::pointer_grab_axis_source(struct weston_pointer_grab *grab, uint32_t source)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->axis_source(grab, source);
    }
}
void InputControllerBase::pointer_grab_frame(struct weston_pointer_grab *grab)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->frame(grab);
    }
}
void InputControllerBase::pointer_grab_cancel(struct weston_pointer_grab *grab)
{
    if(g_testInputController.mp_pointer_grab_interface != nullptr)
    {
        g_testInputController.mp_pointer_grab_interface->cancel(grab);
    }
}
void InputControllerBase::touch_grab_down(struct weston_touch_grab *grab, const struct timespec *time, int touch_id, wl_fixed_t sx, wl_fixed_t sy)
{
    if(g_testInputController.mp_touch_grab_interface != nullptr)
    {
        g_testInputController.mp_touch_grab_interface->down(grab, time, touch_id, sx, sy);
    }
}
void InputControllerBase::touch_grab_up(struct weston_touch_grab *grab, const struct timespec *time, int touch_id)
{
    if(g_testInputController.mp_touch_grab_interface != nullptr)
    {
        g_testInputController.mp_touch_grab_interface->up(grab, time, touch_id);
    }
}
void InputControllerBase::touch_grab_motion(struct weston_touch_grab *grab, const struct timespec *time, int touch_id, wl_fixed_t sx, wl_fixed_t sy)
{
    if(g_testInputController.mp_touch_grab_interface != nullptr)
    {
        g_testInputController.mp_touch_grab_interface->motion(grab, time, touch_id, sx, sy);
    }
}
void InputControllerBase::touch_grab_frame(struct weston_touch_grab *grab)
{
    if(g_testInputController.mp_touch_grab_interface != nullptr)
    {
        g_testInputController.mp_touch_grab_interface->frame(grab);
    }
}
void InputControllerBase::touch_grab_cancel(struct weston_touch_grab *grab)
{
    if(g_testInputController.mp_touch_grab_interface != nullptr)
    {
        g_testInputController.mp_touch_grab_interface->cancel(grab);
    }
}
void InputControllerBase::set_input_focus(struct wl_client *client, struct wl_resource *resource, uint32_t surface, uint32_t device, int32_t enabled)
{
    if(g_testInputController.mp_input_implementation != nullptr)
    {
        g_testInputController.mp_input_implementation->set_input_focus(client, resource, surface, device, enabled);
    }
}
void InputControllerBase::set_input_acceptance(struct wl_client *client, struct wl_resource *resource, uint32_t surface, const char *seat, int32_t accepted)
{
    if(g_testInputController.mp_input_implementation != nullptr)
    {
        g_testInputController.mp_input_implementation->set_input_acceptance(client, resource, surface, seat, accepted);
    }
}
void InputControllerBase::bind_ivi_input(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    if(g_testInputController.mp_bind_ivi_input != nullptr)
    {
        g_testInputController.mp_bind_ivi_input(client, data, version, id);
    }
}
void InputControllerBase::unbind_resource_controller(struct wl_resource *resource)
{
    if(g_testInputController.mp_unbind_resource_controller != nullptr)
    {
        g_testInputController.mp_unbind_resource_controller(resource);
    }
}
void InputControllerBase::input_controller_destroy(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_input_controller_destroy != nullptr)
    {
        g_testInputController.mp_input_controller_destroy(listener, data);
    }
}
void InputControllerBase::handle_surface_create(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_handle_surface_create != nullptr)
    {
        g_testInputController.mp_handle_surface_create(listener, data);
    }
}
void InputControllerBase::handle_surface_destroy(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_handle_surface_destroy != nullptr)
    {
        g_testInputController.mp_handle_surface_destroy(listener, data);
    }
}
void InputControllerBase::handle_seat_create(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_handle_seat_create != nullptr)
    {
        g_testInputController.mp_handle_seat_create(listener, data);
    }
}
void InputControllerBase::handle_seat_destroy(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_handle_seat_destroy != nullptr)
    {
        g_testInputController.mp_handle_seat_destroy(listener, data);
    }
}
void InputControllerBase::handle_seat_updated_caps(struct wl_listener *listener, void *data)
{
    if(g_testInputController.mp_handle_seat_updated_caps != nullptr)
    {
        g_testInputController.mp_handle_seat_updated_caps(listener, data);
    }
}
