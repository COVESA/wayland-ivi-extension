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

#include "ivi-share-server-protocol.h"
#include "ivi-share.h"
#include "ivi-share-gbm.h"

uint32_t
get_buffer_name(struct ivi_share_nativesurface *nativesurface)
{
     return nativesurface->name;
}

uint32_t
get_flink_from_bo(struct gbm_bo *bo, struct gbm_device *gbm, struct drm_gem_flink *flink)
{
    uint32_t ret = 0;
    flink->handle = gbm_bo_get_handle(bo).u32;
    if (0 != drmIoctl(gbm_device_get_fd(gbm), DRM_IOCTL_GEM_FLINK, flink)) {
        weston_log("Texture Sharing gem_flink: returned non-zero failed\n");
        ret |= IVI_SHAREBUFFER_INVALID;
    }
    return ret;
}

uint32_t
update_buffer_nativesurface(struct ivi_share_nativesurface *p_nativesurface,
                            struct ivi_shell_share_ext *shell_ext)
{
    if (NULL == p_nativesurface || NULL == p_nativesurface->surface) {
        return IVI_SHAREBUFFER_NOT_AVAILABLE;
    }

    struct drm_gem_flink flink;
    uint32_t ret = IVI_SHAREBUFFER_STABLE;

    if (p_nativesurface->surface->buffer_ref.buffer == NULL) {
        return IVI_SHAREBUFFER_INVALID;
    }

    struct weston_buffer *buf = p_nativesurface->surface->buffer_ref.buffer;

    struct drm_backend *backend =
        (struct drm_backend*)p_nativesurface->surface->compositor->backend;
    if (NULL == backend) {
        return IVI_SHAREBUFFER_NOT_AVAILABLE;
    }

    struct gbm_bo *bo = gbm_bo_import(backend->gbm, GBM_BO_IMPORT_WL_BUFFER,
                                      buf->legacy_buffer, GBM_BO_USE_SCANOUT);

    if (!bo) {
        weston_log("Texture Sharing Failed to import gbm_bo\n");
        return IVI_SHAREBUFFER_INVALID;
    }

    uint32_t name;
    uint32_t width  = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t format = IVI_SHARE_SURFACE_FORMAT_ARGB8888;

#ifdef ENABLE_SHARE_SUBSURFACE
    check_subsurface(p_nativesurface);

    if (p_nativesurface->has_subsurface) {
        ret |= update_subsurfaces(p_nativesurface);
    } else {
#endif
        uint32_t flink_ret;
        flink_ret = get_flink_from_bo(bo, backend->gbm, &flink);
        if (flink_ret == IVI_SHAREBUFFER_INVALID) {
            ret |= IVI_SHAREBUFFER_INVALID;
        }

        name = flink.name;
        if (name != p_nativesurface->name) {
            ret |= IVI_SHAREBUFFER_DAMAGE;
        }
        p_nativesurface->name = name;
#ifdef ENABLE_SHARE_SUBSURFACE
    }
#endif
    if (width != p_nativesurface->width) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }
    if (height != p_nativesurface->height) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }
    if (stride != p_nativesurface->stride) {
        ret |= IVI_SHAREBUFFER_CONFIGURE;
    }
    p_nativesurface->format = format;

    p_nativesurface->width  = width;
    p_nativesurface->height = height;
    p_nativesurface->stride = stride;

    gbm_bo_destroy(bo);

    return ret;
}
