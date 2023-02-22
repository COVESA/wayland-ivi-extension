/***************************************************************************
 *
 * Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
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

#include "client_api_fake.h"
#include "ilm_control_platform.h"
#include "wayland-client-protocol.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(struct wl_display *, wl_display_connect, const char *);
DEFINE_FAKE_VALUE_FUNC(struct wl_event_queue *, wl_display_create_queue, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_roundtrip, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(int, wl_proxy_add_listener, struct wl_proxy *, registerHandler, void *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_roundtrip_queue, struct wl_display *, struct wl_event_queue *);
DEFINE_FAKE_VALUE_FUNC(int, pthread_create, pthread_t *, const pthread_attr_t *, threadEntry, void *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_flush, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_dispatch_queue, struct wl_display *, struct wl_event_queue *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_dispatch_queue_pending, struct wl_display *, struct wl_event_queue *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_get_error, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_get_fd, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_prepare_read_queue, struct wl_display *, struct wl_event_queue *);
DEFINE_FAKE_VALUE_FUNC(int, wl_display_read_events, struct wl_display *);
DEFINE_FAKE_VOID_FUNC(wl_display_cancel_read, struct wl_display *);
DEFINE_FAKE_VALUE_FUNC(uint32_t, wl_proxy_get_version, struct wl_proxy *);
DEFINE_FAKE_VOID_FUNC(wl_display_disconnect, struct wl_display *);
DEFINE_FAKE_VOID_FUNC(wl_event_queue_destroy, struct wl_event_queue *);
DEFINE_FAKE_VOID_FUNC(wl_proxy_set_queue, struct wl_proxy *, struct wl_event_queue *);
DEFINE_FAKE_VOID_FUNC(wl_proxy_destroy, struct wl_proxy *);
DEFINE_FAKE_VALUE_FUNC(void *, wl_array_add, struct wl_array *, size_t);
DEFINE_FAKE_VOID_FUNC(wl_list_insert, struct wl_list *, struct wl_list *);
DEFINE_FAKE_VALUE_FUNC_VARARG(struct wl_proxy *, wl_proxy_marshal_flags, struct wl_proxy *, uint32_t, const struct wl_interface *, uint32_t, uint32_t, ...);
DEFINE_FAKE_VOID_FUNC(wl_list_remove, struct wl_list *);
DEFINE_FAKE_VOID_FUNC(wl_list_init, struct wl_list *);
DEFINE_FAKE_VOID_FUNC(wl_array_init, struct wl_array *);
DEFINE_FAKE_VOID_FUNC(wl_array_release, struct wl_array *);

//DEFINE_FAKE_VALUE_FUNC(int, wl_list_length, const struct wl_list *);
