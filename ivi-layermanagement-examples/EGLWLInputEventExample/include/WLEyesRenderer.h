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
#ifndef _WLEYESRENDERER_H_
#define _WLEYESRENDERER_H_

#include <wayland-client.h>
#include "WLEGLSurface.h"
#include "WLEyes.h"

bool InitRenderer();
bool InitShader();
bool InitVertexBuffer();
void WaitForEvent(struct wl_display* wlDisplay);
void TerminateRenderer();

// Pointer event handler
void PointerHandleEnter(void*, struct wl_pointer*, uint32_t, struct wl_surface*,
                        wl_fixed_t, wl_fixed_t);
void PointerHandleLeave(void*, struct wl_pointer*, uint32_t, struct wl_surface*);
void PointerHandleMotion(void*, struct wl_pointer*, uint32_t,
                         wl_fixed_t, wl_fixed_t);
void PointerHandleButton(void*, struct wl_pointer*, uint32_t, uint32_t, uint32_t,
                         uint32_t);
void PointerHandleAxis(void*, struct wl_pointer*, uint32_t, uint32_t, wl_fixed_t);

// Keyboard event handler
void KeyboardHandleKeymap(void*, struct wl_keyboard*, uint32_t, int, uint32_t);
void KeyboardHandleEnter(void*, struct wl_keyboard*, uint32_t, struct wl_surface*,
                         struct wl_array*);
void KeyboardHandleLeave(void*, struct wl_keyboard*, uint32_t, struct wl_surface*);
void KeyboardHandleKey(void*, struct wl_keyboard*, uint32_t, uint32_t, uint32_t,
                       uint32_t);
void KeyboardHandleModifiers(void*, struct wl_keyboard*, uint32_t, uint32_t,
                             uint32_t, uint32_t, uint32_t);

// Touch event handler
void TouchHandleDown(void*, struct wl_touch*, uint32_t, uint32_t, struct wl_surface*,
                     int32_t, wl_fixed_t, wl_fixed_t);
void TouchHandleUp(void*, struct wl_touch*, uint32_t, uint32_t, int32_t);
void TouchHandleMotion(void*, struct wl_touch*, uint32_t, int32_t, wl_fixed_t, wl_fixed_t);
void TouchHandleFrame(void*, struct wl_touch*);
void TouchHandleCancel(void*, struct wl_touch*);

bool DrawEyes(WLEGLSurface* surface, WLEyes* eyes);
void DrawFillPoly(const int nPoint, const float* points, const float color[4]);
void DrawPoly(const int nPoint, const float* points, const float color[4], int width);

extern const struct wl_pointer_listener PointerListener;
extern const struct wl_keyboard_listener KeyboardListener;
extern const struct wl_touch_listener TouchListener;

#endif /* _WLEYESRENDERER_H_ */
