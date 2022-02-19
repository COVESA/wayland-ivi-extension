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

#define WL_UNUSED(A) (A)=(A)

//////////////////////////////////////////////////////////////////////////////

static struct wl_registry_listener registryListener = {
    WLContext::RegistryHandleGlobal,
    NULL
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
, m_iviApp(NULL)
, m_wlPointerListener(NULL)
, m_wlKeyboardListener(NULL)
, m_wlTouchListener(NULL)
, m_wlCursorTheme(NULL)
, m_wlCursor(NULL)
, m_wlShm(NULL)
{
}

WLContext::~WLContext()
{
    DestroyWLContext();
}

//////////////////////////////////////////////////////////////////////////////

/*
 * The following correspondences between file names and cursors was copied
 * from: https://bugs.kde.org/attachment.cgi?id=67313
 */

static const char *left_ptrs[] = {
	"left_ptr",
	"default",
	"top_left_arrow",
	"left-arrow"
};

static void
create_cursors(WLContext* wlContext)
{
	int size = 32;
	unsigned int j;
	struct wl_cursor *cursor = NULL;

	wlContext->SetWLCursorTheme(wl_cursor_theme_load(NULL, size, wlContext->GetWLShm()));
	if (!wlContext->GetWLCursorTheme()) {
		fprintf(stderr, "could not load default theme\n");
		return;
	}
	wlContext->SetWLCursor((wl_cursor*) malloc(sizeof(wl_cursor)));

	for (j = 0; !cursor && j < 4; ++j)
		cursor = wl_cursor_theme_get_cursor(wlContext->GetWLCursorTheme(),
				left_ptrs[j]);

	if (!cursor)
		fprintf(stderr, "could not load cursor '%s'\n", left_ptrs[j]);

	wlContext->SetWLCursor(cursor);
}

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

        if (!strcmp(interface, "ivi_application")){
            surface->SetIviApp(
                (struct ivi_application*)wl_registry_bind(registry,
                                                          name,
                                                          &ivi_application_interface,
                                                          1));
        }

        if (!strcmp(interface, "wl_shm")){
            surface->SetWLShm(
                (struct wl_shm*)wl_registry_bind(registry,
                                                          name,
                                                          &wl_shm_interface,
                                                          1));
        }

        if (!strcmp(interface, "wl_seat")){
            struct seat_data *seat_data = (struct seat_data *)calloc(1, sizeof *seat_data);
            seat_data->ctx = surface;
            seat_data->wlSeat = (wl_seat*)wl_registry_bind(
                                 registry, name, &wl_seat_interface, 1);
            wl_seat_add_listener(seat_data->wlSeat, &seatListener,
                                 (void *)seat_data);
        }
    } while (0);
}

void
WLContext::SeatHandleCapabilities(void* data, struct wl_seat* seat, uint32_t caps)
{
	struct seat_data* context =
			static_cast<struct seat_data*>(data);
    assert(context);

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !context->wlPointer){
        context->wlPointer = wl_seat_get_pointer(seat);
        wl_pointer_set_user_data(context->wlPointer, data);
        wl_pointer_add_listener(context->wlPointer,
                                context->ctx->GetWLPointerListener(), data);
        // create cursors
        create_cursors(context->ctx);
        context->ctx->SetPointerSurface(wl_compositor_create_surface(context->ctx->GetWLCompositor()));
    } else
    if (!(caps & WL_SEAT_CAPABILITY_POINTER) && context->wlPointer){
        wl_pointer_destroy(context->wlPointer);
        context->wlPointer = NULL;

        if (context->ctx->GetPointerSurface()){
            wl_surface_destroy(context->ctx->GetPointerSurface());
            context->ctx->SetPointerSurface(NULL);
        }

        if (context->ctx->GetWLCursorTheme())
            wl_cursor_theme_destroy(context->ctx->GetWLCursorTheme());

        if (context->ctx->GetWLCursor())
            free(context->ctx->GetWLCursor());
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !context->wlKeyboard){
        context->wlKeyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_set_user_data(context->wlKeyboard, data);
        wl_keyboard_add_listener(context->wlKeyboard,
                                 context->ctx->GetWLKeyboardListener(), data);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && context->wlKeyboard){
        wl_keyboard_destroy(context->wlKeyboard);
        context->wlKeyboard = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !context->wlTouch){
        context->wlTouch = wl_seat_get_touch(seat);
        wl_touch_set_user_data(context->wlTouch, data);
        wl_touch_add_listener(context->wlTouch, context->ctx->GetWLTouchListener(), data);
    } else
    if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && context->wlTouch){
        wl_touch_destroy(context->wlTouch);
        context->wlTouch = NULL;
    }
    wl_display_dispatch(context->ctx->GetWLDisplay());
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
    if (m_iviApp)
        ivi_application_destroy(m_iviApp);

    if (m_wlCompositor)
        wl_compositor_destroy(m_wlCompositor);

    if (m_pointerSurface){
        wl_surface_destroy(m_pointerSurface);
        m_pointerSurface = NULL;
    }

    if (m_wlCursorTheme)
        wl_cursor_theme_destroy(m_wlCursorTheme);

    if (m_wlCursor)
        free(m_wlCursor);

    wl_registry_destroy(m_wlRegistry);
    wl_display_flush(m_wlDisplay);
    wl_display_disconnect(m_wlDisplay);
}
