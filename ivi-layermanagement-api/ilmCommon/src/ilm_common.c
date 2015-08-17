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

ILM_EXPORT ilmErrorTypes ilmControl_init(t_ilm_nativedisplay);
ILM_EXPORT void ilmControl_destroy(void);

static pthread_mutex_t g_initialize_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_deinitialize_lock = PTHREAD_MUTEX_INITIALIZER;
static int gInitialized = 0;

ILM_EXPORT ilmErrorTypes
ilm_init(void)
{
    return ilm_initWithNativedisplay(0);
}

ILM_EXPORT ilmErrorTypes
ilm_initWithNativedisplay(t_ilm_nativedisplay nativedisplay)
{
    ilmErrorTypes err = ILM_SUCCESS;
    ilmErrorTypes ret = ILM_FAILED;
    t_ilm_nativedisplay display = 0;

    pthread_mutex_lock(&g_initialize_lock);

    do
    {
        init_ilmCommonPlatformTable();

        if (ilm_isInitialized())
        {
            fprintf(stderr, "[Warning] ilm_init or ilm_initWithNativedisplay is called,\n"
                            "          but ilmClientLib has been already initialized.\n"
                            "          gInitialized is incremented by one.\n"
                            "          Returning success and incrementing gInitialized.\n"
                            "          Initialization is skipped at this time.\n");
            gInitialized++;
            ret = ILM_SUCCESS;
            break;
        }

        err = gIlmCommonPlatformFunc.init(nativedisplay);
        if (ILM_SUCCESS != err)
        {
            break;
        }

        display = gIlmCommonPlatformFunc.getNativedisplay();

        err = ilmControl_init(display);
        if (ILM_SUCCESS != err)
        {
            gIlmCommonPlatformFunc.destroy();
            break;
        }

        gInitialized++;
        ret = ILM_SUCCESS;

    } while (0);

    pthread_mutex_unlock(&g_initialize_lock);

    return ret;
}

ILM_EXPORT t_ilm_bool
ilm_isInitialized(void)
{
    return gIlmCommonPlatformFunc.isInitialized();
}

ILM_EXPORT ilmErrorTypes
ilm_destroy(void)
{
    ilmErrorTypes retVal;

    pthread_mutex_lock(&g_deinitialize_lock);
    if (gInitialized > 1)
    {
        fprintf(stderr, "[Warning] ilm_destroy is called, but gInitialized is %d.\n"
                        "          Returning success and deinitialization is skipped\n"
                        "          at this time.\n", gInitialized);
        retVal = ILM_SUCCESS;
    }
    else
    {
        ilmControl_destroy(); // block until control thread is stopped
        retVal = gIlmCommonPlatformFunc.destroy();
    }

    if (retVal == ILM_SUCCESS)
    {
        gInitialized--;
    }

    pthread_mutex_unlock(&g_deinitialize_lock);

    return retVal;
}
