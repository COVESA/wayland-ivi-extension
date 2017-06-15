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
#include <ivi-application-client-protocol.h>
#include <wayland-cursor.h>


struct seat_data {
    struct wl_seat *wlSeat;
    struct wl_keyboard *wlKeyboard;
    struct wl_pointer *wlPointer;
    struct wl_touch *wlTouch;
    class WLContext *ctx;
};

class WLContext
{
// properties
private:
    struct wl_display*    m_wlDisplay;
    struct wl_registry*   m_wlRegistry;
    struct wl_compositor* m_wlCompositor;
    struct ivi_application*    m_iviApp;

    struct wl_pointer_listener*  m_wlPointerListener;
    struct wl_keyboard_listener* m_wlKeyboardListener;
    struct wl_touch_listener*    m_wlTouchListener;

    struct wl_cursor_theme* m_wlCursorTheme;
    struct wl_cursor* m_wlCursor;
    struct wl_shm* m_wlShm;
    struct wl_surface *m_pointerSurface;

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
    struct wl_pointer_listener* GetWLPointerListener() const;
    struct wl_keyboard_listener* GetWLKeyboardListener() const;
    struct wl_touch_listener* GetWLTouchListener() const;
    struct ivi_application* GetIviApp() const;
    struct wl_cursor_theme* GetWLCursorTheme() const;
    struct wl_cursor* GetWLCursor() const;
    struct wl_shm* GetWLShm() const;
    struct wl_surface* GetPointerSurface() const;

    void SetEventMask(uint32_t mask);
    void SetWLCompositor(struct wl_compositor* wlCompositor);
    void SetIviApp(struct ivi_application* iviApp);
    void SetWLSeat(struct wl_seat* wlSeat);
    void SetWLPointer(struct wl_pointer* wlPointer);
    void SetWLKeyboard(struct wl_keyboard* wlKeyboard);
    void SetWLTouch(struct wl_touch* wlTouch);
    void SetWLCursorTheme(struct wl_cursor_theme* wlCursorTheme);
    void SetWLCursor(struct wl_cursor* wlCursor);
    void SetWLShm(struct wl_shm* wlShm);
    void SetPointerSurface(struct wl_surface* pointerSurface);

    static void RegistryHandleGlobal(void* data,
                                     struct wl_registry* registry,
                                     uint32_t name,
                                     const char* interface,
                                     uint32_t version);
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
inline struct wl_pointer_listener* WLContext::GetWLPointerListener() const
    { return m_wlPointerListener; }
inline struct wl_keyboard_listener* WLContext::GetWLKeyboardListener() const
    { return m_wlKeyboardListener; }
inline struct wl_touch_listener* WLContext::GetWLTouchListener() const
    { return m_wlTouchListener; }
inline struct ivi_application* WLContext::GetIviApp() const { return m_iviApp; }
inline struct wl_cursor_theme* WLContext::GetWLCursorTheme() const { return m_wlCursorTheme; }
inline struct wl_cursor* WLContext::GetWLCursor() const { return m_wlCursor; }
inline struct wl_shm* WLContext::GetWLShm() const { return m_wlShm; }
inline struct wl_surface* WLContext::GetPointerSurface() const
    { return m_pointerSurface; }
inline void WLContext::SetWLCompositor(struct wl_compositor* wlCompositor)
    { m_wlCompositor = wlCompositor; }
inline void WLContext::SetIviApp(struct ivi_application* iviApp)
    { m_iviApp = iviApp; }
inline void WLContext::SetWLCursorTheme(struct wl_cursor_theme* wlCursorTheme)
    { m_wlCursorTheme = wlCursorTheme; }
inline void WLContext::SetWLCursor(struct wl_cursor* wlCursor)
    { m_wlCursor = wlCursor; }
inline void WLContext::SetWLShm(struct wl_shm* wlShm) { m_wlShm = wlShm; }
inline void WLContext::SetPointerSurface(struct wl_surface* pointerSurface)
    { m_pointerSurface = pointerSurface; }

#endif /* _WLCONTEXT_H_ */
