
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
#include <signal.h>
#include <stdint.h>
#include "ilm_client.h"
#include "ilm_client_platform.h"

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

ILM_EXPORT ilmErrorTypes
ilmClient_init(t_ilm_nativedisplay nativedisplay)
{
    init_ilmClientPlatformTable();

    return gIlmClientPlatformFunc.init(nativedisplay);
}

ILM_EXPORT ilmErrorTypes
ilmClient_destroy(void)
{
    return gIlmClientPlatformFunc.destroy();
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceCreate(t_ilm_nativehandle nativehandle,
                  t_ilm_int width, t_ilm_int height,
                  ilmPixelFormat pixelFormat, t_ilm_surface* pSurfaceId)
{
    return gIlmClientPlatformFunc.surfaceCreate(
               nativehandle, width, height, pixelFormat, pSurfaceId);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceRemove(t_ilm_surface surfaceId)
{
    return gIlmClientPlatformFunc.surfaceRemove(surfaceId);
}
