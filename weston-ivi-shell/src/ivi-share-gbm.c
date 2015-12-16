/**************************************************************************
 *
 * Copyright 2015 Codethink Ltd
 * Copyright (C) 2015 Advanced Driver Information Technology Joint Venture GmbH
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
#include <stdint.h>

#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <libudev.h>

#include "wayland-server.h"
#include "weston/compositor.h"
#include "ivi-share-server-protocol.h"
#include "ivi-controller-interface.h"
#include "ivi-share.h"

static uint32_t nativesurface_name;

/* copied from libinput-seat.h of weston-1.9.0 */
struct udev_input {
	struct libinput *libinput;
	struct wl_event_source *libinput_source;
	struct weston_compositor *compositor;
	int suspended;
};

/* copied from compositor-drm.c of weston-1.9.0 */
struct drm_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct udev *udev;
	struct wl_event_source *drm_source;

	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_drm_source;

	struct {
		int id;
		int fd;
		char *filename;
	} drm;
	struct gbm_device *gbm;
	uint32_t *crtcs;
	int num_crtcs;
	uint32_t crtc_allocator;
	uint32_t connector_allocator;
	struct wl_listener session_listener;
	uint32_t format;

	/* we need these parameters in order to not fail drmModeAddFB2()
	 * due to out of bounds dimensions, and then mistakenly set
	 * sprites_are_broken:
	 */
	uint32_t min_width, max_width;
	uint32_t min_height, max_height;
	int no_addfb2;

	struct wl_list sprite_list;
	int sprites_are_broken;
	int sprites_hidden;

	int cursors_are_broken;

	int use_pixman;

	uint32_t prev_state;

	struct udev_input input;

	int32_t cursor_width;
	int32_t cursor_height;
};

uint32_t
get_buffer_name(struct weston_surface *surface,
                struct ivi_shell_share_ext *shell_ext)
{
     (void)surface;
     return nativesurface_name;
}

uint32_t
update_buffer_nativesurface(struct ivi_share_nativesurface *p_nativesurface,
                            struct ivi_shell_share_ext *shell_ext)
{
    if (NULL == p_nativesurface || NULL == p_nativesurface->surface) {
        return IVI_SHAREBUFFER_NOT_AVAILABLE;
    }

    struct drm_backend *backend =
        (struct drm_backend*)p_nativesurface->surface->compositor->backend;
    if (NULL == backend) {
        return IVI_SHAREBUFFER_NOT_AVAILABLE;
    }

    struct weston_buffer *buffer = p_nativesurface->surface->buffer_ref.buffer;
    if (!buffer) {
        return IVI_SHAREBUFFER_NOT_AVAILABLE;
    }

    struct gbm_bo *bo = gbm_bo_import(backend->gbm, GBM_BO_IMPORT_WL_BUFFER,
                                      buffer->legacy_buffer, GBM_BO_USE_SCANOUT);
    if (!bo) {
        weston_log("failed to import gbm_bo\n");
        return IVI_SHAREBUFFER_INVALID;
    }

    struct drm_gem_flink flink = {0};
    flink.handle = gbm_bo_get_handle(bo).u32;
    if (drmIoctl(gbm_device_get_fd(backend->gbm), DRM_IOCTL_GEM_FLINK, &flink) != 0) {
        weston_log("gem_flink: returned non-zero failed\n");
        gbm_bo_destroy(bo);
        return IVI_SHAREBUFFER_INVALID;
    }

    uint32_t name   = flink.name;
    uint32_t width  = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t format = IVI_SHARE_SURFACE_FORMAT_ARGB8888;
    uint32_t ret    = IVI_SHAREBUFFER_STABLE;

    nativesurface_name = name;

    if (name != p_nativesurface->name) {
        ret |= IVI_SHAREBUFFER_DAMAGE;
    }
    if (width != p_nativesurface->width) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }
    if (height != p_nativesurface->height) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }
    if (stride != p_nativesurface->stride) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }

    p_nativesurface->name   = name;
    p_nativesurface->width  = width;
    p_nativesurface->height = height;
    p_nativesurface->stride = stride;
    p_nativesurface->format = format;

    gbm_bo_destroy(bo);

    return ret;
}
