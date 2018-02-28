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
int WaitForEvent(struct wl_display* wlDisplay, int fd);
void TerminateRenderer();

bool DrawEyes(WLEGLSurface* surface, WLEyes* eyes);
void DrawFillPoly(const int nPoint, const float* points, const float color[4]);
void DrawPoly(const int nPoint, const float* points, const float color[4], int width);

extern const struct wl_pointer_listener PointerListener;
extern const struct wl_keyboard_listener KeyboardListener;
extern const struct wl_touch_listener TouchListener;

#endif /* _WLEYESRENDERER_H_ */
