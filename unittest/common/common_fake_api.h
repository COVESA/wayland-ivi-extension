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

#ifndef COMMON_FAKE_API
#define COMMON_FAKE_API

#include "stdint.h"
#include "wayland-client-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void custom_wl_list_init(struct wl_list *list);
void custom_wl_list_insert(struct wl_list *list, struct wl_list *elm);
void custom_wl_list_remove(struct wl_list *elm);
int  custom_wl_list_empty(const struct wl_list *list);
void custom_wl_array_init(struct wl_array *array);
void custom_wl_array_release(struct wl_array *array);
void *custom_wl_array_add(struct wl_array *array, size_t size);

#ifdef __cplusplus
}
#endif
#endif // COMMON_FAKE_API