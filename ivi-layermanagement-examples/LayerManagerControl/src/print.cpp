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

#include <iostream>
using std::cout;
using std::cin;
using std::endl;

#include <iomanip>
using std::dec;
using std::hex;

#include <stdlib.h>
#include <map>
using std::map;

#include <vector>
using std::vector;


void printArray(const char* text, unsigned int* array, int count)
{
    cout << count << " " << text << "(s):\n";
    for (int i = 0; i < count; ++i)
    {
        cout << "- " << text << " " << dec << array[i] << hex << " (0x"
                << array[i] << ")" << dec << "\n";
    }
}

template<typename T>
void printArray(const char* text, T* array, int count)
{
    cout << count << " " << text << "(s):\n";
    for (int i = 0; i < count; ++i)
    {
        cout << "- " << text << " " << "[" << array[i] << "]" << "\n";
    }
}

template<typename T>
void printVector(const char* text, vector<T> v)
{
    cout << v.size() << " " << text << "(s) Vector:\n";
    for (int i = 0; i < v.size(); ++i)
    {
        cout << "- " << text << " " << v[i] << endl;
    }
}

template<typename K, typename V>
void printMap(const char* text, std::map<K, V> m)
{
    cout << m.size() << " " << text << endl;

    for (typename map<K, V>::iterator it = m.begin(); it != m.end(); ++it)
    {
        cout << "- " << (*it).first << " -> " << (*it).second << endl;
    }
}

void printScreenProperties(unsigned int screenid, const char* prefix)
{
    cout << prefix << "screen " << screenid << " (0x" << hex << screenid << dec
            << ")\n";
    cout << prefix << "---------------------------------------\n";

    ilmScreenProperties screenProperties;

    ilmErrorTypes callResult = ilm_getPropertiesOfScreen(screenid, &screenProperties);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get properties of screen with ID " << screenid << " found\n";
        return;
    }

    cout << prefix << "- connector name:       " << screenProperties.connectorName << "\n";

    cout << prefix << "- resolution:           x=" << screenProperties.screenWidth << ", y="
            << screenProperties.screenHeight << "\n";

    cout << prefix << "- layer render order:   ";

    for (t_ilm_uint layerIndex = 0; layerIndex < screenProperties.layerCount; ++layerIndex)
    {
        t_ilm_layer layerid = screenProperties.layerIds[layerIndex];
        cout << layerid << "(0x" << hex << layerid << dec << "), ";
    }
    cout << "\n";

    free(screenProperties.layerIds);
}

void printLayerProperties(unsigned int layerid, const char* prefix)
{
    cout << prefix << "layer " << layerid << " (0x" << hex << layerid << dec
            << ")\n";
    cout << prefix << "---------------------------------------\n";

    ilmLayerProperties p;

    ilmErrorTypes callResult = ilm_getPropertiesOfLayer(layerid, &p);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get properties of layer with ID " << layerid << " found\n";
        return;
    }

    cout << prefix << "- destination region:   x=" << p.destX << ", y="
            << p.destY << ", w=" << p.destWidth << ", h=" << p.destHeight
            << "\n";
    cout << prefix << "- source region:        x=" << p.sourceX << ", y="
            << p.sourceY << ", w=" << p.sourceWidth << ", h=" << p.sourceHeight
            << "\n";

    cout << prefix << "- opacity:              " << p.opacity << "\n";
    cout << prefix << "- visibility:           " << p.visibility << "\n";

    cout << prefix << "- surface render order: ";

    int surfaceCount = 0;
    unsigned int* surfaceArray = NULL;

    callResult = ilm_getSurfaceIDsOnLayer(layerid, &surfaceCount, &surfaceArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get surfaces on layer with ID " << layerid << " \n";
        return;
    }

    for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
    {
        cout << surfaceArray[surfaceIndex] << "(0x" << hex
                << surfaceArray[surfaceIndex] << dec << "), ";
    }
    cout << "\n";
    free(surfaceArray);

    cout << prefix << "- on screen:            ";

    unsigned int screenCount = 0;
    unsigned int* screenArray = NULL;

    callResult = ilm_getScreenIDs(&screenCount, &screenArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available screens\n";
        return;
    }

    for (unsigned int screenIndex = 0; screenIndex < screenCount;
            ++screenIndex)
    {
        unsigned int screenid = screenArray[screenIndex];
        int layerCount = 0;
        unsigned int* layerArray = NULL;

        callResult = ilm_getLayerIDsOnScreen(screenid, &layerCount, &layerArray);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get available layers on screen with ID" << screenid << "\n";
            return;
        }

        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
            unsigned int id = layerArray[layerIndex];
            if (id == layerid)
            {
                cout << screenid << "(0x" << hex << screenid << dec << ") ";
            }
        }

        free(layerArray);
    }
    cout << "\n";

    free(screenArray);
}

void printSurfaceProperties(unsigned int surfaceid, const char* prefix)
{
    cout << prefix << "surface " << surfaceid << " (0x" << hex << surfaceid
            << dec << ")\n";
    cout << prefix << "---------------------------------------\n";

    ilmSurfaceProperties p;

    ilmErrorTypes callResult = ilm_getPropertiesOfSurface(surfaceid, &p);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "No surface with ID " << surfaceid << " found\n";
        return;
    }

    cout << prefix << "- created by pid:       " << p.creatorPid << "\n";

    cout << prefix << "- original size:      x=" << p.origSourceWidth << ", y="
            << p.origSourceHeight << "\n";
    cout << prefix << "- destination region: x=" << p.destX << ", y=" << p.destY
            << ", w=" << p.destWidth << ", h=" << p.destHeight << "\n";
    cout << prefix << "- source region:      x=" << p.sourceX << ", y="
            << p.sourceY << ", w=" << p.sourceWidth << ", h=" << p.sourceHeight
            << "\n";

    cout << prefix << "- opacity:            " << p.opacity << "\n";
    cout << prefix << "- visibility:         " << p.visibility << "\n";

    cout << prefix << "- frame counter:      " << p.frameCounter << "\n";

    cout << prefix << "- on layer:           ";
    int layerCount = 0;
    unsigned int* layerArray = NULL;

    callResult = ilm_getLayerIDs(&layerCount, &layerArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available layer IDs\n";
        return;
    }

    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        unsigned int layerid = layerArray[layerIndex];
        int surfaceCount = 0;
        unsigned int* surfaceArray = NULL;

        callResult = ilm_getSurfaceIDsOnLayer(layerid, &surfaceCount, &surfaceArray);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get surface IDs on layer" << layerid << "\n";
            return;
        }

        for (int surfaceIndex = 0; surfaceIndex < surfaceCount;
                ++surfaceIndex)
        {
            unsigned int id = surfaceArray[surfaceIndex];
            if (id == surfaceid)
            {
                cout << layerid << "(0x" << hex << layerid << dec << ") ";
            }
        }

        free(surfaceArray);
    }
    cout << "\n";

    free(layerArray);
}

void printScene()
{
    unsigned int screenCount = 0;
    unsigned int* screenArray = NULL;

    ilmErrorTypes callResult = ilm_getScreenIDs(&screenCount, &screenArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available screen IDs\n";
        return;
    }

    for (unsigned int screenIndex = 0; screenIndex < screenCount; ++screenIndex)
    {
        unsigned int screenid = screenArray[screenIndex];
        printScreenProperties(screenid);
        cout << "\n";

        int layerCount = 0;
        unsigned int* layerArray = NULL;

        callResult = ilm_getLayerIDsOnScreen(screenid, &layerCount, &layerArray);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get available layers on screen with ID" << screenid << "\n";
            return;
        }

        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
            unsigned int layerid = layerArray[layerIndex];
            printLayerProperties(layerid, "    ");
            cout << "\n";

            int surfaceCount = 0;
            unsigned int* surfaceArray = NULL;

            callResult = ilm_getSurfaceIDsOnLayer(layerid, &surfaceCount, &surfaceArray);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to get available surfaces on layer with ID" << layerid << "\n";
                return;
            }

            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                unsigned int surfaceid = surfaceArray[surfaceIndex];
                printSurfaceProperties(surfaceid, "        ");
                cout << "\n";
            }

            free(surfaceArray);
        }

        free(layerArray);
    }

    free(screenArray);
}
