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

#include "wayland-client.h"
#include <ivi-application-client-protocol.h>
#include <vector>

class TestBase
{
public:
    struct iviSurface
    {
        ivi_surface* surface;
        uint32_t surface_id;
    };
    TestBase();
    virtual ~TestBase();
    void SetWLCompositor(struct wl_compositor* wlCompositor);
    void SetIviApp(struct ivi_application* iviApp);
    void SetShm(struct wl_shm* wlShm);
    void SetShmFormats(uint32_t format);

protected:
    std::vector<wl_surface *> wlSurfaces;
    std::vector<iviSurface> iviSurfaces;
    std::vector<wl_buffer *> wlBuffers;
    wl_display*    wlDisplay;
    ivi_application* iviApp;
    wl_shm* wlShm;
    uint32_t shmFormats;

private:
    wl_registry*   wlRegistry;
    wl_compositor* wlCompositor;
};

inline void TestBase::SetWLCompositor(struct wl_compositor* wl_compositor)
    { wlCompositor = wl_compositor; }
inline void TestBase::SetIviApp(struct ivi_application* ivi_application)
    { iviApp = ivi_application; }
inline void TestBase::SetShm(struct wl_shm* wl_shm)
    { wlShm = wl_shm; }
inline void TestBase::SetShmFormats(uint32_t format)
    { shmFormats |= format; }
