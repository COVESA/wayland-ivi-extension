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

#ifndef CLIENT_API_FAKE
#define CLIENT_API_FAKE

#include "stdint.h"
#include <pthread.h>
#include "fff.h"
#include "common_fake_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (**registerHandler) (void);
typedef void *(*threadEntry) (void *);

DECLARE_FAKE_VALUE_FUNC(struct wl_display *, wl_display_connect, const char *);
DECLARE_FAKE_VALUE_FUNC(struct wl_event_queue *, wl_display_create_queue, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_roundtrip, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(int, wl_proxy_add_listener, struct wl_proxy *, registerHandler, void *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_roundtrip_queue, struct wl_display *, struct wl_event_queue *);
DECLARE_FAKE_VALUE_FUNC(int, pthread_create, pthread_t *, const pthread_attr_t *, threadEntry, void *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_flush, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_dispatch_queue, struct wl_display *, struct wl_event_queue *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_dispatch_queue_pending, struct wl_display *, struct wl_event_queue *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_get_error, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_get_fd, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_prepare_read_queue, struct wl_display *, struct wl_event_queue *);
DECLARE_FAKE_VALUE_FUNC(int, wl_display_read_events, struct wl_display *);
DECLARE_FAKE_VALUE_FUNC(uint32_t, wl_proxy_get_version, struct wl_proxy *);
DECLARE_FAKE_VOID_FUNC(wl_display_disconnect, struct wl_display *);
DECLARE_FAKE_VOID_FUNC(wl_event_queue_destroy, struct wl_event_queue *);
DECLARE_FAKE_VOID_FUNC(wl_proxy_set_queue, struct wl_proxy *, struct wl_event_queue *);
DECLARE_FAKE_VOID_FUNC(wl_proxy_destroy, struct wl_proxy *);
DECLARE_FAKE_VOID_FUNC(wl_display_cancel_read, struct wl_display *);
DECLARE_FAKE_VOID_FUNC(wl_list_insert, struct wl_list *, struct wl_list *);
DECLARE_FAKE_VALUE_FUNC(void *, wl_array_add, struct wl_array *, size_t);
DECLARE_FAKE_VALUE_FUNC_VARARG(struct wl_proxy *, wl_proxy_marshal_flags, struct wl_proxy *, uint32_t, const struct wl_interface *, uint32_t, uint32_t, ...);
DECLARE_FAKE_VOID_FUNC(wl_array_init, struct wl_array *);
DECLARE_FAKE_VOID_FUNC(wl_array_release, struct wl_array *);
DECLARE_FAKE_VOID_FUNC(wl_list_init, struct wl_list *);
DECLARE_FAKE_VOID_FUNC(wl_list_remove, struct wl_list *);
// DECLARE_FAKE_VALUE_FUNC(int, wl_list_length, const struct wl_list *);

#define CLIENT_API_FAKE_LIST(FAKE) \
    FAKE(wl_display_connect) \
    FAKE(wl_display_create_queue) \
    FAKE(wl_display_roundtrip) \
    FAKE(wl_proxy_add_listener) \
    FAKE(wl_display_roundtrip_queue) \
    FAKE(pthread_create) \
    FAKE(wl_display_flush) \
    FAKE(wl_display_dispatch_queue) \
    FAKE(wl_display_dispatch_queue_pending) \
    FAKE(wl_display_get_error) \
    FAKE(wl_display_get_fd) \
    FAKE(wl_display_prepare_read_queue) \
    FAKE(wl_display_read_events) \
    FAKE(wl_proxy_get_version) \
    FAKE(wl_display_disconnect) \
    FAKE(wl_event_queue_destroy) \
    FAKE(wl_proxy_set_queue) \
    FAKE(wl_proxy_destroy) \
    FAKE(wl_display_cancel_read) \
    FAKE(wl_list_insert) \
    FAKE(wl_array_add) \
    FAKE(wl_proxy_marshal_flags) \
    FAKE(wl_array_init) \
    FAKE(wl_array_release) \
    FAKE(wl_list_init) \
    FAKE(wl_list_remove) \
    FFF_RESET_HISTORY()

#ifdef __cplusplus
}
#endif
#endif  // CLIENT_API_FAKE
