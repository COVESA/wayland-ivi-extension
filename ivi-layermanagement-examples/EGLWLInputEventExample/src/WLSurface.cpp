/**************************************************************************
 *
 * Copyright 2010, 2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "WLSurface.h"

WLSurface::WLSurface(WLContext* wlContext)
: m_wlContext(wlContext)
, m_wlSurface(NULL)
, m_iviSurface(NULL)
, m_width(0)
, m_height(0)
, m_surfaceId(0)
{
    assert(wlContext);
}

WLSurface::~WLSurface()
{
    if (m_surfaceId > 0)
        ivi_surface_destroy(m_iviSurface);

    if (m_wlSurface)
        wl_surface_destroy(m_wlSurface);
}

bool
WLSurface::CreateSurface(const int width, const int height, const int surfaceId)
{
    m_width  = width;
    m_height = height;
    m_surfaceId = surfaceId;

    m_wlSurface = (struct wl_surface*)
        wl_compositor_create_surface(m_wlContext->GetWLCompositor());
    if (!m_wlSurface){
        return false;
    }

    m_iviSurface = (struct ivi_surface*) ivi_application_surface_create(m_wlContext->GetIviApp(),
            m_surfaceId,
            m_wlSurface);
    if (!m_iviSurface){
        return false;
    }

    return CreatePlatformSurface();
}

bool
WLSurface::CreatePlatformSurface()
{
    return true;
}
