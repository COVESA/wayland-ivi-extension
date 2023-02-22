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

#include "ivi_controller_base_class.hpp"

struct testController
{
    bool m_isInitialized = false;
    wl_notify_func_t m_send_surface_prop = nullptr;
    wl_notify_func_t m_send_layer_prop = nullptr;
    wl_notify_func_t m_output_destroyed_event = nullptr;
    wl_notify_func_t m_output_resized_event = nullptr;
    wl_notify_func_t m_output_created_event = nullptr;
    wl_notify_func_t m_layer_event_create = nullptr;
    wl_notify_func_t m_layer_event_remove = nullptr;
    wl_notify_func_t m_surface_event_create = nullptr;
    wl_notify_func_t m_surface_event_remove = nullptr;
    wl_notify_func_t m_surface_event_configure = nullptr;
    wl_notify_func_t m_ivi_shell_destroy = nullptr;
    wl_notify_func_t m_surface_committed = nullptr;
    wl_notify_func_t m_controller_screenshot_notify = nullptr;
    wl_notify_func_t m_ivi_shell_client_destroy = nullptr;
    wl_notify_func_t m_screenshot_output_destroyed = nullptr;
    wl_event_loop_idle_func_t m_launch_client_process = nullptr;
    wl_global_bind_func_t m_bind_ivi_controller = nullptr;
    wl_resource_destroy_func_t m_unbind_resource_controller = nullptr;
    wl_resource_destroy_func_t m_screenshot_frame_listener_destroy =nullptr;
    wl_resource_destroy_func_t m_destroy_ivicontroller_screen = nullptr;
    struct ivi_wm_interface *mp_iviWmInterface = nullptr;
    struct ivi_wm_screen_interface *mp_iviWmScreenInterface = nullptr;
};

static struct testController g_testController = {};

/**
 * \brief: custom function of weston_config_section_get_int, will help to 
 *         ivi_client_name and bkgnd_surface_id
 */
static int custom_weston_config_section_get_int(struct weston_config_section *section, const char *key, int32_t *value, int32_t default_value)
{
    struct ivishell *lp_iviShell = (struct ivishell *)(uintptr_t(value) - offsetof(struct ivishell, bkgnd_surface_id));
    lp_iviShell->ivi_client_name = strdup("ilm_tests");
    lp_iviShell->bkgnd_surface_id = 10;
    return 0;
}

/**
 * \brief: Getting 20 callback functions and 2 implementation handlers, then
 *         setting them to g_testController
 */
static bool setupForGetFuncCallbacks()
{
    // If setupForGetFuncCallbacks successed, don't need to do it again
    if(g_testController.m_isInitialized)
    {
        return true;
    }

    /* Step 1: Getting wet_module_init success
     * Setting the mock steps for calling wet_module_init success.
     * it will help to get 11 callback functions
     */ 
    int8_t l_fakePointer = 0;
    weston_log_fake.custom_fake = custom_weston_log;
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    struct weston_compositor l_westonCompositor = {};
    custom_wl_list_init(&l_westonCompositor.output_list);

    // Setup stub for weston_plugin_api_get, it will call once time when execute wet_module_init
    struct ivi_layout_interface* lp_iviLayoutInterface = &g_iviLayoutInterfaceFake;
    SET_RETURN_SEQ(weston_plugin_api_get, (const void**)&lp_iviLayoutInterface, 1);

    // To bypass the get_config
    struct weston_config_section *lpp_westonSection[] = {(weston_config_section*)&l_fakePointer};
    struct weston_config *lpp_westonConfig[] = {(weston_config*)&l_fakePointer};
    int lp_failedResut[] = {l_fakePointer};
    SET_RETURN_SEQ(wet_get_config, lpp_westonConfig, 1);
    SET_RETURN_SEQ(weston_config_get_section, lpp_westonSection, 1);
    SET_RETURN_SEQ(weston_config_next_section, lp_failedResut, 1);
    weston_config_section_get_int_fake.custom_fake = custom_weston_config_section_get_int;

    // To bypass init_ivi_shell
    // To bypass setup_ivi_controller_server
    struct wl_global *lpp_wlGlobal[] = {(wl_global*)&l_fakePointer};
    SET_RETURN_SEQ(wl_global_create, lpp_wlGlobal, 1);

    // To bypass load_input_module
    // To bypass load_id_agent_module
    int lp_failedResut_1[] = {0, 0,-1};
    SET_RETURN_SEQ(weston_config_section_get_string, lp_failedResut_1, 3);

    if(wet_module_init(&l_westonCompositor, nullptr, nullptr) != 0)
    {
        printf("Failed to call wet_module_init\n");
        return false;
    }

    // Get function pointers via ivishell object
    if(wl_global_create_fake.call_count != 1)
    {
        printf("wl_global_create_fake should call 1 time, but got: %d\n", wl_global_create_fake.call_count);
        return false;
    }
    struct ivishell * lp_iviShell = (struct ivishell*)wl_global_create_fake.arg3_history[0];
    if(add_listener_create_layer_fake.call_count != 1 || add_listener_remove_layer_fake.call_count != 1 ||
       add_listener_create_surface_fake.call_count != 1 || add_listener_remove_surface_fake.call_count != 1 ||
       add_listener_configure_surface_fake.call_count != 1 || wl_event_loop_add_idle_fake.call_count != 1)
    {
        printf("wl_event_loop_add_idle_fake should call 1 time, but got: %d\n", wl_event_loop_add_idle_fake.call_count);
        return false;
    }

    g_testController.m_layer_event_create = lp_iviShell->layer_created.notify;
    g_testController.m_layer_event_remove = lp_iviShell->layer_removed.notify;
    g_testController.m_surface_event_create = lp_iviShell->surface_created.notify;
    g_testController.m_surface_event_remove = lp_iviShell->surface_removed.notify;
    g_testController.m_surface_event_configure = lp_iviShell->surface_configured.notify;
    g_testController.m_output_created_event = lp_iviShell->output_created.notify;
    g_testController.m_output_destroyed_event = lp_iviShell->output_destroyed.notify;
    g_testController.m_output_resized_event = lp_iviShell->output_resized.notify;
    g_testController.m_bind_ivi_controller = wl_global_create_fake.arg4_history[0];
    g_testController.m_ivi_shell_destroy = lp_iviShell->destroy_listener.notify;
    g_testController.m_ivi_shell_client_destroy = lp_iviShell->client_destroy_listener.notify;
    g_testController.m_launch_client_process = wl_event_loop_add_idle_fake.arg1_history[0];

    /* Step 2: Invoke the bind_ivi_controller
     * Setting the mock steps for calling bind_ivi_controller.
     * it will help to get 1 callback function and ivi_wm implementation handlers
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    // To bypass wl_resource_create
    struct wl_resource *lpp_wlResource[]={(struct wl_resource *)&l_fakePointer};
    SET_RETURN_SEQ(wl_resource_create, lpp_wlResource, 1);

    g_testController.m_bind_ivi_controller(nullptr, lp_iviShell, 1, 1);

    if(wl_resource_set_implementation_fake.call_count != 1)
    {
        printf("phase 2: wl_resource_set_implementation should call 1 time, but got: %d\n", wl_resource_set_implementation_fake.call_count);
        return false;
    }
    struct ivicontroller *lp_iviController = (struct ivicontroller*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct ivicontroller, link));
    g_testController.mp_iviWmInterface = (struct ivi_wm_interface*)wl_resource_set_implementation_fake.arg1_history[0];
    g_testController.m_unbind_resource_controller = wl_resource_set_implementation_fake.arg3_history[0];

    /* Step 3: Invoke the create_screen event of ivi_wm interface
     * Setting the mock steps for trigger create_screen event.
     * it will help to get 1 callback function and ivi_wm_screen implementation handlers
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    struct ivicontroller l_iviController;
    struct weston_head l_westonHead;
    struct weston_output l_westonOutput;
    struct iviscreen l_iviScreen;
    l_westonOutput.name = (char *)"screen1";
    l_iviController.shell = lp_iviShell;
    l_westonHead.output = &l_westonOutput;

    l_iviScreen.output = &l_westonOutput;
    l_iviScreen.id_screen = 0;
    custom_wl_list_insert(&lp_iviShell->list_screen, &l_iviScreen.link);

    // To bypass wl_resource_get_user_data
    void *lpp_getUserData [] = {&l_westonHead, &l_iviController};
    SET_RETURN_SEQ(wl_resource_get_user_data, lpp_getUserData, 2);

    // To bypass wl_resource_create
    SET_RETURN_SEQ(wl_resource_create, lpp_wlResource, 1);

    g_testController.mp_iviWmInterface->create_screen(nullptr, nullptr, nullptr, 1);
    if(wl_resource_set_implementation_fake.call_count != 1)
    {
        printf("phase 3: wl_resource_set_implementation should call 1 time, but got: %d\n", wl_resource_set_implementation_fake.call_count);
        return false;
    }
    g_testController.mp_iviWmScreenInterface = (struct ivi_wm_screen_interface*)wl_resource_set_implementation_fake.arg1_history[0];
    g_testController.m_destroy_ivicontroller_screen = wl_resource_set_implementation_fake.arg3_history[0];

    /* Step 4: Invoke the screenshot event of ivi_wm_screen interface
     * Setting the mock steps for trigger screenshot event.
     * it will help to get 3 callback functions
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    //To bypass wl_resource_get_user_data
    lpp_getUserData [0] = {&l_iviScreen};
    SET_RETURN_SEQ(wl_resource_get_user_data, lpp_getUserData, 1);

    // To bypass wl_resource_create
    SET_RETURN_SEQ(wl_resource_create, lpp_wlResource, 1);

    g_testController.mp_iviWmScreenInterface->screenshot(nullptr, nullptr, 1);
    if(wl_resource_set_implementation_fake.call_count != 1)
    {
        printf("phase 4: wl_resource_set_implementation should call 1 time, but got: %d\n", wl_resource_set_implementation_fake.call_count);
        return false;
    }
    struct screenshot_frame_listener *lp_screenShotFrame = (struct screenshot_frame_listener*)wl_resource_set_implementation_fake.arg2_history[0];
    g_testController.m_screenshot_frame_listener_destroy = wl_resource_set_implementation_fake.arg3_history[0];
    g_testController.m_screenshot_output_destroyed = lp_screenShotFrame->output_destroyed.notify;
    g_testController.m_controller_screenshot_notify = lp_screenShotFrame->frame_listener.notify;

    /* Step 5: Invoke the surface_event_create
     * Setting the mock steps for calling surface_event_create.
     * it will help to get 2 callback functions
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    // To bypass the get_id_of_surface
    uint32_t l_surfaceId[] = {10};
    SET_RETURN_SEQ(get_id_of_surface, l_surfaceId, 1);
    lp_iviShell->bkgnd_surface_id = 0;

    // To bypass the surface_get_weston_surface
    struct weston_surface l_westonSurface;
    struct weston_surface *lpp_westonSurface[] = {&l_westonSurface};
    SET_RETURN_SEQ(surface_get_weston_surface, lpp_westonSurface, 1);
    custom_wl_list_init(&l_westonSurface.commit_signal.listener_list);

    g_testController.m_surface_event_create(&lp_iviShell->surface_created, nullptr);
    if(wl_list_init_fake.call_count != 1)
    {
        printf("wl_list_init_fake should call 1 time, but got %d\n", wl_list_init_fake.call_count);
        return false;
    }
    struct ivisurface *lp_iviSurf = (struct ivisurface *)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivisurface, notification_list));

    if(surface_add_listener_fake.call_count != 1)
    {
        printf("surface_add_listener should call 1 time, but got: %d\n", surface_add_listener_fake.call_count);
        return false;
    }
    g_testController.m_send_surface_prop = lp_iviSurf->property_changed.notify;
    g_testController.m_surface_committed = lp_iviSurf->committed.notify;

    /* Step 6: Invoke the layer_event_create
     * Setting the mock steps for calling layer_event_create.
     * it will help to get 1 callback function
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    wl_list_init_fake.custom_fake = custom_wl_list_init;

    // To bypass the get_id_of_layer
    uint32_t l_layerId[] = {10};
    SET_RETURN_SEQ(get_id_of_layer, l_layerId, 1);

    g_testController.m_layer_event_create(&lp_iviShell->layer_created, nullptr);
    if(wl_list_init_fake.call_count != 1)
    {
        printf("wl_list_init_fake should call 1 time, but got %d\n", wl_list_init_fake.call_count);
        return false;
    }
    struct ivilayer *lp_iviLayer = (struct ivilayer *)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivilayer, notification_list));

    if(layer_add_listener_fake.call_count != 1)
    {
        printf("layer_add_listener_fake should call 1 time, but got: %d\n", layer_add_listener_fake.call_count);
        return false;
    }
    g_testController.m_send_layer_prop = lp_iviLayer->property_changed.notify;

    free(lp_iviShell->ivi_client_name);
    free(lp_iviShell);
    free(lp_iviController);
    free(lp_screenShotFrame);
    free(lp_iviSurf);
    free(lp_iviLayer);

    g_testController.m_isInitialized = true;
    return true;
}

bool ControllerBase::initBaseModule()
{
    return setupForGetFuncCallbacks();
}

void ControllerBase::controller_screen_destroy(struct wl_client *client, struct wl_resource *resource)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->destroy(client, resource);
    }
}
void ControllerBase::controller_screen_clear(struct wl_client *client, struct wl_resource *resource)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->clear(client, resource);
    }
}
void ControllerBase::controller_screen_add_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->add_layer(client, resource, layer_id);
    }
}
void ControllerBase::controller_screen_remove_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->remove_layer(client, resource, layer_id);
    }
}
void ControllerBase::controller_screen_screenshot(struct wl_client *client, struct wl_resource *resource,uint32_t screenshot)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->screenshot(client, resource, screenshot);
    }
}
void ControllerBase::controller_screen_get(struct wl_client *client, struct wl_resource *resource, int32_t param)
{
    if(g_testController.mp_iviWmScreenInterface != nullptr)
    {
        g_testController.mp_iviWmScreenInterface->get(client, resource, param);
    }
}
void ControllerBase::controller_commit_changes(struct wl_client *client, struct wl_resource *resource)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->commit_changes(client, resource);
    }
}
void ControllerBase::controller_create_screen(struct wl_client *client,struct wl_resource *resource, struct wl_resource *output, uint32_t id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->create_screen(client, resource, output, id);
    }
}
void ControllerBase::controller_set_surface_visibility(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, uint32_t visibility)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_surface_visibility(client, resource, surface_id, visibility);
    }
}
void ControllerBase::controller_set_layer_visibility(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, uint32_t visibility)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_layer_visibility(client, resource, layer_id, visibility);
    }
}
void ControllerBase::controller_set_surface_opacity(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id,wl_fixed_t opacity)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_surface_opacity(client, resource, surface_id, opacity);
    }
}
void ControllerBase::controller_set_layer_opacity(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, wl_fixed_t opacity)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_layer_opacity(client, resource, layer_id, opacity);
    }
}
void ControllerBase::controller_set_surface_source_rectangle(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_surface_source_rectangle(client, resource, surface_id, x, y, width, height);
    }
}
void ControllerBase::controller_set_layer_source_rectangle(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_layer_source_rectangle(client, resource, layer_id, x, y, width, height);
    }
}
void ControllerBase::controller_set_surface_destination_rectangle(struct wl_client *client, struct wl_resource *resource,uint32_t surface_id,int32_t x,int32_t y,int32_t width,int32_t height)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_surface_destination_rectangle(client, resource, surface_id, x, y, width, height);
    }
}
void ControllerBase::controller_set_layer_destination_rectangle(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t x,int32_t y,int32_t width,int32_t height)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_layer_destination_rectangle(client, resource, layer_id, x, y, width, height);
    }
}
void ControllerBase::controller_surface_sync(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t sync_state)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->surface_sync(client, resource, surface_id, sync_state);
    }
}
void ControllerBase::controller_layer_sync(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, int32_t sync_state)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->layer_sync(client, resource, layer_id, sync_state);
    }
}
void ControllerBase::controller_surface_get(struct wl_client *client,struct wl_resource *resource,uint32_t surface_id,int32_t param)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->surface_get(client, resource, surface_id, param);
    }
}
void ControllerBase::controller_layer_get(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t param)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->layer_get(client, resource, layer_id, param);
    }
}
void ControllerBase::controller_surface_screenshot(struct wl_client *client, struct wl_resource *resource, uint32_t screenshot, uint32_t surface_id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->surface_screenshot(client, resource, screenshot, surface_id);
    }
}
void ControllerBase::controller_set_surface_type(struct wl_client *client, struct wl_resource *resource, uint32_t surface_id, int32_t type)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->set_surface_type(client, resource, surface_id, type);
    }
}
void ControllerBase::controller_layer_clear(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->layer_clear(client, resource, layer_id);
    }
}
void ControllerBase::controller_layer_add_surface(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,uint32_t surface_id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->layer_add_surface(client, resource, layer_id, surface_id);
    }
}
void ControllerBase::controller_layer_remove_surface(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id, uint32_t surface_id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->layer_remove_surface(client, resource, layer_id, surface_id);
    }
}
void ControllerBase::controller_create_layout_layer(struct wl_client *client,struct wl_resource *resource,uint32_t layer_id,int32_t width,int32_t height)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->create_layout_layer(client, resource, layer_id, width, height);
    }
}
void ControllerBase::controller_destroy_layout_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer_id)
{
    if(g_testController.mp_iviWmInterface != nullptr)
    {
        g_testController.mp_iviWmInterface->destroy_layout_layer(client, resource, layer_id);
    }
}
void ControllerBase::send_surface_prop(struct wl_listener *listener, void *data)
{
    if(g_testController.m_send_surface_prop != nullptr)
    {
        g_testController.m_send_surface_prop(listener, data);
    }
}
void ControllerBase::send_layer_prop(struct wl_listener *listener, void *data)
{
    if(g_testController.m_send_layer_prop != nullptr)
    {
        g_testController.m_send_layer_prop(listener, data);
    }
}
void ControllerBase::output_destroyed_event(struct wl_listener *listener, void *data)
{
    if(g_testController.m_output_destroyed_event != nullptr)
    {
        g_testController.m_output_destroyed_event(listener, data);
    }
}
void ControllerBase::output_resized_event(struct wl_listener *listener, void *data)
{
    if(g_testController.m_output_resized_event != nullptr)
    {
        g_testController.m_output_resized_event(listener, data);
    }
}
void ControllerBase::output_created_event(struct wl_listener *listener, void *data)
{
    if(g_testController.m_output_created_event != nullptr)
    {
        g_testController.m_output_created_event(listener, data);
    }
}
void ControllerBase::layer_event_create(struct wl_listener *listener, void *data)
{
    if(g_testController.m_layer_event_create != nullptr)
    {
        g_testController.m_layer_event_create(listener, data);
    }
}
void ControllerBase::layer_event_remove(struct wl_listener *listener, void *data)
{
    if(g_testController.m_layer_event_remove != nullptr)
    {
        g_testController.m_layer_event_remove(listener, data);
    }
}
void ControllerBase::surface_event_create(struct wl_listener *listener, void *data)
{
    if(g_testController.m_surface_event_create != nullptr)
    {
        g_testController.m_surface_event_create(listener, data);
    }
}
void ControllerBase::surface_event_remove(struct wl_listener *listener, void *data)
{
    if(g_testController.m_surface_event_remove != nullptr)
    {
        g_testController.m_surface_event_remove(listener, data);
    }
}
void ControllerBase::surface_event_configure(struct wl_listener *listener, void *data)
{
    if(g_testController.m_surface_event_configure != nullptr)
    {
        g_testController.m_surface_event_configure(listener, data);
    }
}
void ControllerBase::ivi_shell_destroy(struct wl_listener *listener, void *data)
{
    if(g_testController.m_ivi_shell_destroy != nullptr)
    {
        g_testController.m_ivi_shell_destroy(listener, data);
    }
}
void ControllerBase::surface_committed(struct wl_listener *listener, void *data)
{
    if(g_testController.m_surface_committed != nullptr)
    {
        g_testController.m_surface_committed(listener, data);
    }
}
void ControllerBase::controller_screenshot_notify(struct wl_listener *listener, void *data)
{
    if(g_testController.m_controller_screenshot_notify != nullptr)
    {
        g_testController.m_controller_screenshot_notify(listener, data);
    }
}
void ControllerBase::ivi_shell_client_destroy(struct wl_listener *listener, void *data)
{
    if(g_testController.m_ivi_shell_client_destroy != nullptr)
    {
        g_testController.m_ivi_shell_client_destroy(listener, data);
    }
}
void ControllerBase::screenshot_output_destroyed(struct wl_listener *listener, void *data)
{
    if(g_testController.m_screenshot_output_destroyed != nullptr)
    {
        g_testController.m_screenshot_output_destroyed(listener, data);
    }
}
void ControllerBase::launch_client_process(void *data)
{
    if(g_testController.m_launch_client_process != nullptr)
    {
        g_testController.m_launch_client_process(data);
    }
}
void ControllerBase::bind_ivi_controller(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    if(g_testController.m_bind_ivi_controller != nullptr)
    {
        g_testController.m_bind_ivi_controller(client, data, version, id);
    }
}
void ControllerBase::unbind_resource_controller(struct wl_resource *resource)
{
    if(g_testController.m_unbind_resource_controller != nullptr)
    {
        g_testController.m_unbind_resource_controller(resource);
    }
}
void ControllerBase::screenshot_frame_listener_destroy(struct wl_resource *resource)
{
    if(g_testController.m_screenshot_frame_listener_destroy != nullptr)
    {
        g_testController.m_screenshot_frame_listener_destroy(resource);
    }
}
void ControllerBase::destroy_ivicontroller_screen(struct wl_resource *resource)
{
    if(g_testController.m_destroy_ivicontroller_screen != nullptr)
    {
        g_testController.m_destroy_ivicontroller_screen(resource);
    }
}
