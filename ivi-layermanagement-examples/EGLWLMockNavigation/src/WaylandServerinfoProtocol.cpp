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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

static const struct wl_interface *types[] = {
	NULL,
};

static const struct wl_message serverinfo_requests[] = {
	{ "get_connection_id", "", types + 0 },
};

static const struct wl_message serverinfo_events[] = {
	{ "connection_id", "u", types + 0 },
};

extern "C" WL_EXPORT const struct wl_interface serverinfo_interface = {
	"serverinfo", 1,
	ARRAY_LENGTH(serverinfo_requests), serverinfo_requests,
	ARRAY_LENGTH(serverinfo_events), serverinfo_events,
};
