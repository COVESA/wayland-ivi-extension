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
#ifndef _ILM_CLIENT_PLATFORM_H_
#define _ILM_CLIENT_PLATFORM_H_

#ifdef __cplusplus

extern "C" {
#endif /* __cplusplus */

#include "ilm_common.h"

typedef struct _ILM_CLIENT_PLATFORM_FUNC
{
    ilmErrorTypes (*surfaceCreate)(t_ilm_nativehandle nativehandle,
                   t_ilm_int width, t_ilm_int height,
                   ilmPixelFormat pixelFormat, t_ilm_surface* pSurfaceId);
    ilmErrorTypes (*surfaceRemove)(const t_ilm_surface surfaceId);
    ilmErrorTypes (*init)(t_ilm_nativedisplay nativedisplay);
    ilmErrorTypes (*destroy)();
} ILM_CLIENT_PLATFORM_FUNC;

ILM_CLIENT_PLATFORM_FUNC gIlmClientPlatformFunc;

void init_ilmClientPlatformTable();

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CLIENT_PLATFORM_H_ */
