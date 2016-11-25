/***************************************************************************
 *
 * Copyright 2012 BMW Car IT GmbH
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

#include "ilm_control.h"
#include "LMControl.h"

#include <algorithm>
using std::find;

#include <cmath>
using std::max;
using std::min;

#include <iterator>
using std::iterator;

#include <iostream>
using std::cout;
using std::cin;
using std::endl;

#include <vector>
using std::vector;

#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>


tuple4 getSurfaceScreenCoordinates(ilmSurfaceProperties targetSurfaceProperties, ilmLayerProperties targetLayerProperties)
{
    t_ilm_float horizontalScale = targetLayerProperties.sourceWidth ?
                    1.0 * targetLayerProperties.destWidth / targetLayerProperties.sourceWidth : 0;

    t_ilm_float targetX1 = targetLayerProperties.destX + horizontalScale
            * (targetSurfaceProperties.destX - targetLayerProperties.sourceX);
    t_ilm_float targetX2 = targetX1 + horizontalScale * targetSurfaceProperties.destWidth;

    t_ilm_float verticalScale = targetLayerProperties.sourceHeight ?
            1.0 * targetLayerProperties.destHeight / targetLayerProperties.sourceHeight : 0;

    t_ilm_float targetY1 = targetLayerProperties.destY + verticalScale
            * (targetSurfaceProperties.destY - targetLayerProperties.sourceY);
    t_ilm_float targetY2 = targetY1 + verticalScale * targetSurfaceProperties.destHeight;

    tuple4 targetSurfaceCoordinates(static_cast<t_ilm_int>(targetX1),
            static_cast<t_ilm_int>(targetY1),
            max(0, static_cast<t_ilm_int>(targetX2) - 1),
            max(0, static_cast<t_ilm_int>(targetY2) - 1));

    return targetSurfaceCoordinates;
}

tuple4 getSurfaceScreenCoordinates(t_scene_data* pScene, t_ilm_surface surface)
{
    tuple4 surfaceCoordinates;

    //if surface belongs to a layer make it appear exacrly as it would appear on its current layer
    if (pScene->surfaceLayer.find(surface) != pScene->surfaceLayer.end())
    {
        //set dimensions of the surface to map to the extra layer according to its current placement in the
        t_ilm_layer layer = pScene->surfaceLayer[surface];
        ilmLayerProperties layerProperties = pScene->layerProperties[layer];
        ilmSurfaceProperties surfaceProperties = pScene->surfaceProperties[surface];

        surfaceCoordinates = getSurfaceScreenCoordinates(surfaceProperties, layerProperties);
    }
    //if surface does not belong to a layer just assume it belongs to a layer that fills the screen
    else
    {
        ilmSurfaceProperties surfaceProperties = pScene->surfaceProperties[surface];

        surfaceCoordinates.x = surfaceProperties.destX;
        surfaceCoordinates.y = surfaceProperties.destY;
        surfaceCoordinates.z = surfaceProperties.destX + surfaceProperties.destWidth;
        surfaceCoordinates.w = surfaceProperties.destX + surfaceProperties.destHeight;
    }

    return surfaceCoordinates;
}

vector<t_ilm_surface> getSceneRenderOrder(t_scene_data* pScene)
{
    t_scene_data& scene = *pScene;
    vector<t_ilm_surface> renderOrder;

    //iterate over screens
    for (vector<t_ilm_display>::iterator it = scene.screens.begin(); it != scene.screens.end(); ++it)
    {
        t_ilm_display screen = *it;
        vector<t_ilm_layer> layers = scene.screenLayers[screen];

        //iterate over layers
        for (vector<t_ilm_layer>::iterator layerIterator = layers.begin();
                layerIterator != layers.end(); ++layerIterator)
        {
            t_ilm_layer layer = (*layerIterator);
            vector<t_ilm_surface> surfaces = scene.layerSurfaces[layer];

            //iterate over surfaces
            for (vector<t_ilm_surface>::iterator it = surfaces.begin(); it != surfaces.end(); ++it)
            {
                t_ilm_surface surface = *it;

                renderOrder.push_back(surface);
            }
        }
    }

    return renderOrder;
}

void captureSceneData(t_scene_data* pScene)
{
    t_scene_data& scene = *pScene;

    //get screen information
    t_ilm_uint screenWidth = 0;
    t_ilm_uint screenHeight = 0;

    ilmErrorTypes callResult = ilm_getScreenResolution(0, &screenWidth, &screenHeight);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get screen resolution for screen with ID " << 0 << "\n";
        return;
    }

    scene.screenWidth = screenWidth;
    scene.screenHeight = screenHeight;

    //extra layer for debugging
    scene.extraLayer = 0xFFFFFFFF;

    //get screens
    unsigned int screenCount = 0;
    t_ilm_display* screenArray = NULL;

    callResult = ilm_getScreenIDs(&screenCount, &screenArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available screen IDs\n";
        return;
    }

    scene.screens = vector<t_ilm_display>(screenArray, screenArray + screenCount);

    //layers on each screen
    for (unsigned int i = 0; i < screenCount; ++i)
    {
        t_ilm_display screenId = screenArray[i];

        t_ilm_int layerCount = 0;
        t_ilm_layer* layerArray = NULL;

        callResult = ilm_getLayerIDsOnScreen(screenId, &layerCount, &layerArray);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get layers on screen with ID " << screenId << "\n";
            return;
        }

        scene.screenLayers[screenId] = vector<t_ilm_layer>(layerArray, layerArray + layerCount);

        //preserve rendering order for layers on each screen
        for (int j = 0; j < layerCount; ++j)
        {
            t_ilm_layer layerId = layerArray[j];

            scene.layerScreen[layerId] = screenId;
        }
    }

    //get all layers (rendered and not rendered)
    t_ilm_int layerCount = 0;
    t_ilm_layer* layerArray = NULL;
    ilm_getLayerIDs(&layerCount, &layerArray);
    scene.layers = vector<t_ilm_layer>(layerArray, layerArray + layerCount);

    for (int j = 0; j < layerCount; ++j)
    {
        t_ilm_layer layerId = layerArray[j];

        //layer properties
        ilmLayerProperties lp;

        callResult = ilm_getPropertiesOfLayer(layerId, &lp);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get properties of layer with ID " << layerId << "\n";
            return;
        }

        scene.layerProperties[layerId] = lp;
    }

    //surfaces on each layer
    for (int j = 0; j < layerCount; ++j)
    {
        t_ilm_layer layerId = layerArray[j];

        //surfaces on layer (in rendering order)
        int surfaceCount = 0;
        t_ilm_surface* surfaceArray = NULL;

        callResult = ilm_getSurfaceIDsOnLayer(layerId, &surfaceCount, &surfaceArray);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get surfaces on layer with ID " << layerId << "\n";
            return;
        }

        //rendering order on layer
        scene.layerSurfaces[layerId] = vector<t_ilm_surface>(surfaceArray, surfaceArray + surfaceCount);

        //make each surface aware of its layer
        for (int k = 0; k < surfaceCount; ++k)
        {
            t_ilm_surface surfaceId = surfaceArray[k];
            scene.surfaceLayer[surfaceId] = layerId;
        }
    }

    //get all surfaces (on layers and without layers)
    t_ilm_int surfaceCount = 0;
    t_ilm_surface* surfaceArray = NULL;

    callResult = ilm_getSurfaceIDs(&surfaceCount, &surfaceArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available surfaces\n";
        return;
    }

    scene.surfaces = vector<t_ilm_surface>(surfaceArray, surfaceArray + surfaceCount);

    for (int k = 0; k < surfaceCount; ++k)
    {
        t_ilm_surface surfaceId = surfaceArray[k];

        //surface properties
        ilmSurfaceProperties sp;

        callResult = ilm_getPropertiesOfSurface(surfaceId, &sp);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get properties of surface with ID " << surfaceId << "\n";
            return;
        }

        scene.surfaceProperties[surfaceId] = sp;
    }
}
