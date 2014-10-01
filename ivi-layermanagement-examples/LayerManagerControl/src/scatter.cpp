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
#include "ilm_control.h"
#include "LMControl.h"

#include <cmath>
using std::max;
using std::min;

#include <cstdlib>

#include <iostream>
using std::cout;
using std::cin;
using std::endl;

#include <iomanip>
using std::dec;
using std::hex;

#include <sstream>
using std::stringstream;

#include <pthread.h>
#include <unistd.h>


t_scene_data getScatteredScene(t_scene_data* pInitialScene)
{
    t_scene_data& initialScene = *pInitialScene;

    //make dummy scene
    t_scene_data scatteredScene = cloneToUniLayerScene(&initialScene);

    //get render order of surfaces in initial scene
    vector<t_ilm_surface> renderOrder = getSceneRenderOrder(&initialScene);
    //get surfaces' coordinates of surfaces in initial scene
    map<t_ilm_surface, tuple4> coordinates;

    for (vector<t_ilm_surface>::iterator it = renderOrder.begin();
            it != renderOrder.end(); ++it)
    {
        t_ilm_surface surface = *it;
        coordinates[surface] = getSurfaceScreenCoordinates(&initialScene, surface);
    }

    //scale surfaces' coordinates
    //calculate needed proportions
    int n = renderOrder.size();
    int rows = ceil(sqrt(n));
    int cols = ceil(sqrt(n));
    float scale = min(1.0 / cols, 1.0 / rows);
    int maxWidth = static_cast<int>(initialScene.screenWidth * scale);
    int maxHeight = static_cast<int>(initialScene.screenHeight * scale);

    //scale surface coordinates
    int counter = 0;
    for (vector<t_ilm_surface>::iterator it = renderOrder.begin();
            it != renderOrder.end(); ++it)
    {
        t_ilm_surface surface = *it;
        coordinates[surface].scale(scale);

        coordinates[surface].x += maxWidth * (counter % cols);
        coordinates[surface].y += maxHeight * (counter / cols);
        coordinates[surface].z += maxWidth * (counter % cols);
        coordinates[surface].w += maxHeight * (counter / cols);

        scatteredScene.surfaceProperties[surface].destX = coordinates[surface].x;
        scatteredScene.surfaceProperties[surface].destY = coordinates[surface].y;
        scatteredScene.surfaceProperties[surface].destWidth = coordinates[surface].z - coordinates[surface].x;
        scatteredScene.surfaceProperties[surface].destHeight = coordinates[surface].w - coordinates[surface].y;

        ++counter;
    }

    return scatteredScene;
}

void* scatterThreadCallback(void* param)
{
    map<string, void*> * pParamMap = static_cast<map<string, void*>* >(param);
    t_ilm_surface* surface = static_cast<t_ilm_surface*>((*pParamMap)["pHighlighedSurface"]);
    bool* stop = static_cast<bool*>((*pParamMap)["pStop"]);
    while (! *stop)
    {
        if (*surface != 0xFFFFFFFF)
        {
            t_ilm_surface currentSurface = *surface;

            ilmErrorTypes callResult = ilm_surfaceSetVisibility(currentSurface, false);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set visibility " << false << " for surface with ID " << currentSurface << "\n";
            }

            ilm_commitChanges();
            usleep(100000);

            callResult = ilm_surfaceSetVisibility(currentSurface, true);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set visibility " << true << " for surface with ID " << currentSurface << "\n";
            }

            ilm_commitChanges();
        }

        usleep(400000);
    }
    return NULL;
}

void scatterHandleUserInput(t_scene_data* pOriginalScene, t_scene_data* pScatteredScene)
{
    int n = pScatteredScene->surfaces.size();
    int cols = ceil(sqrt(n));

    //make keys map
    map<string, t_ilm_surface> keySurfaceMap;

    for (int i = 0; i < n; ++i)
    {
        int row = i / cols;
        int col = i % cols;

        char key[3];
        key[0] = 'A' + row;
        key[1] = '0' + col;
        key[2] = '\0';

        keySurfaceMap[key] = pScatteredScene->surfaces[i];
    }

    //start thread for making surfaces blink
    map<string, void*> threadParamMap;
    t_ilm_surface hightlightThreadSurface = 0xFFFFFFFF;
    threadParamMap["pHighlighedSurface"] = static_cast<void*> (& hightlightThreadSurface);
    bool hightlightThreadStop = false;
    threadParamMap["pStop"] = static_cast<void*> (& hightlightThreadStop);

    pthread_t highlightThread;
    pthread_create(& highlightThread, NULL, scatterThreadCallback, &threadParamMap);

    char userInput[3];
    while (true)
    {
        //print Grid
        for (int i = 0; i < n; ++i)
        {
            int row = i / cols;
            int col = i % cols;

            char key[3];
            key[0] = 'A' + row;
            key[1] = '0' + col;
            key[2] = '\0';
            if (col == 0)
            {
                cout << endl;
                cout << string(5 * cols + 1, '-') << endl;
                cout << "| ";
            }
            cout << key << " | ";
        }

        cout << endl << string(5 * cols + 1, '-') << endl;
        //take input from user
        cout << "Choose a surface [Or press ENTER to exit...]:" << endl;

        userInput[0] = cin.get();

        //if the user presses Enter: exit
        if (userInput[0] == '\n')
        {
            hightlightThreadStop = true;
            pthread_join(highlightThread, NULL);
            break;
        }
        else if (userInput[0] >= 'a' && userInput[0] <='z')
        {
            //switch to uppercase
            userInput[0] += 'A' - 'a';
        }

        userInput[1] = cin.get();
        //if the user presses Enter by mistake: reset input
        if (userInput[1] == '\n')
        {
            continue;
        }

        userInput[2] = cin.get();
        //if the user DOES NOT press Enter by mistake: reset input
        if (userInput[2] != '\n')
        {
            continue;
        }

        userInput[2] = '\0';

        if (keySurfaceMap.find(userInput) != keySurfaceMap.end())
        {
            //this change is reflected in the highligh thread
            hightlightThreadSurface = keySurfaceMap[userInput];

            //print surface information
            ilmSurfaceProperties& surfaceProperties = pOriginalScene->surfaceProperties[hightlightThreadSurface];

            map<ilmOrientation, const char*> orientations;

            orientations[ILM_ZERO] = "0 degrees";
            orientations[ILM_NINETY] = "90 degrees";
            orientations[ILM_ONEHUNDREDEIGHTY]= "180 degrees";
            orientations[ILM_TWOHUNDREDSEVENTY] = "270 degrees";

            map<int, const char*> pixelFormats;
            pixelFormats[ILM_PIXELFORMAT_R_8] = "R_8";
            pixelFormats[ILM_PIXELFORMAT_RGB_888] = "RGB_888";
            pixelFormats[ILM_PIXELFORMAT_RGBA_8888] = "RGBA_8888";
            pixelFormats[ILM_PIXELFORMAT_RGB_565] = "RGB_565";
            pixelFormats[ILM_PIXELFORMAT_RGBA_5551] = "RGBA_5551";
            pixelFormats[ILM_PIXELFORMAT_RGBA_6661] = "RGBA_6661";
            pixelFormats[ILM_PIXELFORMAT_RGBA_4444] = "RGBA_4444";
            pixelFormats[ILM_PIXEL_FORMAT_UNKNOWN] = "FORMAT_UNKNOWN";

            string inputDeviceAcceptance = (surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_KEYBOARD ? "Keyboard " : "");
            inputDeviceAcceptance +=(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_POINTER ? "Pointer " : "");
            inputDeviceAcceptance += (surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_TOUCH ? "Touch " : "");

            stringstream tempStream;
            char fillChar = '-';
            int width = 20;
            tempStream << "\n--------------------------------\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Surface" << ":" << hightlightThreadSurface << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Destination Region" << ":" << "[x:" << surfaceProperties.destX << ", y:" << surfaceProperties.destY << ", w:" << surfaceProperties.destWidth << ", h:" << surfaceProperties.destHeight << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Source Region" << ":" << "[x:" << surfaceProperties.sourceX << ", y:" << surfaceProperties.sourceY << ", w:" << surfaceProperties.sourceWidth << ", h:" << surfaceProperties.sourceHeight << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Original Dimensions" << ":" << "[w:" << surfaceProperties.origSourceWidth << ", h:" << surfaceProperties.origSourceHeight << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Opacity" << ":" << surfaceProperties.opacity << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Visibility" << ":" << surfaceProperties.visibility << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Input Devices" << ":" << surfaceProperties.inputDevicesAcceptance << " [" << inputDeviceAcceptance << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Native Surface" << ":" << surfaceProperties.nativeSurface << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Orientation" << ":" << surfaceProperties.orientation << " [" << orientations[surfaceProperties.orientation] << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Pixel Format" << ":" << surfaceProperties.pixelformat << " [" << pixelFormats[surfaceProperties.pixelformat] << "]\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Update Counter" << ":" << surfaceProperties.updateCounter << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Draw Counter" << ":" << surfaceProperties.drawCounter << "\n";
            tempStream << std::setw(width) << std::left << std::setfill(fillChar) << "Frame Counter" << ":" << surfaceProperties.frameCounter << "\n";

            cout << tempStream.str();
        }
        else
        {
            cout << "Unrecognized input!" << endl;
        }
    }
}

void scatter()
{
    //capture initial scene
    t_scene_data initialScene;
    captureSceneData(&initialScene);

    setSceneToRestore(&initialScene);

    //make scattered scene
    t_scene_data scatteredScene = getScatteredScene(&initialScene);

    //transform from initial to scattered scene
    transformScene(&initialScene, &scatteredScene, 1000, 50);

    //wait for user input
    scatterHandleUserInput(&initialScene, &scatteredScene);

    //transform from scattered to initial scene
    transformScene(&scatteredScene, &initialScene, 1000, 50);

    //set initial scene (reset!)
    setScene(&initialScene, true);
}

void scatterAll()
{
    //capture initial scene
    t_scene_data initialScene;
    captureSceneData(&initialScene);

    setSceneToRestore(&initialScene);

    //make a scene in which all surfaces belong to a layer (any layer)

    t_scene_data modifiedScene = cloneToUniLayerScene(&initialScene);
    for (vector<t_ilm_surface>::iterator it = initialScene.surfaces.begin();
            it != initialScene.surfaces.end(); ++it)
    {
        t_ilm_surface surface = *it;

        //if surface does not belong to a layer or layer does not belong to a surface
        if (! surfaceRenderedOnScreen(initialScene, surface))
        {
            modifiedScene.surfaces.push_back(surface);
            modifiedScene.surfaceProperties[surface] = initialScene.surfaceProperties[surface];
            modifiedScene.surfaceLayer[surface] = modifiedScene.layers[0];
            modifiedScene.layerSurfaces.begin()->second.push_back(surface);

            tuple4 coords = getSurfaceScreenCoordinates(&initialScene, surface);

            modifiedScene.surfaceProperties[surface].destX = coords.x;
            modifiedScene.surfaceProperties[surface].destY = coords.y;
            modifiedScene.surfaceProperties[surface].destWidth = coords.z - coords.x;
            modifiedScene.surfaceProperties[surface].destHeight = coords.w - coords.y;
        }

        modifiedScene.surfaceProperties[surface].opacity = 1;
        modifiedScene.surfaceProperties[surface].visibility = true;
    }

    //just scatter !!
    setScene(&modifiedScene);
    scatter();

    //set initial scene (reset)
    setScene(&initialScene, true);
}
