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
#include "WLEGLSurface.h"

//////////////////////////////////////////////////////////////////////////////
#define CHECK_EGL_ERROR(FUNC, STAT) \
    STAT = eglGetError();           \
    if (STAT != EGL_SUCCESS){       \
        fprintf(stderr, "[ERROR] %s failed: %d\n", FUNC, STAT); \
        return false;               \
    }

//////////////////////////////////////////////////////////////////////////////

WLEGLSurface::WLEGLSurface(WLContext* wlContext)
: WLSurface(wlContext)
, m_wlEglWindow(NULL)
, m_eglDisplay(EGL_NO_DISPLAY)
, m_eglConfig(0)
, m_eglSurface(EGL_NO_SURFACE)
, m_eglContext(EGL_NO_CONTEXT)
{
}

WLEGLSurface::~WLEGLSurface()
{
    if (m_wlEglWindow)
        wl_egl_window_destroy(m_wlEglWindow);
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(m_eglDisplay);
}

bool
WLEGLSurface::CreatePlatformSurface()
{
    EGLint eglStat = EGL_SUCCESS;
    EGLint major, minor;
    int nConfig;
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        //EGL_SAMPLE_BUFFERS, 1,
        //EGL_SAMPLES,        2,
        EGL_NONE,
    };
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    m_wlEglWindow = wl_egl_window_create(m_wlSurface, m_width, m_height);

    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_wlContext->GetWLDisplay());
    CHECK_EGL_ERROR("eglGetDisplay", eglStat);

    if (!eglInitialize(m_eglDisplay, &major, &minor)){
        CHECK_EGL_ERROR("eglInitialize", eglStat);
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    CHECK_EGL_ERROR("eglBindAPI", eglStat);

    if (!eglChooseConfig(m_eglDisplay, configAttribs, &m_eglConfig, 1, &nConfig)
        || (nConfig != 1)){
        CHECK_EGL_ERROR("eglChooseConfig", eglStat);
    }

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig,
                                          (EGLNativeWindowType)m_wlEglWindow, NULL);
    CHECK_EGL_ERROR("eglCreateWindowSurface", eglStat);

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, NULL, contextAttribs);
    CHECK_EGL_ERROR("eglCreateContext", eglStat);

    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    CHECK_EGL_ERROR("eglMakeCurrent", eglStat);

    //eglSwapInterval(m_eglDisplay, 1);
    //CHECK_EGL_ERROR("eglSwapInterval", eglStat);

    return true;
}
