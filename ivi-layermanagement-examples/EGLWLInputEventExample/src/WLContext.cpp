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
#include "WLContext.h"
#include "WaylandServerinfoClientProtocol.h"

#define WL_UNUSED(A) (A)=(A)

//////////////////////////////////////////////////////////////////////////////

static struct wl_registry_listener registryListener = {
    WLContext::RegistryHandleGlobal,
    NULL
};

static struct serverinfo_listener serverInfoListenerList = {
    WLContext::ServerInfoListener,
};

static struct wl_seat_listener seatListener = {
    WLContext::SeatHandleCapabilities,
    NULL
};

//////////////////////////////////////////////////////////////////////////////

WLContext::WLContext()
: m_wlDisplay(NULL)
, m_wlRegistry(NULL)
, m_wlCompositor(NULL)
, m_wlSeat(NULL)
, m_wlPointer(NULL)
, m_wlTouch(NULL)
, m_wlServerInfo(NULL)
, m_connectionId(0)
, m_wlPointerListener(NULL)
, m_wlKeyboardListener(NULL)
, m_wlTouchListener(NULL)
{
}

WLContext::~WLContext()
{
    DestroyWLContext();
}

//////////////////////////////////////////////////////////////////////////////

void
WLContext::RegistryHandleGlobal(void* data,
                                struct wl_registry* registry,
                                uint32_t name,
                                const char* interface,
                                uint32_t version)
{
    WL_UNUSED(version);

    WLContext* surface = static_cast<WLContext*>(data);
    assert(surface);

    do {
        if (!strcmp(interface, "wl_compositor")){
            surface->SetWLCompositor(
                (wl_compositor*)wl_registry_bind(registry,
                                                name,
                                                &wl_compositor_interface,
                                                1));
            break;
        }

        if (!strcmp(interface, "serverinfo")){
            struct serverinfo* wlServerInfo = (struct serverinfo*)wl_registry_bind(
                registry, name, &serverinfo_interface, 1);
            serverinfo_add_listener(wlServerInfo, &serverInfoListenerList, data);
            serverinfo_get_connection_id(wlServerInfo);
            surface->SetWLServerInfo(wlServerInfo);
            break;
        }

        if (!strcmp(interface, "wl_seat")){
            struct wl_seat* wlSeat = (wl_seat*)wl_registry_bind(
                registry, name, &wl_seat_interface, 1);
            wl_seat_add_listener(wlSeat, &seatListener, data);
            surface->SetWLSeat(wlSeat);
        }
    } while (0);
}

void
WLContext::ServerInfoListener(void* data,
                              struct serverinfo* serverInfo,
                              uint32_t clientHandle)
{
    WL_UNUSED(serverInfo);

    WLContext* surface = static_cast<WLContext*>(data);
    assert(surface);

    surface->SetConnectionId(clientHandle);
}

void
WLContext::SeatHandleCapabilities(void* data, struct wl_seat* seat, uint32_t caps)
{
    WL_UNUSED(seat);

    WLContext* context = static_cast<WLContext*>(data);
    assert(context);

    struct wl_seat* wlSeat = context->GetWLSeat();
    if (!wlSeat)
        return;

    struct wl_pointer* wlPointer = context->GetWLPointer();
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wlPointer){
        wlPointer = wl_seat_get_pointer(wlSeat);
        wl_pointer_set_user_data(wlPointer, data);
        wl_pointer_add_listener(wlPointer, context->GetWLPointerListener(), data);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlPointer){
        wl_pointer_destroy(wlPointer);
        wlPointer = NULL;
    }
    context->SetWLPointer(wlPointer);

    struct wl_keyboard* wlKeyboard = context->GetWLKeyboard();
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !wlKeyboard){
        wlKeyboard = wl_seat_get_keyboard(wlSeat);
        wl_keyboard_set_user_data(wlKeyboard, data);
        wl_keyboard_add_listener(wlKeyboard, context->GetWLKeyboardListener(), data);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wlKeyboard){
        wl_keyboard_destroy(wlKeyboard);
        wlKeyboard = NULL;
    }
    context->SetWLKeyboard(wlKeyboard);

    struct wl_touch* wlTouch = context->GetWLTouch();
    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !wlTouch){
        wlTouch = wl_seat_get_touch(wlSeat);
        wl_touch_set_user_data(wlTouch, data);
        wl_touch_add_listener(wlTouch, context->GetWLTouchListener(), data);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && wlTouch){
        wl_touch_destroy(wlTouch);
        wlTouch = NULL;
    }
    context->SetWLTouch(wlTouch);
}

//////////////////////////////////////////////////////////////////////////////

bool
WLContext::InitWLContext(const struct wl_pointer_listener* wlPointerListener,
                         const struct wl_keyboard_listener* wlKeyboardListener,
                         const struct wl_touch_listener* wlTouchListener)
{
    m_wlPointerListener = const_cast<wl_pointer_listener*>(wlPointerListener);
    m_wlKeyboardListener = const_cast<wl_keyboard_listener*>(wlKeyboardListener);
    m_wlTouchListener = const_cast<wl_touch_listener*>(wlTouchListener);

    m_wlDisplay = wl_display_connect(NULL);

    m_wlRegistry = wl_display_get_registry(m_wlDisplay);
    wl_registry_add_listener(m_wlRegistry, &registryListener, this);
    wl_display_dispatch(m_wlDisplay);
    wl_display_roundtrip(m_wlDisplay);

    return true;
}

void
WLContext::DestroyWLContext()
{
    if (m_wlCompositor)
        wl_compositor_destroy(m_wlCompositor);
}
