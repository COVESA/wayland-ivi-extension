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
#include "ilm_common.h"
#include "ilm_control_platform.h"

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

ILM_EXPORT ilmErrorTypes
ilmControl_init(t_ilm_nativedisplay nativedisplay)
{
    init_ilmControlPlatformTable();

    return gIlmControlPlatformFunc.init(nativedisplay);
}

ILM_EXPORT void
ilmControl_destroy(void)
{
    gIlmControlPlatformFunc.destroy();
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfLayer(t_ilm_uint layerID,
                         struct ilmLayerProperties* pLayerProperties)
{
    return gIlmControlPlatformFunc.getPropertiesOfLayer(
               layerID, pLayerProperties);
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfScreen(t_ilm_display screenID,
                          struct ilmScreenProperties* pScreenProperties)
{
    return gIlmControlPlatformFunc.getPropertiesOfScreen(
               screenID, pScreenProperties);
}

ILM_EXPORT ilmErrorTypes
ilm_getNumberOfHardwareLayers(t_ilm_uint screenID,
                              t_ilm_uint* pNumberOfHardwareLayers)
{
    return gIlmControlPlatformFunc.getNumberOfHardwareLayers(
               screenID, pNumberOfHardwareLayers);
}

ILM_EXPORT ilmErrorTypes
ilm_getScreenIDs(t_ilm_uint* pNumberOfIDs, t_ilm_uint** ppIDs)
{
    return gIlmControlPlatformFunc.getScreenIDs(pNumberOfIDs, ppIDs);
}

ILM_EXPORT ilmErrorTypes
ilm_getLayerIDs(t_ilm_int* pLength, t_ilm_layer** ppArray)
{
    return gIlmControlPlatformFunc.getLayerIDs(pLength, ppArray);
}

ILM_EXPORT ilmErrorTypes
ilm_getLayerIDsOnScreen(t_ilm_uint screenId, t_ilm_int* pLength,
                        t_ilm_layer** ppArray)
{
    return gIlmControlPlatformFunc.getLayerIDsOnScreen(
               screenId, pLength, ppArray);
}

ILM_EXPORT ilmErrorTypes
ilm_getSurfaceIDs(t_ilm_int* pLength, t_ilm_surface** ppArray)
{
    return gIlmControlPlatformFunc.getSurfaceIDs(pLength, ppArray);
}

ILM_EXPORT ilmErrorTypes
ilm_getSurfaceIDsOnLayer(t_ilm_layer layer, t_ilm_int* pLength,
                         t_ilm_surface** ppArray)
{
    return gIlmControlPlatformFunc.getSurfaceIDsOnLayer(
               layer, pLength, ppArray);
}

ILM_EXPORT ilmErrorTypes
ilm_layerCreateWithDimension(t_ilm_layer* pLayerId,
                             t_ilm_uint width, t_ilm_uint height)
{
    return gIlmControlPlatformFunc.layerCreateWithDimension(
               pLayerId, width, height);
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemove(t_ilm_layer layerId)
{
    return gIlmControlPlatformFunc.layerRemove(layerId);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetType(t_ilm_layer layerId, ilmLayerType* pLayerType)
{
    return gIlmControlPlatformFunc.layerGetType(layerId, pLayerType);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetVisibility(t_ilm_layer layerId, t_ilm_bool newVisibility)
{
    return gIlmControlPlatformFunc.layerSetVisibility(layerId, newVisibility);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetVisibility(t_ilm_layer layerId, t_ilm_bool *pVisibility)
{
    return gIlmControlPlatformFunc.layerGetVisibility(layerId, pVisibility);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetOpacity(t_ilm_layer layerId, t_ilm_float opacity)
{
    return gIlmControlPlatformFunc.layerSetOpacity(layerId, opacity);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetOpacity(t_ilm_layer layerId, t_ilm_float *pOpacity)
{
    return gIlmControlPlatformFunc.layerGetOpacity(layerId, pOpacity);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetSourceRectangle(t_ilm_layer layerId,
                            t_ilm_uint x, t_ilm_uint y,
                            t_ilm_uint width, t_ilm_uint height)
{
    return gIlmControlPlatformFunc.layerSetSourceRectangle(
               layerId, x, y, width, height);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetDestinationRectangle(t_ilm_layer layerId,
                                 t_ilm_int x, t_ilm_int y,
                                 t_ilm_int width, t_ilm_int height)
{
    return gIlmControlPlatformFunc.layerSetDestinationRectangle(
               layerId, x, y, width, height);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetDimension(t_ilm_layer layerId, t_ilm_uint *pDimension)
{
    return gIlmControlPlatformFunc.layerGetDimension(layerId, pDimension);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetDimension(t_ilm_layer layerId, t_ilm_uint *pDimension)
{
    return gIlmControlPlatformFunc.layerSetDimension(layerId, pDimension);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetPosition(t_ilm_layer layerId, t_ilm_uint *pPosition)
{
    return gIlmControlPlatformFunc.layerGetPosition(layerId, pPosition);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetPosition(t_ilm_layer layerId, t_ilm_uint *pPosition)
{
    return gIlmControlPlatformFunc.layerSetPosition(layerId, pPosition);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetOrientation(t_ilm_layer layerId, ilmOrientation orientation)
{
    return gIlmControlPlatformFunc.layerSetOrientation(layerId, orientation);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetOrientation(t_ilm_layer layerId, ilmOrientation *pOrientation)
{
    return gIlmControlPlatformFunc.layerGetOrientation(layerId, pOrientation);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetChromaKey(t_ilm_layer layerId, t_ilm_int* pColor)
{
    return gIlmControlPlatformFunc.layerSetChromaKey(layerId, pColor);
}

ILM_EXPORT ilmErrorTypes
ilm_layerSetRenderOrder(t_ilm_layer layerId,
                        t_ilm_layer *pSurfaceId,
                        t_ilm_int number)
{
    return gIlmControlPlatformFunc.layerSetRenderOrder(
               layerId, pSurfaceId, number);
}

ILM_EXPORT ilmErrorTypes
ilm_layerGetCapabilities(t_ilm_layer layerId,
                         t_ilm_layercapabilities *pCapabilities)
{
    return gIlmControlPlatformFunc.layerGetCapabilities(layerId, pCapabilities);
}

ILM_EXPORT ilmErrorTypes
ilm_layerTypeGetCapabilities(ilmLayerType layerType,
                             t_ilm_layercapabilities *pCapabilities)
{
    return gIlmControlPlatformFunc.layerTypeGetCapabilities(
               layerType, pCapabilities);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetVisibility(t_ilm_surface surfaceId, t_ilm_bool newVisibility)
{
    return gIlmControlPlatformFunc.surfaceSetVisibility(
               surfaceId, newVisibility);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetOpacity(t_ilm_surface surfaceId, t_ilm_float opacity)
{
    return gIlmControlPlatformFunc.surfaceSetOpacity(surfaceId, opacity);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetOpacity(t_ilm_surface surfaceId, t_ilm_float *pOpacity)
{
    return gIlmControlPlatformFunc.surfaceGetOpacity(surfaceId, pOpacity);
}

ILM_EXPORT ilmErrorTypes
ilm_SetKeyboardFocusOn(t_ilm_surface surfaceId)
{
    return gIlmControlPlatformFunc.SetKeyboardFocusOn(surfaceId);
}

ILM_EXPORT ilmErrorTypes
ilm_GetKeyboardFocusSurfaceId(t_ilm_surface* pSurfaceId)
{
    return gIlmControlPlatformFunc.GetKeyboardFocusSurfaceId(pSurfaceId);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetDestinationRectangle(t_ilm_surface surfaceId,
                                   t_ilm_int x, t_ilm_int y,
                                   t_ilm_int width, t_ilm_int height)
{
    return gIlmControlPlatformFunc.surfaceSetDestinationRectangle(
               surfaceId, x, y, width, height);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetDimension(t_ilm_surface surfaceId, t_ilm_uint *pDimension)
{
    return gIlmControlPlatformFunc.surfaceSetDimension(surfaceId, pDimension);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetPosition(t_ilm_surface surfaceId, t_ilm_uint *pPosition)
{
    return gIlmControlPlatformFunc.surfaceGetPosition(surfaceId, pPosition);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetPosition(t_ilm_surface surfaceId, t_ilm_uint *pPosition)
{
    return gIlmControlPlatformFunc.surfaceSetPosition(surfaceId, pPosition);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetOrientation(t_ilm_surface surfaceId, ilmOrientation orientation)
{
    return gIlmControlPlatformFunc.surfaceSetOrientation(
               surfaceId, orientation);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetOrientation(t_ilm_surface surfaceId, ilmOrientation *pOrientation)
{
    return gIlmControlPlatformFunc.surfaceGetOrientation(
               surfaceId, pOrientation);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetPixelformat(t_ilm_layer surfaceId, ilmPixelFormat *pPixelformat)
{
    return gIlmControlPlatformFunc.surfaceGetPixelformat(
               surfaceId, pPixelformat);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetChromaKey(t_ilm_surface surfaceId, t_ilm_int* pColor)
{
    return gIlmControlPlatformFunc.surfaceSetChromaKey(surfaceId, pColor);
}

ILM_EXPORT ilmErrorTypes
ilm_displaySetRenderOrder(t_ilm_display display,
                          t_ilm_layer *pLayerId, const t_ilm_uint number)
{
    return gIlmControlPlatformFunc.displaySetRenderOrder(
               display, pLayerId, number);
}

ILM_EXPORT ilmErrorTypes
ilm_takeScreenshot(t_ilm_uint screen, t_ilm_const_string filename)
{
    return gIlmControlPlatformFunc.takeScreenshot(screen, filename);
}

ILM_EXPORT ilmErrorTypes
ilm_takeLayerScreenshot(t_ilm_const_string filename, t_ilm_layer layerid)
{
    return gIlmControlPlatformFunc.takeLayerScreenshot(filename, layerid);
}

ILM_EXPORT ilmErrorTypes
ilm_takeSurfaceScreenshot(t_ilm_const_string filename, t_ilm_surface surfaceid)
{
    return gIlmControlPlatformFunc.takeSurfaceScreenshot(filename, surfaceid);
}

ILM_EXPORT ilmErrorTypes
ilm_SetOptimizationMode(ilmOptimization id, ilmOptimizationMode mode)
{
    return gIlmControlPlatformFunc.SetOptimizationMode(id, mode);
}

ILM_EXPORT ilmErrorTypes
ilm_GetOptimizationMode(ilmOptimization id, ilmOptimizationMode* pMode)
{
    return gIlmControlPlatformFunc.GetOptimizationMode(id, pMode);
}

ILM_EXPORT ilmErrorTypes
ilm_layerAddNotification(t_ilm_layer layer, layerNotificationFunc callback)
{
    return gIlmControlPlatformFunc.layerAddNotification(layer, callback);
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveNotification(t_ilm_layer layer)
{
    return gIlmControlPlatformFunc.layerRemoveNotification(layer);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceAddNotification(t_ilm_surface surface, surfaceNotificationFunc callback)
{
    return gIlmControlPlatformFunc.surfaceAddNotification(surface, callback);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceRemoveNotification(t_ilm_surface surface)
{
    return gIlmControlPlatformFunc.surfaceRemoveNotification(surface);
}

ILM_EXPORT ilmErrorTypes
ilm_getNativeHandle(t_ilm_uint pid, t_ilm_const_char *p_window_title,
                    t_ilm_int *p_handle, t_ilm_nativehandle **p_handles)
{
    return gIlmControlPlatformFunc.getNativeHandle(pid, p_handle, p_handles);
}

ILM_EXPORT ilmErrorTypes
ilm_getPropertiesOfSurface(t_ilm_uint surfaceID,
                           struct ilmSurfaceProperties* pSurfaceProperties)
{
    return gIlmControlPlatformFunc.getPropertiesOfSurface(
               surfaceID, pSurfaceProperties);
}

ILM_EXPORT ilmErrorTypes
ilm_layerAddSurface(t_ilm_layer layerId, t_ilm_surface surfaceId)
{
    return gIlmControlPlatformFunc.layerAddSurface(layerId, surfaceId);
}

ILM_EXPORT ilmErrorTypes
ilm_layerRemoveSurface(t_ilm_layer layerId, t_ilm_surface surfaceId)
{
    return gIlmControlPlatformFunc.layerRemoveSurface(layerId, surfaceId);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetDimension(t_ilm_surface surfaceId, t_ilm_uint *pDimension)
{
    return gIlmControlPlatformFunc.surfaceGetDimension(surfaceId, pDimension);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceGetVisibility(t_ilm_surface surfaceId, t_ilm_bool *pVisibility)
{
    return gIlmControlPlatformFunc.surfaceGetVisibility(surfaceId, pVisibility);
}

ILM_EXPORT ilmErrorTypes
ilm_surfaceSetSourceRectangle(t_ilm_surface surfaceId,
                              t_ilm_int x, t_ilm_int y,
                              t_ilm_int width, t_ilm_int height)
{
    return gIlmControlPlatformFunc.surfaceSetSourceRectangle(
               surfaceId, x, y, width, height);
}

ILM_EXPORT ilmErrorTypes
ilm_commitChanges(void)
{
    return gIlmControlPlatformFunc.commitChanges();
}
