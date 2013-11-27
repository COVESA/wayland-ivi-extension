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

#ifndef IVI_APPLICATION_SERVER_PROTOCOL_H
#define IVI_APPLICATION_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct ivi_surface;
struct ivi_application;

extern const struct wl_interface ivi_surface_interface;
extern const struct wl_interface ivi_application_interface;

/**
 * ivi_surface - Tell property change of ivi_surface to application
 * @destroy: destroy ivi_surface
 *
 *
 */
struct ivi_surface_interface {
	/**
	 * destroy - destroy ivi_surface
	 *
	 *
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#define IVI_SURFACE_VISIBILITY	0

static inline void
ivi_surface_send_visibility(struct wl_resource *resource_, int32_t visibility)
{
	wl_resource_post_event(resource_, IVI_SURFACE_VISIBILITY, visibility);
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
 * @surface_create: ilm_surfaceCreate
 *
 *
 */
struct ivi_application_interface {
	/**
	 * surface_create - ilm_surfaceCreate
	 * @id_surface: (none)
	 * @surface: (none)
	 * @id: (none)
	 *
	 *
	 */
	void (*surface_create)(struct wl_client *client,
			       struct wl_resource *resource,
			       uint32_t id_surface,
			       struct wl_resource *surface,
			       uint32_t id);
};

#define IVI_APPLICATION_ERROR	0

static inline void
ivi_application_send_error(struct wl_resource *resource_, int32_t error_code, const char *error_text)
{
	wl_resource_post_event(resource_, IVI_APPLICATION_ERROR, error_code, error_text);
}

#ifdef  __cplusplus
}
#endif

#endif
