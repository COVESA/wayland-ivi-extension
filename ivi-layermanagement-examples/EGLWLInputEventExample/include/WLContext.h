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
#ifndef _WLCONTEXT_H_
#define _WLCONTEXT_H_

#include <wayland-client.h>
#include <wayland-client-protocol.h>

struct serverinfo;

class WLContext
{
// properties
private:
    struct wl_display*    m_wlDisplay;
    struct wl_registry*   m_wlRegistry;
    struct wl_compositor* m_wlCompositor;
    struct wl_seat*       m_wlSeat;
    struct wl_pointer*    m_wlPointer;
    struct wl_keyboard*   m_wlKeyboard;
    struct wl_touch*      m_wlTouch;
    struct serverinfo*    m_wlServerInfo;
    uint32_t m_connectionId;

    struct wl_pointer_listener*  m_wlPointerListener;
    struct wl_keyboard_listener* m_wlKeyboardListener;
    struct wl_touch_listener*    m_wlTouchListener;

// methods
public:
    WLContext();
    virtual ~WLContext();

    bool InitWLContext(const struct wl_pointer_listener* wlPointerListener = NULL,
                       const struct wl_keyboard_listener* wlKeyboardListener = NULL,
                       const struct wl_touch_listener* wlTouchListener = NULL);

    struct wl_compositor* GetWLCompositor() const;
    struct wl_display* GetWLDisplay() const;
    struct wl_registry* GetWLRegistry() const;
    struct wl_pointer* GetWLPointer() const;
    struct wl_keyboard* GetWLKeyboard() const;
    struct wl_touch* GetWLTouch() const;
    struct wl_seat* GetWLSeat() const;
    struct wl_pointer_listener* GetWLPointerListener() const;
    struct wl_keyboard_listener* GetWLKeyboardListener() const;
    struct wl_touch_listener* GetWLTouchListener() const;
    uint32_t GetConnectionId() const;

    void SetEventMask(uint32_t mask);
    void SetWLCompositor(struct wl_compositor* wlCompositor);
    void SetWLServerInfo(struct serverinfo* wlServerInfo);
    void SetWLSeat(struct wl_seat* wlSeat);
    void SetConnectionId(uint32_t connectionId);
    void SetWLPointer(struct wl_pointer* wlPointer);
    void SetWLKeyboard(struct wl_keyboard* wlKeyboard);
    void SetWLTouch(struct wl_touch* wlTouch);

    static void RegistryHandleGlobal(void* data,
                                     struct wl_registry* registry,
                                     uint32_t name,
                                     const char* interface,
                                     uint32_t version);
    static void ServerInfoListener(void* data,
                                   struct serverinfo* serverInfo,
                                   uint32_t clientHandle);
    static void SeatHandleCapabilities(void* data,
                                       struct wl_seat* seat,
                                       uint32_t caps);
    static int EventMaskUpdate(uint32_t mask, void* data);

protected:
    void DestroyWLContext();
};

inline struct wl_compositor* WLContext::GetWLCompositor() const { return m_wlCompositor; }
inline struct wl_display* WLContext::GetWLDisplay() const { return m_wlDisplay; }
inline struct wl_registry* WLContext::GetWLRegistry() const { return m_wlRegistry; }
inline struct wl_pointer* WLContext::GetWLPointer() const { return m_wlPointer; }
inline struct wl_keyboard* WLContext::GetWLKeyboard() const { return m_wlKeyboard; }
inline struct wl_touch* WLContext::GetWLTouch() const { return m_wlTouch; }
inline struct wl_seat* WLContext::GetWLSeat() const { return m_wlSeat; }
inline struct wl_pointer_listener* WLContext::GetWLPointerListener() const
    { return m_wlPointerListener; }
inline struct wl_keyboard_listener* WLContext::GetWLKeyboardListener() const
    { return m_wlKeyboardListener; }
inline struct wl_touch_listener* WLContext::GetWLTouchListener() const
    { return m_wlTouchListener; }
inline uint32_t WLContext::GetConnectionId() const { return m_connectionId; }

inline void WLContext::SetWLCompositor(struct wl_compositor* wlCompositor)
    { m_wlCompositor = wlCompositor; }
inline void WLContext::SetWLServerInfo(struct serverinfo* wlServerInfo)
    { m_wlServerInfo = wlServerInfo; }
inline void WLContext::SetWLSeat(struct wl_seat* wlSeat)
    { m_wlSeat = wlSeat; }
inline void WLContext::SetConnectionId(uint32_t connectionId)
    { m_connectionId = connectionId; }
inline void WLContext::SetWLPointer(struct wl_pointer* wlPointer)
    { m_wlPointer = wlPointer; }
inline void WLContext::SetWLKeyboard(struct wl_keyboard* wlKeyboard)
    { m_wlKeyboard = wlKeyboard; }
inline void WLContext::SetWLTouch(struct wl_touch* wlTouch)
    { m_wlTouch = wlTouch; }

#endif /* _WLCONTEXT_H_ */
