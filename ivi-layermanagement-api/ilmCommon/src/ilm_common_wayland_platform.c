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
#include "ilm_tools.h"
#include "ilm_types.h"
#include "ilm_configuration.h"
#include "wayland-util.h"

static ilmErrorTypes wayland_init(t_ilm_nativedisplay nativedisplay);
static t_ilm_bool wayland_isInitialized();
static ilmErrorTypes wayland_destroy();

void init_ilmCommonPlatformTable()
{
    gIlmCommonPlatformFunc.init = wayland_init;
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

/* available to all client APIs, exported in ilm_common.h */
struct IpcModule gIpcModule;

struct ilm_common_context {
    int32_t valid;
};

static struct ilm_common_context ilm_context = {0};

static ilmErrorTypes
wayland_init(t_ilm_nativedisplay nativedisplay)
{
    struct ilm_common_context *ctx = &ilm_context;
    (void)nativedisplay;

    ctx->valid = 1;

    return ILM_SUCCESS;
}

static t_ilm_bool
wayland_isInitialized()
{
    struct ilm_common_context *ctx = &ilm_context;

    if (ctx->valid != 0) {
        return ILM_TRUE;
    } else {
        return ILM_FALSE;
    }
}

static ilmErrorTypes
wayland_destroy()
{
    struct ilm_common_context *ctx = &ilm_context;

    ctx->valid = 0;

    return ILM_SUCCESS;
}
