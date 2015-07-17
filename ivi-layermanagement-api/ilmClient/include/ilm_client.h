/***************************************************************************
*
* Copyright 2010,2011 BMW Car IT GmbH
* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef _ILM_CLIENT_H_
#define _ILM_CLIENT_H_

#ifdef __cplusplus

extern "C" {
#endif /* __cplusplus */

#include "ilm_common.h"

/**
 * \brief  Initializes the IVI LayerManagement Client APIs.
 * \ingroup ilmControl
 * \param[in] nativedisplay the wl_display of native application
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if a connection can not be established to the services.
 */
ilmErrorTypes ilmClient_init(t_ilm_nativedisplay nativedisplay);

/**
 * \brief Create a surface
 * \ingroup ilmClient
 * \param[in] nativehandle The native windowsystem's handle for the surface
 * \param[in] width The original width of the surface
 * \param[in] height The original height of the surface
 * \param[in] pixelFormat The pixelformat to be used for the surface
 * \param[in] pSurfaceId The value pSurfaceId points to is used as ID for new surface;
 * \param[out] pSurfaceId The ID of the newly created surface is returned in this parameter
 *
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceCreate(t_ilm_nativehandle nativehandle,
                                t_ilm_int width,
                                t_ilm_int height,
                                ilmPixelFormat pixelFormat,
                                t_ilm_surface *pSurfaceId);

/**
 * \brief Remove a surface
 * \ingroup ilmClient
 * \param[in] surfaceId The id of the surface to be removed
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceRemove(const t_ilm_surface surfaceId);

/**
 * \brief Destroys the IVI LayerManagement Client APIs.
 * \ingroup ilmCommon
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not be closed or was not initialized.
 */
ilmErrorTypes ilmClient_destroy(void);

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CLIENT_H_ */
