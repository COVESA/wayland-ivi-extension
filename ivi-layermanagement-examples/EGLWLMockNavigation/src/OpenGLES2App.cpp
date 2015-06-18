/***************************************************************************
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
#include "OpenGLES2App.h"
#include "LayerScene.h"
#include <ilm_common.h>
#include <ilm_client.h>

#include <iostream>
using std::cout;
using std::endl;

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <stdint.h>
#include <signal.h>
#include <sys/stat.h>
#include <linux/fb.h>


extern "C"
{
    ///////////////////////////////////////////////////////////////////////////////
    static void
    handlePing(void *data, struct wl_shell_surface *shellSurface, uint32_t serial)
    {
        (void)data;
        wl_shell_surface_pong(shellSurface, serial);
    }

    static void
    handleConfigure(void *data, struct wl_shell_surface *shellSurface,
                    uint32_t edges, int32_t width, int32_t height)
    {
        (void)data;
        (void)shellSurface;
        (void)edges;
        (void)width;
        (void)height;
    }

    static void
    handlePopupDone(void *data, struct wl_shell_surface *shellSurface)
    {
        (void)data;
        (void)shellSurface;
    }

    static const struct wl_shell_surface_listener shellSurfaceListener = {
        handlePing,
        handleConfigure,
        handlePopupDone,
    };

    ///////////////////////////////////////////////////////////////////////////////

    void OpenGLES2App::registry_handle_global(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
    {
        WLContextStruct* p_wlCtx = (WLContextStruct*)data;
        int ans_strcmp = 0;
        (void)version;

        do
        {
            ans_strcmp = strcmp(interface, "wl_compositor");
            if (0 == ans_strcmp)
            {
                p_wlCtx->wlCompositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
                break;
            }

            ans_strcmp = strcmp(interface, "wl_shell");
            if (0 == ans_strcmp)
            {
                p_wlCtx->wlShell = (struct wl_shell*)wl_registry_bind(registry, id, &wl_shell_interface, 1);
                break;
            }

        } while(0);
    }

    static const struct wl_registry_listener registry_listener = {
        OpenGLES2App::registry_handle_global,
        NULL
    };
}

#define RUNTIME_IN_MS() (GetTickCount() - startTimeInMS)


OpenGLES2App::OpenGLES2App(float fps, float animationSpeed, SurfaceConfiguration* config)
: m_framesPerSecond(fps)
, m_animationSpeed(animationSpeed)
, m_timerIntervalInMs(1000.0 / m_framesPerSecond)
, m_surfaceId(0)
{
    createWLContext(config);
    createEGLContext();

    ilmClient_init((t_ilm_nativedisplay)m_wlContextStruct.wlDisplay);
    setupLayerMangement(config);

    if (config->nosky)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        glClearColor(0.2f, 0.2f, 0.5f, 1.0f);
    }
    glDisable(GL_BLEND);

    glClearDepthf(1.0f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

OpenGLES2App::~OpenGLES2App()
{
    destroyEglContext();
    destroyWLContext();

    ilm_surfaceRemove(m_surfaceId);
    ilmClient_destroy();
}

void OpenGLES2App::mainloop()
{
    unsigned int startTimeInMS = GetTickCount();
    unsigned int frameStartTimeInMS = 0;
    unsigned int renderTimeInMS = 0;
    unsigned int frameEndTimeInMS = 0;
    unsigned int frameTimeInMS = 0;

    while (true)
    {
        frameTimeInMS = frameEndTimeInMS - frameStartTimeInMS;
        frameStartTimeInMS = RUNTIME_IN_MS();

        update(m_animationSpeed * frameStartTimeInMS, m_animationSpeed * frameTimeInMS);
        render();
        swapBuffers();

        renderTimeInMS = RUNTIME_IN_MS() - frameStartTimeInMS;

        if (renderTimeInMS < m_timerIntervalInMs)
        {
            usleep((m_timerIntervalInMs - renderTimeInMS) * 1000);
        }

        frameEndTimeInMS = RUNTIME_IN_MS();
    }
}

bool OpenGLES2App::createWLContext(SurfaceConfiguration* config)
{
    t_ilm_bool result = ILM_TRUE;
    int width = config->surfaceWidth;
    int height = config->surfaceHeight;

    memset(&m_wlContextStruct, 0, sizeof(m_wlContextStruct));

    m_wlContextStruct.width = width;
    m_wlContextStruct.height = height;
    m_wlContextStruct.wlDisplay = wl_display_connect(NULL);
    if (NULL == m_wlContextStruct.wlDisplay)
    {
        cout<<"Error: wl_display_connect() failed.\n";
    }

    m_wlContextStruct.wlRegistry = wl_display_get_registry(m_wlContextStruct.wlDisplay);
    wl_registry_add_listener(m_wlContextStruct.wlRegistry, &registry_listener, &m_wlContextStruct);
    wl_display_dispatch(m_wlContextStruct.wlDisplay);
    wl_display_roundtrip(m_wlContextStruct.wlDisplay);

    m_wlContextStruct.wlSurface = wl_compositor_create_surface(m_wlContextStruct.wlCompositor);
    if (NULL == m_wlContextStruct.wlSurface)
    {
        cout<<"Error: wl_compositor_create_surface() failed.\n";
        destroyWLContext();
    }

    if (m_wlContextStruct.wlShell) {
        m_wlContextStruct.wlShellSurface = wl_shell_get_shell_surface(m_wlContextStruct.wlShell,
                                                                      m_wlContextStruct.wlSurface);
    }

    if (m_wlContextStruct.wlShellSurface) {
        wl_shell_surface_add_listener(
            reinterpret_cast<struct wl_shell_surface*>(m_wlContextStruct.wlShellSurface),
            &shellSurfaceListener, &m_wlContextStruct);
    }

    m_wlContextStruct.wlNativeWindow = wl_egl_window_create(m_wlContextStruct.wlSurface, width, height);
    if (NULL == m_wlContextStruct.wlNativeWindow)
    {
        cout<<"Error: wl_egl_window_create() failed"<<endl;
        destroyWLContext();
    }

    if (m_wlContextStruct.wlShellSurface) {
        wl_shell_surface_set_title(m_wlContextStruct.wlShellSurface, "mocknavi");
        wl_shell_surface_set_toplevel(m_wlContextStruct.wlShellSurface);
    }

    return result;
}

bool OpenGLES2App::createEGLContext()
{
    t_ilm_bool result = ILM_TRUE;
    EGLint eglstatus = EGL_SUCCESS;
    m_eglContextStruct.eglDisplay = NULL;
    m_eglContextStruct.eglSurface = NULL;
    m_eglContextStruct.eglContext = NULL;

    m_eglContextStruct.eglDisplay = eglGetDisplay(m_wlContextStruct.wlDisplay);
    eglstatus = eglGetError();
    if (!m_eglContextStruct.eglDisplay)
    {
	cout << "Error: eglGetDisplay() failed.\n";
    }

    EGLint iMajorVersion, iMinorVersion;
    if (!eglInitialize(m_eglContextStruct.eglDisplay, &iMajorVersion,
            &iMinorVersion))
    {
	cout << "Error: eglInitialize() failed.\n";
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    eglstatus = eglGetError();
    if (eglstatus != EGL_SUCCESS)
    {
	cout << "Error: eglBindAPI() failed.\n";
    }
    EGLint pi32ConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE };

    int iConfigs;

    if (!eglChooseConfig(m_eglContextStruct.eglDisplay, pi32ConfigAttribs, &m_eglContextStruct.eglConfig, 1, &iConfigs) || (iConfigs != 1))
    {
	cout << "Error: eglChooseConfig() failed.\n";
    }

    m_eglContextStruct.eglSurface = eglCreateWindowSurface(
            m_eglContextStruct.eglDisplay, m_eglContextStruct.eglConfig,
            m_wlContextStruct.wlNativeWindow, NULL);
    eglstatus = eglGetError();

    if (eglstatus != EGL_SUCCESS)
    {
	cout << "Error: eglCreateWindowSurface() failed.\n";
    }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    m_eglContextStruct.eglContext = eglCreateContext(
            m_eglContextStruct.eglDisplay, m_eglContextStruct.eglConfig, NULL,
            contextAttribs);

    eglstatus = eglGetError();
    if (eglstatus != EGL_SUCCESS)
    {
	cout << "Error: eglCreateContext() failed.\n";
    }

    eglMakeCurrent(m_eglContextStruct.eglDisplay,
            m_eglContextStruct.eglSurface, m_eglContextStruct.eglSurface,
            m_eglContextStruct.eglContext);
    eglSwapInterval(m_eglContextStruct.eglDisplay, 1);
    eglstatus = eglGetError();
    if (eglstatus != EGL_SUCCESS)
    {
	cout << "Error: eglMakeCurrent() failed.\n";
    }

    return result;
}

ilmErrorTypes OpenGLES2App::setupLayerMangement(SurfaceConfiguration* config)
{
    ilmErrorTypes error = ILM_FAILED;

    // register surfaces to layermanager
    t_ilm_surface surfaceid = (t_ilm_surface)config->surfaceId;//SURFACE_EXAMPLE_EGLX11_APPLICATION;
    int width = config->surfaceWidth;
    int height = config->surfaceHeight;

    if (config->nosky)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        glClearColor(0.2f, 0.2f, 0.5f, 1.0f);
    }

    ilm_surfaceCreate((t_ilm_nativehandle)m_wlContextStruct.wlSurface, width, height,
            ILM_PIXELFORMAT_RGBA_8888, &surfaceid);

    m_surfaceId = surfaceid;

    return error;
}

void OpenGLES2App::destroyEglContext()
{
    if (m_eglContextStruct.eglDisplay != NULL)
    {
        eglMakeCurrent(m_eglContextStruct.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(m_eglContextStruct.eglDisplay);
    }
}

void OpenGLES2App::destroyWLContext()
{
    if (m_wlContextStruct.wlNativeWindow)
    {
        wl_egl_window_destroy(m_wlContextStruct.wlNativeWindow);
    }
    if (m_wlContextStruct.wlShellSurface)
    {
        wl_shell_surface_destroy(reinterpret_cast<struct wl_shell_surface*>(m_wlContextStruct.wlShellSurface));
    }
    if (m_wlContextStruct.wlSurface)
    {
        wl_surface_destroy(m_wlContextStruct.wlSurface);
    }
    if (m_wlContextStruct.wlShell)
    {
        wl_shell_destroy(m_wlContextStruct.wlShell);
    }
    if (m_wlContextStruct.wlCompositor)
    {
        wl_compositor_destroy(m_wlContextStruct.wlCompositor);
    }
}

unsigned int OpenGLES2App::GetTickCount()
{
    struct timeval ts;
    gettimeofday(&ts, 0);
    return (t_ilm_uint) (ts.tv_sec * 1000 + (ts.tv_usec / 1000));
}

extern "C" void
OpenGLES2App::frame_listener_func(void *data, struct wl_callback *callback, uint32_t time)
{
    data = data; // TODO:to avoid warning
    time = time; // TODO:to avoid warning
    if (callback)
    {
        wl_callback_destroy(callback);
    }
}

static const struct wl_callback_listener frame_listener = {
    OpenGLES2App::frame_listener_func
};

void OpenGLES2App::swapBuffers()
{
    struct wl_callback* callback = wl_surface_frame(m_wlContextStruct.wlSurface);
    wl_callback_add_listener(callback, &frame_listener, NULL);

    eglSwapBuffers(m_eglContextStruct.eglDisplay, m_eglContextStruct.eglSurface);
}
