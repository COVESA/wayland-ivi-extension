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

#ifndef IVI_ID_AGENT_BASE_CLASS
#define IVI_ID_AGENT_BASE_CLASS

#include <string>
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"

extern "C"{
    WL_EXPORT int32_t id_agent_module_init(struct weston_compositor *compositor,
        const struct ivi_layout_interface *interface);
}

/**
 * @note struct ivi_id_agent is in ivi-id-agent.c
 * @note any changes from the original file should be updated here
*/
struct ivi_id_agent
{
    uint32_t default_behavior_set;
    uint32_t default_surface_id;
    uint32_t default_surface_id_max;
    struct wl_list app_list;
    struct weston_compositor *compositor;
    const struct ivi_layout_interface *interface;

    struct wl_listener desktop_surface_configured;
    struct wl_listener destroy_listener;
    struct wl_listener surface_removed;
};

/**
 * @note struct db_elem is in ivi-id-agent.c
 * @note any changes from the original file should be updated here
*/
struct db_elem
{
    struct wl_list link;
    uint32_t surface_id;
    char *cfg_app_id;
    char *cfg_title;
    struct ivi_layout_surface *layout_surface;
};

/**
 * \brief: IdAgentBase will help to show all callback functions in ivi-id-aent-modules, there are 3 callbacks
 */
class IdAgentBase
{
public:
    IdAgentBase() {}
    virtual ~IdAgentBase() {};
    virtual bool initBaseModule();
    void desktop_surface_event_configure(struct wl_listener *listener, void *data);
    void surface_event_remove(struct wl_listener *listener, void *data);
    void id_agent_module_deinit(struct wl_listener *listener, void *data);
};

#endif //IVI_ID_AGENT_BASE_CLASS