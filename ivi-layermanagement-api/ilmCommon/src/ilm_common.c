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

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

ILM_EXPORT ilmErrorTypes ilmClient_init(t_ilm_nativedisplay);
ILM_EXPORT ilmErrorTypes ilmControl_init(t_ilm_nativedisplay);
ILM_EXPORT void ilmClient_destroy(void);
ILM_EXPORT void ilmControl_destroy(void);

ILM_EXPORT ilmErrorTypes
ilm_init(void)
{
    return ilm_initWithNativedisplay(0);
}

ILM_EXPORT ilmErrorTypes
ilm_initWithNativedisplay(t_ilm_nativedisplay nativedisplay)
{
    ilmErrorTypes err = ILM_SUCCESS;
    t_ilm_nativedisplay display = 0;

    init_ilmCommonPlatformTable();

    err = gIlmCommonPlatformFunc.init(nativedisplay);
    if (ILM_SUCCESS != err)
    {
        return err;
    }

    display = gIlmCommonPlatformFunc.getNativedisplay();

    err = ilmClient_init(display);
    if (ILM_SUCCESS != err)
    {
        gIlmCommonPlatformFunc.destroy();
        return err;
    }

    err = ilmControl_init(display);
    if (ILM_SUCCESS != err)
    {
        ilmClient_destroy();
        gIlmCommonPlatformFunc.destroy();
        return err;
    }

    return ILM_SUCCESS;
}

ILM_EXPORT t_ilm_bool
ilm_isInitialized(void)
{
    return gIlmCommonPlatformFunc.isInitialized();
}

ILM_EXPORT ilmErrorTypes
ilm_destroy(void)
{
    ilmControl_destroy(); // block until control thread is stopped
    ilmClient_destroy();
    ilmErrorTypes retVal = gIlmCommonPlatformFunc.destroy();
    return retVal;
}
