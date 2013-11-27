/*
 * Copyright (C) 2013 DENSO CORPORATION
 * Copyright (c) 2013 BMW Car IT GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IVI_APPLICATION_CLIENT_PROTOCOL_H
#define IVI_APPLICATION_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct ivi_surface;
struct ivi_application;

extern const struct wl_interface ivi_surface_interface;
extern const struct wl_interface ivi_application_interface;

/**
 * ivi_surface - Tell property change of ivi_surface to application
 * @visibility: visibility of ivi_surface has changed
 *
 *
 */
struct ivi_surface_listener {
	/**
	 * visibility - visibility of ivi_surface has changed
	 * @visibility: (none)
	 *
	 *
	 */
	void (*visibility)(void *data,
			   struct ivi_surface *ivi_surface,
			   int32_t visibility);
};

static inline int
ivi_surface_add_listener(struct ivi_surface *ivi_surface,
			 const struct ivi_surface_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ivi_surface,
				     (void (**)(void)) listener, data);
}

#define IVI_SURFACE_DESTROY	0

static inline void
ivi_surface_set_user_data(struct ivi_surface *ivi_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_surface, user_data);
}

static inline void *
ivi_surface_get_user_data(struct ivi_surface *ivi_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_surface);
}

static inline void
ivi_surface_destroy(struct ivi_surface *ivi_surface)
{
	wl_proxy_marshal((struct wl_proxy *) ivi_surface,
			 IVI_SURFACE_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) ivi_surface);
}

#ifndef IVI_APPLICATION_ERROR_CODE_ENUM
#define IVI_APPLICATION_ERROR_CODE_ENUM
/**
 * ivi_application_error_code - possible error codes returned in error
 *	event
 * @IVI_APPLICATION_ERROR_CODE_UNKNOWN_ERROR: unknown error encountered
 * @IVI_APPLICATION_ERROR_CODE_RESOURCE_IN_USE: resource is in use and
 *	can not be shared
 *
 *
 */
enum ivi_application_error_code {
	IVI_APPLICATION_ERROR_CODE_UNKNOWN_ERROR = 1,
	IVI_APPLICATION_ERROR_CODE_RESOURCE_IN_USE = 2,
};
#endif /* IVI_APPLICATION_ERROR_CODE_ENUM */

/**
 * ivi_application - creation of ivi_surface
 * @error: request resulted in server-side error
 *
 *
 */
struct ivi_application_listener {
	/**
	 * error - request resulted in server-side error
	 * @error_code: (none)
	 * @error_text: (none)
	 *
	 *
	 */
	void (*error)(void *data,
		      struct ivi_application *ivi_application,
		      int32_t error_code,
		      const char *error_text);
};

static inline int
ivi_application_add_listener(struct ivi_application *ivi_application,
			     const struct ivi_application_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ivi_application,
				     (void (**)(void)) listener, data);
}

#define IVI_APPLICATION_SURFACE_CREATE	0

static inline void
ivi_application_set_user_data(struct ivi_application *ivi_application, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ivi_application, user_data);
}

static inline void *
ivi_application_get_user_data(struct ivi_application *ivi_application)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ivi_application);
}

static inline void
ivi_application_destroy(struct ivi_application *ivi_application)
{
	wl_proxy_destroy((struct wl_proxy *) ivi_application);
}

static inline struct ivi_surface *
ivi_application_surface_create(struct ivi_application *ivi_application, uint32_t id_surface, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_create((struct wl_proxy *) ivi_application,
			     &ivi_surface_interface);
	if (!id)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) ivi_application,
			 IVI_APPLICATION_SURFACE_CREATE, id_surface, surface, id);

	return (struct ivi_surface *) id;
}

#ifdef  __cplusplus
}
#endif

#endif
