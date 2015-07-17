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
#ifndef _ILM_CONTROL_H_
#define _ILM_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ilm_common.h"

/**
 * \brief Get the surface properties from the Layermanagement
 * \ingroup ilmClient
 * \param[in] surfaceID surface Indentifier as a Number from 0 .. MaxNumber of Surfaces
 * \param[out] pSurfaceProperties pointer where the surface properties should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not get the resolution.
 */
ilmErrorTypes ilm_getPropertiesOfSurface(t_ilm_uint surfaceID, struct ilmSurfaceProperties* pSurfaceProperties);

/**
 * \brief  Get the layer properties from the Layermanagement
 * \ingroup ilmControl
 * \param[in] layerID layer Indentifier as a Number from 0 .. MaxNumber of Layer
 * \param[out] pLayerProperties pointer where the layer properties should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not get the resolution.
 */
ilmErrorTypes ilm_getPropertiesOfLayer(t_ilm_uint layerID, struct ilmLayerProperties* pLayerProperties);

/**
 * \brief Get the screen properties from the Layermanagement
 * \ingroup ilmControl
 * \param[in] screenID screen Indentifier
 * \param[out] pScreenProperties pointer where the screen properties should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not get the resolution.
 */
ilmErrorTypes ilm_getPropertiesOfScreen(t_ilm_display screenID, struct ilmScreenProperties* pScreenProperties);

/**
 * \brief Get the screen Ids
 * \ingroup ilmControl
 * \param[out] pNumberOfIDs pointer where the number of Screen Ids should be returned
 * \param[out] ppIDs pointer to array where the IDs should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not get the resolution.
 */
ilmErrorTypes ilm_getScreenIDs(t_ilm_uint* pNumberOfIDs, t_ilm_uint** ppIDs);

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
 * \brief Get all LayerIds which are currently registered and managed by the services
 * \ingroup ilmControl
 * \param[out] pLength Pointer where length of ids array should be stored
 * \param[out] ppArray Array where the ids should be stored,
 *                     the array will be allocated inside
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_getLayerIDs(t_ilm_int* pLength, t_ilm_layer** ppArray);

/**
 * \brief Get all LayerIds of the given screen
 * \ingroup ilmControl
 * \param[in] screenID The id of the screen to get the layer IDs of
 * \param[out] pLength Pointer where length of ids array should be stored
 * \param[out] ppArray Array where the ids should be stored,
 *                     the array will be allocated inside
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_getLayerIDsOnScreen(t_ilm_uint screenID, t_ilm_int* pLength, t_ilm_layer** ppArray);

/**
 * \brief Get all SurfaceIDs which are currently registered and managed by the services
 * \ingroup ilmControl
 * \param[out] pLength Pointer where length of ids array should be stored
 * \param[out] ppArray Array where the ids should be stored,
 *                     the array will be allocated inside
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_getSurfaceIDs(t_ilm_int* pLength, t_ilm_surface** ppArray);

/**
 * \brief Get all SurfaceIds which are currently registered to a given layer and are managed by the services
 * \ingroup ilmControl
 * \param[in] layer Id of the Layer whose surfaces are to be returned
 * \param[out] pLength Pointer where the array length of ids should be stored
 * \param[out] ppArray Array where the surface id should be stored,
 *                     the array will be allocated inside
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_getSurfaceIDsOnLayer(t_ilm_layer layer, t_ilm_int* pLength, t_ilm_surface** ppArray);

/**
 * \brief Create a layer which should be managed by the service
 * \ingroup ilmControl
 * \param[out] pLayerId pointer where the id should be/is stored. It is possible
 *                      to set a id from outside, 0 will create a new id.
 * \param[in] width     horizontal dimension of the layer
 *
 * \param[in] height    vertical dimension of the layer
 *
 *
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerCreateWithDimension(t_ilm_layer* pLayerId, t_ilm_uint width, t_ilm_uint height);

/**
 * \brief Removes a layer which is currently managed by the service
 * \ingroup ilmControl
 * \param[in] layerId Layer to be removed
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerRemove(t_ilm_layer layerId);

/**
 * \brief Add a surface to a layer which is currently managed by the service
 * \ingroup ilmClient
 * \param[in] layerId Id of layer which should host the surface.
 * \param[in] surfaceId Id of surface which should be added to the layer.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerAddSurface(t_ilm_layer layerId, t_ilm_surface surfaceId);

/**
 * \brief Removes a surface from a layer which is currently managed by the service
 * \ingroup ilmClient
 * \param[in] layerId Id of the layer which contains the surface.
 * \param[in] surfaceId Id of the surface which should be removed from the layer.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerRemoveSurface(t_ilm_layer layerId, t_ilm_surface surfaceId);

/**
 * \brief Set the visibility of a layer. If a layer is not visible, the layer and its
 * surfaces will not be rendered.
 * \ingroup ilmControl
 * \param[in] layerId Id of the layer.
 * \param[in] newVisibility ILM_SUCCESS sets layer visible, ILM_FALSE disables the visibility.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetVisibility(t_ilm_layer layerId, t_ilm_bool newVisibility);

/**
 * \brief Get the visibility of a layer. If a layer is not visible, the layer and its
 * surfaces will not be rendered.
 * \ingroup ilmControl
 * \param[in] layerId Id of layer.
 * \param[out] pVisibility pointer where the visibility of the layer should be stored
 *                         ILM_SUCCESS if the Layer is visible,
 *                         ILM_FALSE if the visibility is disabled.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerGetVisibility(t_ilm_layer layerId, t_ilm_bool *pVisibility);

/**
 * \brief Set the opacity of a layer.
 * \ingroup ilmControl
 * \param[in] layerId Id of the layer.
 * \param[in] opacity 0.0 means the layer is fully transparent,
 *                    1.0 means the layer is fully opaque
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetOpacity(t_ilm_layer layerId, t_ilm_float opacity);

/**
 * \brief Get the opacity of a layer.
 * \ingroup ilmControl
 * \param[in] layerId Id of the layer to obtain the opacity of.
 * \param[out] pOpacity pointer where the layer opacity should be stored.
 *                      0.0 means the layer is fully transparent,
 *                      1.0 means the layer is fully opaque
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerGetOpacity(t_ilm_layer layerId, t_ilm_float *pOpacity);

/**
 * \brief Set the area of a layer which should be used for the rendering. Only this part will be visible.
 * \ingroup ilmControl
 * \param[in] layerId Id of the layer.
 * \param[in] x horizontal start position of the used area
 * \param[in] y vertical start position of the used area
 * \param[in] width width of the area
 * \param[in] height height of the area
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetSourceRectangle(t_ilm_layer layerId, t_ilm_uint x, t_ilm_uint y, t_ilm_uint width, t_ilm_uint height);

/**
 * \brief Set the destination area on the display for a layer. The layer will be scaled and positioned to this rectangle for rendering
 * \ingroup ilmControl
 * \param[in] layerId Id of the layer.
 * \param[in] x horizontal start position of the used area
 * \param[in] y vertical start position of the used area
 * \param[in] width width of the area
 * \param[in] height height of the area
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetDestinationRectangle(t_ilm_layer layerId, t_ilm_int x, t_ilm_int y, t_ilm_int width, t_ilm_int height);

/**
 * \brief Sets the orientation of a layer.
 * \ingroup ilmControl
 * \param[in] layerId Id of layer.
 * \param[in] orientation Orientation of the layer.
 * \note ilmOrientation for more information on orientation values
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetOrientation(t_ilm_layer layerId, ilmOrientation orientation);

/**
 * \brief Gets the orientation of a layer.
 * \ingroup ilmControl
 * \param[in] layerId Id of layer.
 * \param[out] pOrientation Address where orientation of the layer should be stored.
 * \note ilmOrientation for more information on orientation values
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerGetOrientation(t_ilm_layer layerId, ilmOrientation *pOrientation);

/**
 * \brief Sets render order of surfaces within one layer
 * \ingroup ilmControl
 * \param[in] layerId Id of layer.
 * \param[in] pSurfaceId array of surface ids
 * \param[in] number Number of elements in the given array of ids
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_layerSetRenderOrder(t_ilm_layer layerId, t_ilm_layer *pSurfaceId, t_ilm_int number);

/**
 * \brief Set the visibility of a surface. If a surface is not visible it will not be rendered.
 * \ingroup ilmControl
 * \param[in] surfaceId Id of the surface to set the visibility of
 * \param[in] newVisibility ILM_SUCCESS sets surface visible, ILM_FALSE disables the visibility.
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetVisibility(t_ilm_surface surfaceId, t_ilm_bool newVisibility);

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
 * \brief Set the opacity of a surface.
 * \ingroup ilmControl
 * \param surfaceId Id of the surface to set the opacity of.
 * \param opacity 0.0 means the surface is fully transparent,
 *                1.0 means the surface is fully opaque
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetOpacity(const t_ilm_surface surfaceId, t_ilm_float opacity);

/**
 * \brief Get the opacity of a surface.
 * \ingroup ilmControl
 * \param[in] surfaceId Id of the surface to get the opacity of.
 * \param[out] pOpacity pointer where the surface opacity should be stored.
 *                      0.0 means the surface is fully transparent,
 *                      1.0 means the surface is fully opaque
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceGetOpacity(const t_ilm_surface surfaceId, t_ilm_float *pOpacity);

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

/**
 * \brief Set the destination area of a surface within a layer for rendering. The surface will be scaled to this rectangle for rendering.
 * \ingroup ilmControl
 * \param[in] surfaceId Id of surface.
 * \param[in] x horizontal start position of the used area
 * \param[in] y vertical start position of the used area
 * \param[in] width width of the area
 * \param[in] height height of the area
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetDestinationRectangle(t_ilm_surface surfaceId, t_ilm_int x, t_ilm_int y, t_ilm_int width, t_ilm_int height);

/**
 * \brief Sets the orientation of a surface.
 * \ingroup ilmControl
 * \param[in] surfaceId Id of surface.
 * \param[in] orientation Orientation of the surface.
 * \note ilmOrientation for information about orientation values
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceSetOrientation(t_ilm_surface surfaceId, ilmOrientation orientation);

/**
 * \brief Gets the orientation of a surface.
 * \ingroup ilmControl
 * \param[in]  surfaceId Id of surface.
 * \param[out] pOrientation Address where orientation of the surface should be stored.
 * \note ilmOrientation for information about orientation values
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceGetOrientation(t_ilm_surface surfaceId, ilmOrientation *pOrientation);

/**
 * \brief Gets the pixelformat of a surface.
 * \ingroup ilmControl
 * \param[in] surfaceId Id of surface.
 * \param[out] pPixelformat Pointer where the pixelformat of the surface should be stored
 * \note ilmPixelFormat for information about pixel format values
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_surfaceGetPixelformat(t_ilm_layer surfaceId, ilmPixelFormat *pPixelformat);

/**
 * \brief Sets render order of layers on a display
 * \ingroup ilmControl
 * \param[in] display Id of display to set the given order of layers.
 * \param[in] pLayerId array of layer ids
 * \param[in] number number of layerids in the given array
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_displaySetRenderOrder(t_ilm_display display, t_ilm_layer *pLayerId, const t_ilm_uint number);

/**
 * \brief Take a screenshot from the current displayed layer scene.
 * The screenshot is saved as bmp file with the corresponding filename.
 * \ingroup ilmControl
 * \param[in] screen Id of screen where screenshot should be taken
 * \param[in] filename Location where the screenshot should be stored
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_takeScreenshot(t_ilm_uint screen, t_ilm_const_string filename);

/**
 * \brief Take a screenshot of a certain layer
 * The screenshot is saved as bmp file with the corresponding filename.
 * \ingroup ilmControl
 * \param[in] filename Location where the screenshot should be stored
 * \param[in] layerid Identifier of the layer to take the screenshot of
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_takeLayerScreenshot(t_ilm_const_string filename, t_ilm_layer layerid);

/**
 * \brief Take a screenshot of a certain surface
 * The screenshot is saved as bmp file with the corresponding filename.
 * \ingroup ilmControl
 * \param[in] filename Location where the screenshot should be stored
 * \param[in] surfaceid Identifier of the surface to take the screenshot of
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_takeSurfaceScreenshot(t_ilm_const_string filename, t_ilm_surface surfaceid);

/**
 * \brief register for notification on property changes of layer
 * \ingroup ilmControl
 * \param[in] layer id of layer to register for notification
 * \param[in] callback pointer to function to be called for notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given layer already has notification callback registered
 */
ilmErrorTypes ilm_layerAddNotification(t_ilm_layer layer, layerNotificationFunc callback);

/**
 * \brief remove notification on property changes of layer
 * \ingroup ilmControl
 * \param[in] layer id of layer to remove notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given layer has no notification callback registered
 */
ilmErrorTypes ilm_layerRemoveNotification(t_ilm_layer layer);

/**
 * \brief register for notification on property changes of surface
 * \ingroup ilmClient
 * \param[in] surface id of surface to register for notification
 * \param[in] callback pointer to function to be called for notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given surface already has notification callback registered
 */
ilmErrorTypes ilm_surfaceAddNotification(t_ilm_surface surface, surfaceNotificationFunc callback);

/**
 * \brief remove notification on property changes of surface
 * \ingroup ilmClient
 * \param[in] surface id of surface to remove notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given surface has no notification callback registered
 */
ilmErrorTypes ilm_surfaceRemoveNotification(t_ilm_surface surface);

/**
 * \brief register for notification on property creation/deletion events for surfaces/layers
 * \ingroup ilmControl
 * \param[in] surface id of surface to register for notification
 * \param[in] callback pointer to function to be called for notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given surface already has notification callback registered
 */
ilmErrorTypes ilm_registerNotification(notificationFunc callback, void *user_data);

/**
 * \brief remove notification on property changes of surface
 * \ingroup ilmClient
 * \param[in] surface id of surface to remove notification
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 * \return ILM_ERROR_INVALID_ARGUMENT if the given surface has no notification callback registered
 */
ilmErrorTypes ilm_unregisterNotification();
#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CONTROL_H_ */
