/***************************************************************************
*
* Copyright 2015, Codethink Ltd.
*
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
#ifndef _ILM_INPUT_H_
#define _ILM_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ilm_common.h"

/**
 * \brief      Set the surface's accepted seats to the list specified
 * \ingroup    ilmControl
 * \param[in]  surfaceID   The surface whose list of accepted seats is to be
 *                         changed
 * \param[in]  num_seats   The number of seats stored in seats
 * \param[in]  seats       A pointer to an array of strings listing each seat
 *                         to accept by its seat name
 * \return     ILM_SUCCESS if the method call was successful
 * \return     ILM_FAILED  if the client cannot call the method on the surface
 */
ilmErrorTypes
ilm_setInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint num_seats,
                         t_ilm_string *seats);

/**
 * \brief      Get the surface's list of accepted seats
 * \ingroup    ilmControl
 * \param[in]  surfaceID   The surface that the list of seats comes from
 * \param[out] num_seats   The number of seats returned
 * \param[out] seats       A pointer to the memory where an array of seats is
 *                         stored. It is the caller's responsibility to free
 *                         this memory after use.
 * \return     ILM_SUCCESS if the method call was successful
 * \return     ILM_FAILED  if the client cannot call the method on the surface
 */
ilmErrorTypes
ilm_getInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint *num_seats,
                         t_ilm_string **seats);

/**
 * \brief      Get the list of seats that support the device types specified in
 *             bitmask
 * \ingroup    ilmControl
 * \param[in]  bitmask      The bitmask that indicates what type of devices
 *                          are requested (e.g. ILM_INPUT_DEVICE_POINTER)
 * \param[out] num_seats    The number of seats returned
 * \param[out] seats        A pointer to the memory where an array of seats is
 *                          stored. It is the caller's responsibility to free
 *                          this memory after use.
 * \return     ILM_SUCCESS  if the method call was successful
 * \return     ILM_FAILED   if the method call was unsuccessful
 */
ilmErrorTypes
ilm_getInputDevices(ilmInputDevice bitmask, t_ilm_uint *num_seats,
                    t_ilm_string **seats);
/**
 * \brief      Get the device capabilities of a seat
 * \ingroup    ilmControl
 * \param[in]  seat_name    The name of the seat
 * \param[out] bitmask      A pointer to the bitmask that should be set
 * \return     ILM_SUCCESS  if the method call was successful
 * \return     ILM_FAILED   if the method call was unsuccessful
 */
ilmErrorTypes
ilm_getInputDeviceCapabilities(t_ilm_string seat_name, ilmInputDevice* bitmask);

/**
 * \brief      Set whether the specified surfaces have input focus set for the
 *             given device types
 * \ingroup    ilmControl
 * \param[in]  surfaceIDs   An array of surface IDs whose input focus may be
 *                          changed
 * \param[in]  num_surfaces The number of surfaces in surfaceIDs
 * \param[in]  bitmask      A bitmask of the types of device for which focus
 *                          will be set
 * \param[in]  is_set       ILM_TRUE if focus is to be set, ILM_FALSE if focus
 *                          is to be unset
 * \return     ILM_SUCCESS if the method call was successful
 * \return     ILM_FAILED  if the method call was unsuccessful
 */
ilmErrorTypes
ilm_setInputFocus(t_ilm_surface *surfaceIDs, t_ilm_uint num_surfaces,
                  ilmInputDevice bitmask, t_ilm_bool is_set);

/**
 * \brief      Get all surface IDs and their corresponding focus bitmasks
 * \ingroup    ilmControl
 * \param[out] surfaceIDs  A pointer to the memory where an array of surface
 *                         IDs will be created. The caller is responsible for
 *                         freeing this memory after use.
 * \param[out] bitmasks    A pointer to the memory where an array of bitmasks
 *                         will be created. The caller is responsible for
 *                         freeing this memory after use.
 * \param[out] num_ids     The number of surface IDs that were returned
 * \return     ILM_SUCCESS if the method call was successful
 * \return     ILM_FAILED  if the method call was unsuccessful
 */
ilmErrorTypes
ilm_getInputFocus(t_ilm_surface **surfaceIDs, ilmInputDevice** bitmasks,
                  t_ilm_uint *num_ids);

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_INPUT_H_ */
