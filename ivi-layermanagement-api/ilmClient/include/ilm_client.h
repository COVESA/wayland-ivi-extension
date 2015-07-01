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
 * \brief Get the screen resolution of a specific screen from the Layermanagement
 * \ingroup ilmClient
 * \param[in] screenID Screen Indentifier as a Number from 0 .. MaxNumber of Screens
 * \param[out] pWidth pointer where width of screen should be stored
 * \param[out] pHeight pointer where height of screen should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not get the resolution.
 */
ilmErrorTypes ilm_getScreenResolution(t_ilm_uint screenID, t_ilm_uint* pWidth, t_ilm_uint* pHeight);

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
 * \brief Get the horizontal and vertical dimension of the surface.
 * \ingroup ilmClient
 * \param[in] surfaceId Id of surface.
 * \param[out] pDimension pointer to an array where the dimension should be stored.
 *                        dimension[0]=width, dimension[1]=height
 *
 *
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceGetDimension(t_ilm_surface surfaceId, t_ilm_uint *pDimension);

/**
 * \brief Get the visibility of a surface. If a surface is not visible, the surface
 * will not be rendered.
 * \ingroup ilmClient
 * \param[in] surfaceId Id of the surface to get the visibility of.
 * \param[out] pVisibility pointer where the visibility of a surface should be stored
 *                         ILM_SUCCESS if the surface is visible,
 *                         ILM_FALSE if the visibility is disabled.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceGetVisibility(t_ilm_surface surfaceId, t_ilm_bool *pVisibility);

/**
 * \brief Remove a surface
 * \ingroup ilmClient
 * \param[in] surfaceId The id of the surface to be removed
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceRemove(const t_ilm_surface surfaceId);

/**
 * \brief Remove the native content of a surface
 * \ingroup ilmClient
 * \param[in] surfaceId The ID of the surface
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceRemoveNativeContent(t_ilm_surface surfaceId);

/**
 * \brief Set the native content of an application to be used as surface content
 * \ingroup ilmClient
 * \param[in] nativehandle The native windowsystem's handle for the surface
 * \param[in] width The original width of the surface
 * \param[in] height The original height of the surface
 * \param[in] pixelFormat The pixelformat to be used for the surface
 * \param[in] surfaceId The ID of the surface
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetNativeContent(t_ilm_nativehandle nativehandle,
                                            t_ilm_int width,
                                            t_ilm_int height,
                                            ilmPixelFormat pixelFormat,
                                            t_ilm_surface surfaceId);

/**
 * \brief Set the area of a surface which should be used for the rendering.
 * \ingroup ilmClient
 * \param[in] surfaceId Id of surface.
 * \param[in] x horizontal start position of the used area
 * \param[in] y vertical start position of the used area
 * \param[in] width width of the area
 * \param[in] height height of the area
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetSourceRectangle(t_ilm_surface surfaceId, t_ilm_int x, t_ilm_int y, t_ilm_int width, t_ilm_int height);

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CLIENT_H_ */
