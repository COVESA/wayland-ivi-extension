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

#include "ivi_id_agent_base_class.hpp"

struct testIdAgent
{
    bool m_isInitialized = false;
    wl_notify_func_t m_desktop_surface_event_configure = nullptr;
    wl_notify_func_t m_surface_event_remove = nullptr;
    wl_notify_func_t m_id_agent_module_deinit = nullptr;
};

static struct testIdAgent g_testIdAgent = {};

/**
 * \brief: Getting 3 callback functions, then setting them to g_testIdAgent
 */
static bool setupForGetFuncCallbacks()
{
    if(g_testIdAgent.m_isInitialized)
    {
        return true;
    }

    /* Getting id_agent_module_init success
     * Setting the mock steps for calling id_agent_module_init success.
     * it will help to get 3 callback functions
     */ 
    IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
    SERVER_API_FAKE_LIST(RESET_FAKE);
    struct weston_compositor l_westonCompositor = {};
    uint8_t l_fakePointer = 0;

    weston_log_fake.custom_fake = custom_weston_log;

    struct weston_config *lpp_westonConfig[] = {(weston_config*)&l_fakePointer};
    SET_RETURN_SEQ(wet_get_config, lpp_westonConfig, 1);

    if(id_agent_module_init(&l_westonCompositor, &g_iviLayoutInterfaceFake) == IVI_FAILED)
    {
        return false;
    }

    struct ivi_id_agent *lp_idAgent = (struct ivi_id_agent*)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivi_id_agent, app_list));
    g_testIdAgent.m_desktop_surface_event_configure = lp_idAgent->desktop_surface_configured.notify;
    g_testIdAgent.m_surface_event_remove = lp_idAgent->surface_removed.notify;
    g_testIdAgent.m_id_agent_module_deinit = lp_idAgent->destroy_listener.notify;
    free(lp_idAgent);

    g_testIdAgent.m_isInitialized = true;
    return true;
}

bool IdAgentBase::initBaseModule()
{
    return setupForGetFuncCallbacks();
}

void IdAgentBase::desktop_surface_event_configure(struct wl_listener *listener, void *data)
{
    if(g_testIdAgent.m_desktop_surface_event_configure != nullptr)
    {
        g_testIdAgent.m_desktop_surface_event_configure(listener, data);
    }
}

void IdAgentBase::surface_event_remove(struct wl_listener *listener, void *data)
{
    if(g_testIdAgent.m_surface_event_remove != nullptr)
    {
        g_testIdAgent.m_surface_event_remove(listener, data);
    }
}

void IdAgentBase::id_agent_module_deinit(struct wl_listener *listener, void *data)
{
    if(g_testIdAgent.m_id_agent_module_deinit != nullptr)
    {
        g_testIdAgent.m_id_agent_module_deinit(listener, data);
    }
}
