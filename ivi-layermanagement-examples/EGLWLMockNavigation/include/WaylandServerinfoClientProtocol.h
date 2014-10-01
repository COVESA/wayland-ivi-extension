/***************************************************************************
*
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
*
* THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
* SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
* FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
* SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
* RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
* CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
* CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
****************************************************************************/

#ifndef WAYLANDSERVERINFOCLIENTPROTOCOL_H
#define WAYLANDSERVERINFOCLIENTPROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct serverinfo;

extern const struct wl_interface serverinfo_interface;

struct serverinfo_listener {
	void (*connection_id)(void *data,
			      struct serverinfo *serverinfo,
			      uint32_t connection_id);
};

static inline int
serverinfo_add_listener(struct serverinfo *serverinfo,
			const struct serverinfo_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) serverinfo,
				     (void (**)(void)) listener, data);
}

#define SERVERINFO_GET_CONNECTION_ID	0

static inline void
serverinfo_set_user_data(struct serverinfo *serverinfo, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) serverinfo, user_data);
}

static inline void *
serverinfo_get_user_data(struct serverinfo *serverinfo)
{
	return wl_proxy_get_user_data((struct wl_proxy *) serverinfo);
}

static inline void
serverinfo_destroy(struct serverinfo *serverinfo)
{
	wl_proxy_destroy((struct wl_proxy *) serverinfo);
}

static inline void
serverinfo_get_connection_id(struct serverinfo *serverinfo)
{
	wl_proxy_marshal((struct wl_proxy *) serverinfo,
			 SERVERINFO_GET_CONNECTION_ID);
}

#ifdef  __cplusplus
}
#endif

#endif /* WAYLANDSERVERINFOCLIENTPROTOCOL_H */
