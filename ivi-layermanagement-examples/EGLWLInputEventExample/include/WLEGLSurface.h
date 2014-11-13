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
#ifndef _WLEGLSURFACE_H_
#define _WLEGLSURFACE_H_

#include <EGL/egl.h>
#include <wayland-egl.h>
#include "WLSurface.h"

class WLEGLSurface : public WLSurface
{
// properties
protected:
    struct wl_egl_window* m_wlEglWindow;
    EGLDisplay            m_eglDisplay;
    EGLConfig             m_eglConfig;
    EGLSurface            m_eglSurface;
    EGLContext            m_eglContext;

// methods
public:
    WLEGLSurface(WLContext* wlContext);
    virtual ~WLEGLSurface();

    EGLDisplay GetEGLDisplay() const;
    EGLSurface GetEGLSurface() const;
    EGLContext GetEGLContext() const;

protected:
    virtual bool CreatePlatformSurface();
};

inline EGLDisplay WLEGLSurface::GetEGLDisplay() const { return m_eglDisplay; }
inline EGLSurface WLEGLSurface::GetEGLSurface() const { return m_eglSurface; }
inline EGLContext WLEGLSurface::GetEGLContext() const { return m_eglContext; }

#endif /* _WLEGLSURFACE_H_ */
