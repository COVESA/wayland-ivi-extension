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

#include "ilm_client.h"
#include "LMControl.h"

#include <algorithm>
using std::find;

#include <cstdio>

#include <iterator>
using std::iterator;

#include <iostream>
using std::cout;
using std::cin;
using std::endl;

#include <iomanip>
using std::dec;
using std::hex;
using std::left;
using std::right;
using std::setw;

#include <cmath>
using std::max;
using std::min;

#include <string>
using std::string;

#include <vector>
using std::vector;

namespace
{
void analyzePrintHelper(string tag, string flag, string description)
{
    cout << left << setw(25) << tag << " | " << setw(7) << flag << " | " << description << endl;
}

void analyzeVisibilityAndOpacity(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    t_ilm_layer targetSurfaceLayer = scene.surfaceLayer[targetSurfaceId];
    ilmSurfaceProperties& targetSurfaceProperties = scene.surfaceProperties[targetSurfaceId];
    ilmLayerProperties& targetLayerProperties = scene.layerProperties[targetSurfaceLayer];
    string tag;
    string flag;
    char description[300] = "";

    //check visibility
    tag = "Surface Visibility";
    if (targetSurfaceProperties.visibility == ILM_FALSE)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i visibility set to false", targetSurfaceId);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer Visibility";
    if (targetLayerProperties.visibility == ILM_FALSE)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i visibility set to false.", targetSurfaceLayer);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    //check opacity
    tag = "Surface Opacity";
    if (targetSurfaceProperties.opacity <= 0.2)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i opacity set to %f, it is (almost) invisible", targetSurfaceId, targetSurfaceProperties.opacity);
    }
    else if (targetSurfaceProperties.opacity < 1.0)
    {
        flag = "WARNING";
        sprintf(description, "Surface %i opacity set to %f, it might not be easy to see", targetSurfaceId, targetSurfaceProperties.opacity);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer Opacity";
    if (targetLayerProperties.opacity <= 0.2)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i opacity set to %f, it is (almost) invisible", targetSurfaceLayer, targetLayerProperties.opacity);
    }
    else if (targetLayerProperties.opacity < 1.0)
    {
        flag = "WARNING";
        sprintf(description, "Layer %i opacity set to %f, it might not be easy to see", targetSurfaceLayer, targetLayerProperties.opacity);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);
}

void analyzeSurfaceDimensions(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    ilmSurfaceProperties& targetSurfaceProperties = scene.surfaceProperties[targetSurfaceId];
    string tag;
    string flag;
    char description[300] = "";

    t_ilm_uint minDimension = 32;

    tag = "Surface dest width";
    if (targetSurfaceProperties.destWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [destWidth=%i]", targetSurfaceId, targetSurfaceProperties.destWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Surface source width";
    if (targetSurfaceProperties.sourceWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [sourceWidth=%i]", targetSurfaceId, targetSurfaceProperties.sourceWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Surface original width";
    if (targetSurfaceProperties.origSourceWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [origSourceWidth=%i]", targetSurfaceId, targetSurfaceProperties.origSourceWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Surface dest height";
    if (targetSurfaceProperties.destHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [destHeight=%i]", targetSurfaceId, targetSurfaceProperties.destHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Surface source height";
    if (targetSurfaceProperties.sourceHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [sourceHeight=%i]", targetSurfaceId, targetSurfaceProperties.sourceHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Surface original height";
    if (targetSurfaceProperties.origSourceHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i has [origSourceHeight=%i]", targetSurfaceId, targetSurfaceProperties.origSourceHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);
}

void analyzeLayerDimensions(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    t_ilm_layer targetSurfaceLayer = scene.surfaceLayer[targetSurfaceId];
    ilmLayerProperties& targetLayerProperties = scene.layerProperties[targetSurfaceLayer];
    t_ilm_uint minDimension = 32;

    string tag;
    string flag;
    char description[300] = "";
    tag = "Layer dest width";
    if (targetLayerProperties.destWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [destWidth=%i]", targetSurfaceLayer, targetLayerProperties.destWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer source width";
    if (targetLayerProperties.sourceWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [sourceWidth=%i]", targetSurfaceLayer, targetLayerProperties.sourceWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer original width";
    if (targetLayerProperties.origSourceWidth <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [origSourceWidth=%i]", targetSurfaceLayer, targetLayerProperties.origSourceWidth);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer dest height";
    if (targetLayerProperties.destHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [destHeight=%i]", targetSurfaceLayer, targetLayerProperties.destHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer source height";
    if (targetLayerProperties.sourceHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [sourceHeight=%i]", targetSurfaceLayer, targetLayerProperties.sourceHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    tag = "Layer original source";
    if (targetLayerProperties.origSourceHeight <= minDimension)
    {
        flag = "PROBLEM";
        sprintf(description, "Layer %i has [origSourceHeight=%i]", targetSurfaceLayer, targetLayerProperties.origSourceHeight);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);
}

void analyzeDimensions(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    analyzeSurfaceDimensions(targetSurfaceId, scene);
    analyzeLayerDimensions(targetSurfaceId, scene);
}

void analyzeSurfaceCheckInsideLayer(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    t_ilm_layer targetSurfaceLayer = scene.surfaceLayer[targetSurfaceId];
    tuple4 targetSurfaceCoordinates = getSurfaceScreenCoordinates(&scene, targetSurfaceId);
    ilmLayerProperties& targetLayerProperties = scene.layerProperties[targetSurfaceLayer];
    string tag;
    string flag;
    char description[300] = "";

    tuple4 layerCoordinates(targetLayerProperties.destX,
            targetLayerProperties.destY,
            targetLayerProperties.destX + targetLayerProperties.destWidth,
            targetLayerProperties.destY + targetLayerProperties.destHeight);

    tag = "Surface inside Layer";
    if (!inside(targetSurfaceCoordinates, layerCoordinates))
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i is not viewed completely insde the destination region of layer %i",
                targetSurfaceId, targetSurfaceLayer);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);
}

void analyzeOcclusion(t_ilm_surface targetSurfaceId,
        map<t_ilm_surface, t_ilm_layer>& surfaceLayers,
        map<t_ilm_surface, ilmSurfaceProperties>& surfaceProperties,
        map<t_ilm_layer, ilmLayerProperties>& layerProperties,
        vector<t_ilm_surface>& allSurfaces, tuple4 targetSurfaceCoordinates)
{
    string tag;
    string flag;
    char description[300] = "";

    vector<t_ilm_surface> occludingSurfaces;

    vector<t_ilm_surface>::iterator it = find(allSurfaces.begin(), allSurfaces.end(), targetSurfaceId);

    t_ilm_bool occluded = ILM_FALSE;
    tag = "Occlusion";
    ++it;
    for (; it != allSurfaces.end(); ++it)
    {
        t_ilm_surface surfaceId = *it;
        t_ilm_layer surfaceLayer = surfaceLayers[surfaceId];

        //if surface or layer invisible: neglect
        if (layerProperties[surfaceLayer].visibility == ILM_FALSE || surfaceProperties[surfaceId].visibility == ILM_FALSE)
            continue;

        //if multiplication of their opacity is zero: neglect
        if (layerProperties[surfaceLayer].opacity * surfaceProperties[surfaceId].opacity == 0)
            continue;

        //coordinates of the surface on screen
        t_ilm_int horizontalScale = layerProperties[surfaceLayer].destWidth / layerProperties[surfaceLayer].sourceWidth;
        t_ilm_int surfaceX1 = layerProperties[surfaceLayer].destX + horizontalScale
                * (surfaceProperties[surfaceId].destX - layerProperties[surfaceLayer].sourceX);
        t_ilm_int surfaceX2 = surfaceX1 + horizontalScale * surfaceProperties[surfaceId].destWidth;

        t_ilm_int verticalScale = layerProperties[surfaceLayer].destHeight / layerProperties[surfaceLayer].sourceHeight;
        t_ilm_int surfaceY1 = layerProperties[surfaceLayer].destY + verticalScale
                * (surfaceProperties[surfaceId].destY - layerProperties[surfaceLayer].sourceY);
        t_ilm_int surfaceY2 = surfaceY1 + verticalScale * surfaceProperties[surfaceId].destHeight;

        tuple4 surfaceCoordinates(surfaceX1, surfaceY1, surfaceX2, surfaceY2);

        //if the surface is completely occluded
        if (inside(targetSurfaceCoordinates, surfaceCoordinates))
        {
            flag = "PROBLEM";
            sprintf(description, "Surface %i is completely occluded by surface %i", targetSurfaceId, surfaceId);
            analyzePrintHelper(tag, flag, description);

            occluded = ILM_TRUE;
        }
        //if the surface is partially occluded
        else if (intersect(targetSurfaceCoordinates, surfaceCoordinates))
        {
            flag = "PROBLEM";
            sprintf(description, "Surface %i is partially occluded by surface %i", targetSurfaceId, surfaceId);
            analyzePrintHelper(tag, flag, description);
            occluded = ILM_TRUE;
        }
    }

    if (!occluded)
    {
        flag = "OK";
        sprintf(description, "%s", "");
        analyzePrintHelper(tag, flag, description);
    }
}

void analyzeOcclusion(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    string tag;
    string flag;
    char description[300] = "";

    vector<t_ilm_surface> renderedSurfaces = getSceneRenderOrder(&scene);
    vector<t_ilm_surface> occludingSurfaces;

    vector<t_ilm_surface>::iterator it = find(renderedSurfaces.begin(), renderedSurfaces.end(), targetSurfaceId);

    tuple4 targetSurfaceCoordinates = getSurfaceScreenCoordinates(&scene, targetSurfaceId);

    t_ilm_bool occluded = ILM_FALSE;
    tag = "Occlusion";
    ++it;
    for (; it != renderedSurfaces.end(); ++it)
    {
        t_ilm_surface surfaceId = *it;
        t_ilm_layer surfaceLayer = scene.surfaceLayer[surfaceId];

        //if surface or layer invisible: neglect
        if (scene.layerProperties[surfaceLayer].visibility == ILM_FALSE || scene.surfaceProperties[surfaceId].visibility == ILM_FALSE)
            continue;

        //if multiplication of their opacity is zero: neglect
        if (scene.layerProperties[surfaceLayer].opacity * scene.surfaceProperties[surfaceId].opacity == 0)
            continue;

        //coordinates of the surface on screen
        tuple4 surfaceCoordinates = getSurfaceScreenCoordinates(&scene, surfaceId);

        //if the surface is completely occluded
        if (inside(targetSurfaceCoordinates, surfaceCoordinates))
        {
            flag = "PROBLEM";
            sprintf(description, "Surface %i is completely occluded by surface %i", targetSurfaceId, surfaceId);
            analyzePrintHelper(tag, flag, description);

            occluded = ILM_TRUE;
        }
        //if the surface is partially occluded
        else if (intersect(targetSurfaceCoordinates, surfaceCoordinates))
        {
            flag = "PROBLEM";
            sprintf(description, "Surface %i is partially occluded by surface %i", targetSurfaceId, surfaceId);
            analyzePrintHelper(tag, flag, description);
            occluded = ILM_TRUE;
        }
    }

    if (!occluded)
    {
        flag = "OK";
        sprintf(description, "%s", "");
        analyzePrintHelper(tag, flag, description);
    }
}

t_ilm_bool analyzeCheckSurfaceExists(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    t_ilm_bool exists = ILM_FALSE;

    string tag;
    string flag;
    char description[300] = "";

    tag = "Surface existance";
    //check if surface exists
    if (find(scene.surfaces.begin(), scene.surfaces.end(), targetSurfaceId)
            == scene.surfaces.end())
    {
        flag = "PROBLEM";
        sprintf(description, "There is no surface with ID %i", targetSurfaceId);
    }
    else
    {
        exists = ILM_TRUE;
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    return exists;
}

t_ilm_bool analyzeCheckRendered(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    t_ilm_bool onLayer = ILM_FALSE;
    t_ilm_bool layerOnScreen = ILM_FALSE;
    //is surface on layer?
    map<t_ilm_surface, t_ilm_layer>::iterator surfaceLayerIt = scene.surfaceLayer.find(targetSurfaceId);

    if (surfaceLayerIt != scene.surfaceLayer.end())
    {
        onLayer = ILM_TRUE;
        t_ilm_layer layer = (*surfaceLayerIt).second;

        //is layer on screen?
        layerOnScreen = scene.layerScreen.find(layer) != scene.layerScreen.end();
    }

    //output
    string tag;
    string flag;
    char description[300] = "";

    tag = "Surface on layer";
    if (!onLayer)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i is not on any layer", targetSurfaceId);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    if (onLayer)
    {
        tag = "Layer on screen";
        if (!layerOnScreen)
        {
            flag = "PROBLEM";
            sprintf(description, "Layer %i is not on any screen", scene.surfaceLayer[targetSurfaceId]);
        }
        else
        {
            flag = "OK";
            sprintf(description, "%s", "");
        }

        analyzePrintHelper(tag, flag, description);
    }

    return onLayer && layerOnScreen;
}

t_ilm_bool analyzeSharedNative(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    string tag;
    string flag;
    char description[300] = "";

    tag = "Shared native";

    t_ilm_bool shared = ILM_FALSE;

    //native of the target surface
    t_ilm_uint targetNative = scene.surfaceProperties[targetSurfaceId].nativeSurface;

    //iterate all surface properties
    for (map<t_ilm_surface, ilmSurfaceProperties>::iterator it = scene.surfaceProperties.begin();
            it != scene.surfaceProperties.end(); ++it)
    {
        t_ilm_surface surface = (*it).first;
        ilmSurfaceProperties& properties = (*it).second;
        //if there is a surface that has the same surface as the target surface
        if (surface != targetSurfaceId && properties.nativeSurface == targetNative)
        {
            shared = ILM_TRUE;

            flag = "WARNING";
            sprintf(description, "Surface %i shares native that has ID %i with surface %i",
                    targetSurfaceId, targetNative, surface);
            analyzePrintHelper(tag, flag, description);
        }
    }

    if (!shared)
    {
        flag = "OK";
        sprintf(description, "%s", "");
        analyzePrintHelper(tag, flag, description);
    }

    return !shared;
}

t_ilm_bool analyzeUpdateCounter(t_ilm_surface targetSurfaceId, t_scene_data& scene)
{
    ilmSurfaceProperties& targetSurfaceProperties = scene.surfaceProperties[targetSurfaceId];

    t_ilm_bool problem = targetSurfaceProperties.updateCounter == 0;
    string tag;
    string flag;
    char description[300] = "";

    tag = "Update Counter";
    //check if surface counter was updated since its creation
    if (problem)
    {
        flag = "PROBLEM";
        sprintf(description, "Surface %i update counter is %i, no content was added to the surface since its creation",
                targetSurfaceId, targetSurfaceProperties.updateCounter);
    }
    else
    {
        flag = "OK";
        sprintf(description, "%s", "");
    }

    analyzePrintHelper(tag, flag, description);

    return !problem;
}
} //end of anonymous namespace


t_ilm_bool analyzeSurface(t_ilm_surface targetSurfaceId)
{
    t_scene_data scene;
    captureSceneData(&scene);

    if (!analyzeCheckSurfaceExists(targetSurfaceId, scene))
        return ILM_TRUE;

    if (!analyzeCheckRendered(targetSurfaceId, scene))
        return ILM_TRUE;

    //check no visibility or low opacity
    analyzeVisibilityAndOpacity(targetSurfaceId, scene);

    //check small dimensions
    analyzeDimensions(targetSurfaceId, scene);

    //check if surface is completely inside the destination region of the layer
    analyzeSurfaceCheckInsideLayer(targetSurfaceId, scene);

    //get occluding visible surfaces
    analyzeOcclusion(targetSurfaceId, scene);

    //check if the surface has been updated (if it has any content)
    analyzeUpdateCounter(targetSurfaceId, scene);

    //check if the surface shares the native with another surface
    analyzeSharedNative(targetSurfaceId, scene);

    return ILM_TRUE;
}
