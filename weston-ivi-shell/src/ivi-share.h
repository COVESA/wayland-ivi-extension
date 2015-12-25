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
#include <stdbool.h>
#include <sys/time.h>

#include <wayland-server.h>
#include <weston/compositor.h>

#include "ivi-controller-interface.h"

/**
 * convenience macro to access single bits of a bitmask
 */
#define IVI_BIT(x) (1 << (x))

struct ivi_share_nativesurface
{
    struct weston_surface *surface; /* resource                                   */
    uint32_t id;                    /* object id                                  */
    struct wl_list link;            /* link                                       */
    struct wl_list client_list;     /* ivi_nativesurface_client_link list         */
    uint32_t bufferType;            /* buffer type (GBM only)                     */
    uint32_t name;                  /* buffer name                                */
    uint32_t width;                 /* buffer width                               */
    uint32_t height;                /* buffer height                              */
    uint32_t stride;                /* buffer stride[LSB:byte]                    */
    uint32_t format;                /* ARGB8888                                   */
    uint32_t surface_id;
    uint32_t send_flag;
    struct wl_listener surface_destroy_listener;
    struct ivi_shell_share_ext *shell_ext;
};

struct ivi_shell_share_ext
{
    struct weston_compositor *wc;
    const struct ivi_controller_interface *controller_interface;
    struct wl_resource *resource;
    struct wl_list list_shell_surface;           /* shell_surface list */
    struct wl_list list_nativesurface;           /* ivi_nativesurface list */
    struct wl_list list_redirect_target;         /* redirect_target list */
    struct wl_listener surface_created_listener;
};

enum ivi_sharebuffer_updatetype
{
    IVI_SHAREBUFFER_STABLE        = IVI_BIT(0),
    IVI_SHAREBUFFER_DAMAGE        = IVI_BIT(1),
    IVI_SHAREBUFFER_CONFIGURE     = IVI_BIT(2),
    IVI_SHAREBUFFER_INVALID       = IVI_BIT(3),
    IVI_SHAREBUFFER_NOT_AVAILABLE = IVI_BIT(4)
};

int32_t setup_buffer_sharing(struct weston_compositor *wc,
                             const struct ivi_controller_interface *interface);

uint32_t get_buffer_name(struct weston_surface *surface,
                         struct ivi_shell_share_ext *shell_ext);
uint32_t update_buffer_nativesurface(struct ivi_share_nativesurface *nativesurface,
                                     struct ivi_shell_share_ext *shell_ext);
