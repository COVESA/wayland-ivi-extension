/**************************************************************************
 *
 * Copyright 2015 Codethink Ltd
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

#include "ivi-input-client-protocol.h"
#include "ilm_input.h"
#include "ilm_control_platform.h"


/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

extern struct ilm_control_context ilm_context;

ILM_EXPORT ilmErrorTypes
ilm_setInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint num_seats,
                         t_ilm_string *seats)
{
    return ILM_FAILED;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint *num_seats,
                         t_ilm_string **seats)
{
    return ILM_FAILED;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputDevices(ilmInputDevice bitmask, t_ilm_uint *num_seats,
                    t_ilm_string **seats)
{
    return ILM_FAILED;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputDeviceCapabilities(t_ilm_string seat_name, ilmInputDevice *bitmask)
{
    return ILM_FAILED;
}

ILM_EXPORT ilmErrorTypes
ilm_setInputFocus(t_ilm_surface *surfaceIDs, t_ilm_uint num_surfaces,
                  ilmInputDevice bitmask, t_ilm_bool is_set)
{
    return ILM_FAILED;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputFocus(t_ilm_surface **surfaceIDs, ilmInputDevice **bitmasks,
                  t_ilm_uint *num_ids)
{
    return ILM_FAILED;
}
