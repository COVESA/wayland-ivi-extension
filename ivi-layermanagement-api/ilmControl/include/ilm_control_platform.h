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
#ifndef _ILM_CONTROL_PLATFORM_H_
#define _ILM_CONTROL_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ilm_common.h"

typedef struct _ILM_CONTROL_PLATFORM_FUNC
{
    ilmErrorTypes (*getPropertiesOfLayer)(t_ilm_uint layerID,
                   struct ilmLayerProperties* pLayerProperties);
    ilmErrorTypes (*getPropertiesOfScreen)(t_ilm_display screenID,
                   struct ilmScreenProperties* pScreenProperties);
    ilmErrorTypes (*getNumberOfHardwareLayers)(t_ilm_uint screenID,
                   t_ilm_uint* pNumberOfHardwareLayers);
    ilmErrorTypes (*getScreenIDs)(t_ilm_uint* pNumberOfIDs,
                   t_ilm_uint** ppIDs);
    ilmErrorTypes (*getLayerIDs)(t_ilm_int* pLength,
                   t_ilm_layer** ppArray);
    ilmErrorTypes (*getLayerIDsOnScreen)(t_ilm_uint screenId,
                   t_ilm_int* pLength, t_ilm_layer** ppArray);
    ilmErrorTypes (*getSurfaceIDs)(t_ilm_int* pLength,
                   t_ilm_surface** ppArray);
    ilmErrorTypes (*getSurfaceIDsOnLayer)(t_ilm_layer layer,
                   t_ilm_int* pLength, t_ilm_surface** ppArray);
    ilmErrorTypes (*layerCreateWithDimension)(t_ilm_layer* pLayerId,
                   t_ilm_uint width, t_ilm_uint height);
    ilmErrorTypes (*layerRemove)(t_ilm_layer layerId);
    ilmErrorTypes (*layerGetType)(t_ilm_layer layerId,
                   ilmLayerType* pLayerType);
    ilmErrorTypes (*layerSetVisibility)(t_ilm_layer layerId,
                   t_ilm_bool newVisibility);
    ilmErrorTypes (*layerGetVisibility)(t_ilm_layer layerId,
                   t_ilm_bool *pVisibility);
    ilmErrorTypes (*layerSetOpacity)(t_ilm_layer layerId,
                   t_ilm_float opacity);
    ilmErrorTypes (*layerGetOpacity)(t_ilm_layer layerId,
                   t_ilm_float *pOpacity);
    ilmErrorTypes (*layerSetSourceRectangle)(t_ilm_layer layerId,
                   t_ilm_uint x, t_ilm_uint y,
                   t_ilm_uint width, t_ilm_uint height);
    ilmErrorTypes (*layerSetDestinationRectangle)(t_ilm_layer layerId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*layerGetDimension)(t_ilm_layer layerId,
                   t_ilm_uint *pDimension);
    ilmErrorTypes (*layerSetDimension)(t_ilm_layer layerId,
                   t_ilm_uint *pDimension);
    ilmErrorTypes (*layerGetPosition)(t_ilm_layer layerId,
                   t_ilm_uint *pPosition);
    ilmErrorTypes (*layerSetPosition)(t_ilm_layer layerId,
                   t_ilm_uint *pPosition);
    ilmErrorTypes (*layerSetOrientation)(t_ilm_layer layerId,
                   ilmOrientation orientation);
    ilmErrorTypes (*layerGetOrientation)(t_ilm_layer layerId,
                   ilmOrientation *pOrientation);
    ilmErrorTypes (*layerSetChromaKey)(t_ilm_layer layerId,
                   t_ilm_int* pColor);
    ilmErrorTypes (*layerSetRenderOrder)(t_ilm_layer layerId,
                   t_ilm_layer *pSurfaceId, t_ilm_int number);
    ilmErrorTypes (*layerTypeGetCapabilities)(ilmLayerType layerType,
                   t_ilm_layercapabilities *pCapabilities);
    ilmErrorTypes (*surfaceSetVisibility)(t_ilm_surface surfaceId,
                   t_ilm_bool newVisibility);
    ilmErrorTypes (*surfaceSetOpacity)(t_ilm_surface surfaceId,
                   t_ilm_float opacity);
    ilmErrorTypes (*surfaceGetOpacity)(t_ilm_surface surfaceId,
                   t_ilm_float *pOpacity);
    ilmErrorTypes (*SetKeyboardFocusOn)(t_ilm_surface surfaceId);
    ilmErrorTypes (*GetKeyboardFocusSurfaceId)(t_ilm_surface* pSurfaceId);
    ilmErrorTypes (*surfaceSetDestinationRectangle)(t_ilm_surface surfaceId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*surfaceSetDimension)(t_ilm_surface surfaceId,
                   t_ilm_uint *pDimension);
    ilmErrorTypes (*surfaceGetPosition)(t_ilm_surface surfaceId,
                   t_ilm_uint *pPosition);
    ilmErrorTypes (*surfaceSetPosition)(t_ilm_surface surfaceId,
                   t_ilm_uint *pPosition);
    ilmErrorTypes (*surfaceSetOrientation)(t_ilm_surface surfaceId,
                   ilmOrientation orientation);
    ilmErrorTypes (*surfaceGetOrientation)(t_ilm_surface surfaceId,
                   ilmOrientation *pOrientation);
    ilmErrorTypes (*surfaceGetPixelformat)(t_ilm_layer surfaceId,
                   ilmPixelFormat *pPixelformat);
    ilmErrorTypes (*surfaceSetChromaKey)(t_ilm_surface surfaceId,
                   t_ilm_int* pColor);
    ilmErrorTypes (*displaySetRenderOrder)(t_ilm_display display,
                   t_ilm_layer *pLayerId, const t_ilm_uint number);
    ilmErrorTypes (*takeScreenshot)(t_ilm_uint screen,
                   t_ilm_const_string filename);
    ilmErrorTypes (*takeLayerScreenshot)(t_ilm_const_string filename,
                   t_ilm_layer layerid);
    ilmErrorTypes (*takeSurfaceScreenshot)(t_ilm_const_string filename,
                   t_ilm_surface surfaceid);
    ilmErrorTypes (*SetOptimizationMode)(ilmOptimization id,
                   ilmOptimizationMode mode);
    ilmErrorTypes (*GetOptimizationMode)(ilmOptimization id,
                   ilmOptimizationMode* pMode);
    ilmErrorTypes (*layerAddNotification)(t_ilm_layer layer,
                   layerNotificationFunc callback);
    ilmErrorTypes (*layerRemoveNotification)(t_ilm_layer layer);
    ilmErrorTypes (*surfaceAddNotification)(t_ilm_surface surface,
                   surfaceNotificationFunc callback);
    ilmErrorTypes (*surfaceRemoveNotification)(t_ilm_surface surface);
    ilmErrorTypes (*init)(t_ilm_nativedisplay nativedisplay);
    void (*destroy)();
    ilmErrorTypes (*getNativeHandle)(t_ilm_uint pid, t_ilm_int *p_handle,
                   t_ilm_nativehandle **p_handles);
    ilmErrorTypes (*getPropertiesOfSurface)(t_ilm_uint surfaceID,
                   struct ilmSurfaceProperties* pSurfaceProperties);
    ilmErrorTypes (*layerAddSurface)(t_ilm_layer layerId,
                   t_ilm_surface surfaceId);
    ilmErrorTypes (*layerRemoveSurface)(t_ilm_layer layerId,
                   t_ilm_surface surfaceId);
    ilmErrorTypes (*surfaceGetDimension)(t_ilm_surface surfaceId,
                   t_ilm_uint *pDimension);
    ilmErrorTypes (*surfaceGetVisibility)(t_ilm_surface surfaceId,
                   t_ilm_bool *pVisibility);
    ilmErrorTypes (*surfaceSetSourceRectangle)(t_ilm_surface surfaceId,
                   t_ilm_int x, t_ilm_int y,
                   t_ilm_int width, t_ilm_int height);
    ilmErrorTypes (*commitChanges)();
} ILM_CONTROL_PLATFORM_FUNC;

ILM_CONTROL_PLATFORM_FUNC gIlmControlPlatformFunc;

void init_ilmControlPlatformTable();

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_CONTROL_PLATFORM_H_ */
