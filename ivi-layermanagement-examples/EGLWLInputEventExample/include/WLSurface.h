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
#ifndef _WLSURFACE_H_
#define _WLSURFACE_H_

#include "ilm_client.h"
#include "WLContext.h"

class WLSurface
{
// properties
protected:
    WLContext*         m_wlContext;
    struct wl_surface* m_wlSurface;
    int           m_width;
    int           m_height;
    t_ilm_layer   m_ilmLayerId;
    t_ilm_surface m_ilmSurfaceId;

// methods
public:
    WLSurface(WLContext* wlContext);
    virtual ~WLSurface();

    struct wl_display* GetWLDisplay() const;
    struct wl_surface* GetWLSurface() const;

    virtual bool CreateSurface(const int width, const int height);
    virtual bool CreateIlmSurface(t_ilm_surface* surfaceId,
                                  t_ilm_int width,
                                  t_ilm_int height);
    virtual void DestroyIlmSurface();

protected:
    virtual bool CreatePlatformSurface();
};

inline struct wl_surface* WLSurface::GetWLSurface() const { return m_wlSurface; }
inline struct wl_display* WLSurface::GetWLDisplay() const
    { return m_wlContext->GetWLDisplay(); }

#endif /* _WLSURFACE_H_ */
