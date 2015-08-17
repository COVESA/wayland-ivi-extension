/**************************************************************************
 *
 * Copyright (C) 2013 DENSO CORPORATION
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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include "ilm_common.h"
#include "ilm_common_platform.h"
#include "ilm_types.h"
#include "wayland-util.h"
#include "wayland-client.h"

static ilmErrorTypes wayland_init(t_ilm_nativedisplay nativedisplay);
static t_ilm_nativedisplay wayland_getNativedisplay(void);
static t_ilm_bool wayland_isInitialized(void);
static ilmErrorTypes wayland_destroy(void);

void init_ilmCommonPlatformTable(void)
{
    gIlmCommonPlatformFunc.init = wayland_init;
    gIlmCommonPlatformFunc.getNativedisplay = wayland_getNativedisplay;
    gIlmCommonPlatformFunc.isInitialized = wayland_isInitialized;
    gIlmCommonPlatformFunc.destroy = wayland_destroy;
}

/*
 *=============================================================================
 * global vars
 *=============================================================================
 */
/* automatically gets assigned argv[0] */
extern char *__progname;

struct ilm_common_context {
    int32_t valid;
    int32_t disconnect_display;
    struct wl_display *display;
};

static struct ilm_common_context ilm_context = {0};

static ilmErrorTypes
wayland_init(t_ilm_nativedisplay nativedisplay)
{
    struct ilm_common_context *ctx = &ilm_context;

    if (nativedisplay != 0) {
        ctx->display = (struct wl_display*)nativedisplay;
        ctx->disconnect_display = 0;
    } else {
        ctx->display = wl_display_connect(NULL);
        ctx->disconnect_display = 1;
        if (ctx->display == NULL) {
            fprintf(stderr, "Failed to connect display in libilmCommon\n");
            return ILM_FAILED;
        }
    }

    ctx->valid = 1;

    return ILM_SUCCESS;
}

static t_ilm_nativedisplay
wayland_getNativedisplay(void)
{
    struct ilm_common_context *ctx = &ilm_context;

    return (t_ilm_nativedisplay)ctx->display;
}

static t_ilm_bool
wayland_isInitialized(void)
{
    struct ilm_common_context *ctx = &ilm_context;

    if (ctx->valid != 0) {
        return ILM_TRUE;
    } else {
        return ILM_FALSE;
    }
}

static ilmErrorTypes
wayland_destroy(void)
{
    struct ilm_common_context *ctx = &ilm_context;

    if (ctx->valid == 0)
    {
        fprintf(stderr, "[Warning] The ilm_common_context is already destroyed\n");
        return ILM_FAILED;
    }

    ctx->valid = 0;

    // we own the display, act like it.
    if (ctx->disconnect_display)
    {
       wl_display_roundtrip(ctx->display);
       wl_display_disconnect(ctx->display);
       ctx->display = NULL;
    }

    return ILM_SUCCESS;
}
