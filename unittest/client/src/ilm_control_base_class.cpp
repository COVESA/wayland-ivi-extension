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

#include "ilm_control_base_class.hpp"
#include "ilm_control.h"
#include <cstdio>
extern "C"{
    ILM_EXPORT ilmErrorTypes ilmControl_init(t_ilm_nativedisplay);
}

struct testIlmControl
{
    struct ivi_wm_listener *mp_iviWmListener = nullptr;
    struct ivi_wm_screen_listener *mp_iviWmScreenListener = nullptr;
    struct ivi_input_listener *mp_iviInputListener = nullptr;
    struct ivi_screenshot_listener *mp_iviScreenShotListener = nullptr;
    struct wl_output_listener *mp_wlOutputListener = nullptr;
    struct wl_registry_listener *mp_wlRegistryListener = nullptr;
    struct ilm_control_context *mp_controlContext = nullptr;
    bool m_isInitialized = false;
};

static struct testIlmControl g_testIlmControl = {};

/**
 * \brief: Getting 6 implementation handlers, then setting them to g_testIlmControl
 */
static bool setupForGetFuncCallbacks()
{
    if(g_testIlmControl.m_isInitialized)
    {
        return true;
    }
    /* Step 1: Invoke ilmControl_init
     * Setting the mock steps for calling ilmControl_init.
     * it will help to get registry implementation handlers
     */ 
    CLIENT_API_FAKE_LIST(RESET_FAKE);
    uint64_t l_wlDisplay, l_wlEventQueueFakePointer, l_wlProxyFakePointer;

    struct wl_event_queue *lpp_wlEventQueue[] = {(struct wl_event_queue*)&l_wlEventQueueFakePointer};
    SET_RETURN_SEQ(wl_display_create_queue, lpp_wlEventQueue, 1);

    struct wl_proxy *lpp_wlProxy[] = {(struct wl_proxy*)&l_wlProxyFakePointer};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lpp_wlProxy, 1);

    ilmControl_init((t_ilm_nativedisplay)&l_wlDisplay);
    if(wl_proxy_add_listener_fake.call_count != 1)
    {
        printf("wl_proxy_add_listener need call 1 times\n");
        return false;
    }
    g_testIlmControl.mp_wlRegistryListener = (struct wl_registry_listener*)wl_proxy_add_listener_fake.arg1_history[0];
    g_testIlmControl.mp_controlContext = (struct ilm_control_context*)wl_proxy_add_listener_fake.arg2_history[0];

    /* Step 2: Invoke global event of registry interface
     * it will help to get ivi_wm implementation handlers
     */
    g_testIlmControl.mp_wlRegistryListener->global((void*)(&g_testIlmControl.mp_controlContext->wl), (struct wl_registry *)&l_wlProxyFakePointer, 2, "ivi_wm", 1);
    if(wl_proxy_add_listener_fake.call_count != 2)
    {
        printf("wl_proxy_add_listener need call 2 times\n");
        return false;
    }
    g_testIlmControl.mp_iviWmListener = (struct ivi_wm_listener*)wl_proxy_add_listener_fake.arg1_history[1];

    /* Step 3: Invoke global event of registry interface
     * it will help to get ivi_input implementation handlers
     */
    g_testIlmControl.mp_wlRegistryListener->global((void*)(&g_testIlmControl.mp_controlContext->wl), (struct wl_registry *)&l_wlProxyFakePointer, 2, "ivi_input", 1);
    if(wl_proxy_add_listener_fake.call_count != 3)
    {
        printf("wl_proxy_add_listener need call 3 times\n");
        return false;
    }
    g_testIlmControl.mp_iviInputListener = (struct ivi_input_listener*)wl_proxy_add_listener_fake.arg1_history[2];

    /* Step 4: Invoke global event of registry interface
     * it will help to get wl_output and ivi_wm_screen implementation handlers
     */
    g_testIlmControl.mp_wlRegistryListener->global((void*)(&g_testIlmControl.mp_controlContext->wl), (struct wl_registry *)&l_wlProxyFakePointer, 2, "wl_output", 1);
    if(wl_proxy_add_listener_fake.call_count != 5)
    {
        printf("wl_proxy_add_listener need call 5 times\n");
        return false;
    }
    g_testIlmControl.mp_wlOutputListener = (struct wl_output_listener*)wl_proxy_add_listener_fake.arg1_history[3];
    g_testIlmControl.mp_iviWmScreenListener = (struct ivi_wm_screen_listener*)wl_proxy_add_listener_fake.arg1_history[4];
    free(wl_proxy_add_listener_fake.arg2_history[4]);
    
    /* Step 5: Invoke ilm_takeSurfaceScreenshot
     * it will help to get ivi_wm_screenshot implementation handlers
     */
    //@todo

    g_testIlmControl.m_isInitialized = true;
    return true;
}

bool IlmControlInitBase::initBaseModule()
{
    return setupForGetFuncCallbacks();
}

void IlmControlInitBase::wm_screen_listener_screen_id(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t id)
{
    if(g_testIlmControl.mp_iviWmScreenListener != nullptr)
    {
        g_testIlmControl.mp_iviWmScreenListener->screen_id(data, ivi_wm_screen, id);
    }
}
void IlmControlInitBase::wm_screen_listener_layer_added(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t layer_id)
{
    if(g_testIlmControl.mp_iviWmScreenListener != nullptr)
    {
        g_testIlmControl.mp_iviWmScreenListener->layer_added(data, ivi_wm_screen, layer_id);
    }
}
void IlmControlInitBase::wm_screen_listener_connector_name(void *data, struct ivi_wm_screen *ivi_wm_screen, const char *process_name)
{
    if(g_testIlmControl.mp_iviWmScreenListener != nullptr)
    {
        g_testIlmControl.mp_iviWmScreenListener->connector_name(data, ivi_wm_screen, process_name);
    }
}
void IlmControlInitBase::wm_screen_listener_error(void *data, struct ivi_wm_screen *ivi_wm_screen, uint32_t error, const char *message)
{
    if(g_testIlmControl.mp_iviWmScreenListener != nullptr)
    {
        g_testIlmControl.mp_iviWmScreenListener->error(data, ivi_wm_screen, error, message);
    }
}
void IlmControlInitBase::wm_listener_surface_visibility(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t visibility)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_visibility(data, ivi_wm, surface_id, visibility);
    }
}
void IlmControlInitBase::wm_listener_layer_visibility(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t visibility)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_visibility(data, ivi_wm, layer_id, visibility);
    }
}
void IlmControlInitBase::wm_listener_surface_opacity(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, wl_fixed_t opacity)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_opacity(data, ivi_wm, surface_id, opacity);
    }
}
void IlmControlInitBase::wm_listener_layer_opacity(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, wl_fixed_t opacity)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_opacity(data, ivi_wm, layer_id, opacity);
    }
}
void IlmControlInitBase::wm_listener_surface_source_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_source_rectangle(data, ivi_wm, surface_id, x, y, width, height);
    }
}
void IlmControlInitBase::wm_listener_layer_source_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_source_rectangle(data, ivi_wm, layer_id, x, y, width, height);
    }
}
void IlmControlInitBase::wm_listener_surface_destination_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_destination_rectangle(data, ivi_wm, surface_id, x, y, width, height);
    }
}
void IlmControlInitBase::wm_listener_layer_destination_rectangle(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_destination_rectangle(data, ivi_wm, layer_id, x, y, width, height);
    }
}
void IlmControlInitBase::wm_listener_surface_created(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_created(data, ivi_wm, surface_id);
    }
}
void IlmControlInitBase::wm_listener_layer_created(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_created(data, ivi_wm, layer_id);
    }
}
void IlmControlInitBase::wm_listener_surface_destroyed(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_destroyed(data, ivi_wm, surface_id);
    }
}
void IlmControlInitBase::wm_listener_layer_destroyed(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_destroyed(data, ivi_wm, layer_id);
    }
}
void IlmControlInitBase::wm_listener_surface_error(void *data, struct ivi_wm *ivi_wm, uint32_t object_id, uint32_t error, const char *message)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_error(data, ivi_wm, object_id, error, message);
    }
}
void IlmControlInitBase::wm_listener_layer_error(void *data, struct ivi_wm *ivi_wm, uint32_t object_id, uint32_t error, const char *message)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_error(data, ivi_wm, object_id, error, message);
    }
}
void IlmControlInitBase::wm_listener_surface_size(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, int32_t width, int32_t height)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_size(data, ivi_wm, surface_id, width, height);
    }
}
void IlmControlInitBase::wm_listener_surface_stats(void *data, struct ivi_wm *ivi_wm, uint32_t surface_id, uint32_t frame_count, uint32_t pid)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->surface_stats(data, ivi_wm, surface_id, frame_count, pid);
    }
}
void IlmControlInitBase::wm_listener_layer_surface_added(void *data, struct ivi_wm *ivi_wm, uint32_t layer_id, uint32_t surface_id)
{
    if(g_testIlmControl.mp_iviWmListener != nullptr)
    {
        g_testIlmControl.mp_iviWmListener->layer_surface_added(data, ivi_wm, layer_id, surface_id);
    }
}
void IlmControlInitBase::input_listener_seat_created(void *data, struct ivi_input *ivi_input, const char *name, uint32_t capabilities)
{
    if(g_testIlmControl.mp_iviInputListener != nullptr)
    {
        g_testIlmControl.mp_iviInputListener->seat_created(data, ivi_input, name, capabilities);
    }
}
void IlmControlInitBase::input_listener_seat_capabilities(void *data, struct ivi_input *ivi_input, const char *name, uint32_t capabilities)
{
    if(g_testIlmControl.mp_iviInputListener != nullptr)
    {
        g_testIlmControl.mp_iviInputListener->seat_capabilities(data, ivi_input, name, capabilities);
    }
}
void IlmControlInitBase::input_listener_seat_destroyed(void *data, struct ivi_input *ivi_input, const char *name)
{
    if(g_testIlmControl.mp_iviInputListener != nullptr)
    {
        g_testIlmControl.mp_iviInputListener->seat_destroyed(data, ivi_input, name);
    }
}
void IlmControlInitBase::input_listener_input_focus(void *data, struct ivi_input *ivi_input, uint32_t surface, uint32_t device, int32_t enabled)
{
    if(g_testIlmControl.mp_iviInputListener != nullptr)
    {
        g_testIlmControl.mp_iviInputListener->input_focus(data, ivi_input, surface, device, enabled);
    }
}
void IlmControlInitBase::input_listener_input_acceptance(void *data, struct ivi_input *ivi_input, uint32_t surface, const char *seat, int32_t accepted)
{
    if(g_testIlmControl.mp_iviInputListener != nullptr)
    {
        g_testIlmControl.mp_iviInputListener->input_acceptance(data, ivi_input, surface, seat, accepted);
    }
}
void IlmControlInitBase::output_listener_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make, const char *model, int32_t transform)
{
    if(g_testIlmControl.mp_wlOutputListener != nullptr)
    {
        g_testIlmControl.mp_wlOutputListener->geometry(data, wl_output, x, y, physical_width, physical_height, subpixel, make, model, transform);
    }
}
void IlmControlInitBase::output_listener_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    if(g_testIlmControl.mp_wlOutputListener != nullptr)
    {
        g_testIlmControl.mp_wlOutputListener->mode(data, wl_output, flags, width, height, refresh);
    }
}
void IlmControlInitBase::output_listener_done(void *data, struct wl_output *wl_output)
{
    if(g_testIlmControl.mp_wlOutputListener != nullptr)
    {
        g_testIlmControl.mp_wlOutputListener->done(data, wl_output);
    }
}
void IlmControlInitBase::output_listener_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
    if(g_testIlmControl.mp_wlOutputListener != nullptr)
    {
        g_testIlmControl.mp_wlOutputListener->scale(data, wl_output, factor);
    }
}
void IlmControlInitBase::registry_handle_control(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
    if(g_testIlmControl.mp_wlRegistryListener != nullptr)
    {
        g_testIlmControl.mp_wlRegistryListener->global(data, wl_registry, name, interface, version);
    }
}
void IlmControlInitBase::registry_handle_control_remove(void *data, struct wl_registry *wl_registry, uint32_t name)
{
    if(g_testIlmControl.mp_wlRegistryListener != nullptr)
    {
        g_testIlmControl.mp_wlRegistryListener->global_remove(data, wl_registry, name);
    }
}
void IlmControlInitBase::screenshot_done(void *data, struct ivi_screenshot *ivi_screenshot, int32_t fd, int32_t width, int32_t height, int32_t stride, uint32_t format, uint32_t timestamp)
{
    if(g_testIlmControl.mp_iviScreenShotListener != nullptr)
    {
        g_testIlmControl.mp_iviScreenShotListener->done(data, ivi_screenshot, fd, width, height, stride, format, timestamp);
    }
}
void IlmControlInitBase::screenshot_error(void *data, struct ivi_screenshot *ivi_screenshot, uint32_t error, const char *message)
{
    if(g_testIlmControl.mp_iviScreenShotListener != nullptr)
    {
        g_testIlmControl.mp_iviScreenShotListener->error(data, ivi_screenshot, error, message);
    }
}