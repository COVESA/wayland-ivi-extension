/***************************************************************************
 *
 * Copyright 2012 BMW Car IT GmbH
 * Copyright (C) 2016 Advanced Driver Information Technology Joint Venture GmbH
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

#include "TestBase.h"
#include <cstring>
#include <stdexcept>

void registry_listener_callback(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    TestBase* base = static_cast<TestBase*>(data);

    if (0 == strcmp(interface, "wl_compositor"))
    {
        base->SetWLCompositor(reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 1)));
    }

    if (0 == strcmp(interface, "ivi_application"))
    {
        base->SetIviApp(reinterpret_cast<ivi_application*>(wl_registry_bind(registry, id, &ivi_application_interface, 1)));

    }
}

TestBase::TestBase()
: wlDisplay(NULL)
, wlRegistry(NULL)
{
    wlDisplay = wl_display_connect(NULL);
    if (!wlDisplay)
    {
        throw std::runtime_error("could not connect to wayland display");
    }
    wlRegistry = wl_display_get_registry(wlDisplay);

    static const struct wl_registry_listener registry_listener = {
        registry_listener_callback,
        NULL
    };

    wl_registry_add_listener(wlRegistry, &registry_listener, this);

    if (wl_display_roundtrip(wlDisplay) == -1 || wl_display_roundtrip(wlDisplay) == -1)
    {
        throw std::runtime_error("wl_display error");
    }

    wlSurfaces.reserve(10);
    for (int i = 0; i < (int)wlSurfaces.capacity(); ++i)
    {
        wlSurfaces.push_back(wl_compositor_create_surface(wlCompositor));
    }
}

TestBase::~TestBase()
{
    for (std::vector<wl_surface *>::reverse_iterator it = wlSurfaces.rbegin();
         it != wlSurfaces.rend();
         ++it)
    {
        wl_surface_destroy(*it);
    }
    wlSurfaces.clear();
    wl_compositor_destroy(wlCompositor);
    ivi_application_destroy(iviApp);
    wl_registry_destroy(wlRegistry);
    wl_display_disconnect(wlDisplay);
}

