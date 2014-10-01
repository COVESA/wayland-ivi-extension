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
#include "Expression.h"
#include "ExpressionInterpreter.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <signal.h> // signal
#include <unistd.h> // alarm

using namespace std;


#define COMMAND(text) COMMAND2(__COUNTER__,text)

#define COMMAND2(x,y) COMMAND3(x,y)

#define COMMAND3(funcNumber, text) \
    void func_ ## funcNumber(Expression* input); \
    static const bool reg_ ## funcNumber = \
        ExpressionInterpreter::addExpression(func_ ## funcNumber, text); \
    void func_ ## funcNumber(Expression* input)



//=============================================================================
COMMAND("help")
//=============================================================================
{
    (void)input;
    cout << "help: supported commands:\n\n";
    ExpressionInterpreter::printExpressionList();
    cout << "\n";
}

//=============================================================================
COMMAND("tree")
//=============================================================================
{
    (void)input;
    cout << "help: supported commands:\n\n";
    ExpressionInterpreter::printExpressionTree();
    cout << "\n";
}

//=============================================================================
COMMAND("get scene|screens|layers|surfaces")
//=============================================================================
{
    if (input->contains("scene"))
    {
        printScene();
    }
    else if (input->contains("screens"))
    {
        (void)input;
        unsigned int count = 0;
        unsigned int* array = NULL;

        ilmErrorTypes callResult = ilm_getScreenIDs(&count, &array);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get screen IDs\n";
            return;
        }

        printArray("Screen", array, count);
    }
    else if (input->contains("layers"))
    {
        (void)input;
        int count = 0;
        unsigned int* array = NULL;

        ilmErrorTypes callResult = ilm_getLayerIDs(&count, &array);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get layer IDs\n";
            return;
        }

        printArray("Layer", array, count);
    }
    else if (input->contains("surfaces"))
    {
        (void)input;
        int count = 0;
        unsigned int* array = NULL;

        ilmErrorTypes callResult = ilm_getSurfaceIDs(&count, &array);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get surface IDs\n";
            return;
        }

        printArray("Surface", array, count);
    }
}

//=============================================================================
COMMAND("get screen|layer|surface <id>")
//=============================================================================
{
    if (input->contains("screen"))
    {
        printScreenProperties(input->getUint("id"));
    }
    else if (input->contains("layer"))
    {
        printLayerProperties(input->getUint("id"));
    }
    else if (input->contains("surface"))
    {
        printSurfaceProperties(input->getUint("id"));
    }
}

//=============================================================================
COMMAND("dump screen|layer|surface <id> to <file>")
//=============================================================================
{
    if (input->contains("screen"))
    {
        ilmErrorTypes callResult = ilm_takeScreenshot(input->getUint("id"),
                                                        input->getString("file").c_str());
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to take screenshot of screen with ID " << input->getUint("id") << "\n";
            return;
        }
    }
    else if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_takeLayerScreenshot(input->getString("file").c_str(),
                                                            input->getUint("id"));
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to take screenshot of layer with ID " << input->getUint("id") << "\n";
            return;
        }
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_takeSurfaceScreenshot(input->getString("file").c_str(),
                                                                input->getUint("id"));
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to take screenshot of surface with ID " << input->getUint("id") << "\n";
            return;
        }
    }
}

//=============================================================================
COMMAND("set layer|surface <id> source region <x> <y> <w> <h>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    t_ilm_uint x = input->getUint("x");
    t_ilm_uint y = input->getUint("y");
    t_ilm_uint w = input->getUint("w");
    t_ilm_uint h = input->getUint("h");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetSourceRectangle(id, x, y, w, h);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set source rectangle (" << x << "," << y << ", " << w << ", " << h << ") for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetSourceRectangle(id, x, y, w, h);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set source rectangle (" << x << ", " << y << ", " << w << ", " << h << ") for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> destination region <x> <y> <w> <h>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    t_ilm_uint x = input->getUint("x");
    t_ilm_uint y = input->getUint("y");
    t_ilm_uint w = input->getUint("w");
    t_ilm_uint h = input->getUint("h");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetDestinationRectangle(id, x, y, w, h);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set destination rectangle (" << x << ", " << y << ", " << w << ", " << h << ") for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetDestinationRectangle(id, x, y, w, h);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set destination rectangle (" << x << ", " << y << ", " << w << ", " << h << ") for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> opacity <opacity>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    double opacity = input->getDouble("opacity");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetOpacity(id, opacity);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set opacity " << opacity << " for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetOpacity(id, opacity);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set opacity " << opacity << " for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> visibility <visibility>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    t_ilm_bool visibility = input->getBool("visibility");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetVisibility(id, visibility);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set visibility " << visibility << " for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetVisibility(id, visibility);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set visibility " << visibility << " for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> orientation <orientation>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    ilmOrientation orientation = (ilmOrientation)input->getInt("orientation");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetOrientation(id, orientation);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set orientation " << orientation << " for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetOrientation(id, orientation);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set orientation " << orientation << " for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set screen|layer <id> render order [<idarray>]")
//=============================================================================
{
    if (input->contains("screen"))
    {
        if (input->contains("idarray"))
        {
            unsigned int count = 0;
            unsigned int* array = NULL;
            unsigned int screenid = input->getUint("id");
            input->getUintArray("idarray", &array, &count);

            ilmErrorTypes callResult = ilm_displaySetRenderOrder(screenid, array, count);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set render order for screen with ID " << screenid << "\n";
                return;
            }

            ilm_commitChanges();
        }
        else
        {
            unsigned int screenid = input->getUint("id");

            ilmErrorTypes callResult = ilm_displaySetRenderOrder(screenid, NULL, 0);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set render order for screen with ID " << screenid << "\n";
                return;
            }

            ilm_commitChanges();
        }
    }
    else if (input->contains("layer"))
    {
        if (input->contains("idarray"))
        {
            unsigned int count = 0;
            unsigned int* array = NULL;
            unsigned int layerid = input->getUint("id");
            input->getUintArray("idarray", &array, &count);

            ilmErrorTypes callResult = ilm_layerSetRenderOrder(layerid, array, count);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set render order for layer with ID " << layerid << "\n";
                return;
            }

            ilm_commitChanges();
        }
        else
        {
            unsigned int layerid = input->getUint("id");

            ilmErrorTypes callResult = ilm_layerSetRenderOrder(layerid, NULL, 0);
            if (ILM_SUCCESS != callResult)
            {
                cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
                cout << "Failed to set render order for layer with ID " << layerid << "\n";
                return;
            }

            ilm_commitChanges();
        }
    }
}

//=============================================================================
COMMAND("set layer|surface <id> width <width>")
//=============================================================================
{
    if (input->contains("layer"))
    {
        unsigned int dimension[2];
        unsigned int layerid = input->getUint("id");

        ilmErrorTypes callResult = ilm_layerGetDimension(layerid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get dimensions of layer with ID " << layerid << "\n";
            return;
        }

        dimension[0] = input->getUint("width");

        callResult = ilm_layerSetDimension(layerid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set dimensions of layer with ID " << layerid << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        unsigned int dimension[2];
        unsigned int surfaceid = input->getUint("id");

        ilmErrorTypes callResult = ilm_surfaceGetDimension(surfaceid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get dimensions of surface with ID " << surfaceid << "\n";
            return;
        }

        dimension[0] = input->getUint("width");

        callResult = ilm_surfaceSetDimension(surfaceid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set dimensions of surface with ID " << surfaceid << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> height <height>")
//=============================================================================
{
    if (input->contains("layer"))
    {
        unsigned int dimension[2];
        unsigned int layerid = input->getUint("id");

        ilmErrorTypes callResult = ilm_layerGetDimension(layerid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get dimensions of layer with ID " << layerid << "\n";
            return;
        }

        dimension[1] = input->getUint("height");

        callResult = ilm_layerSetDimension(layerid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set dimensions of layer with ID " << layerid << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        unsigned int dimension[2];
        unsigned int surfaceid = input->getUint("id");

        ilmErrorTypes callResult = ilm_surfaceGetDimension(surfaceid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get dimensions of surface with ID " << surfaceid << "\n";
            return;
        }

        dimension[1] = input->getUint("height");

        callResult = ilm_surfaceSetDimension(surfaceid, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set dimensions of surface with ID " << surfaceid << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set layer|surface <id> position <x> <y>")
//=============================================================================
{
    unsigned int id = input->getUint("id");
    unsigned int dimension[2];
    dimension[0] = input->getUint("x");
    dimension[1] = input->getUint("y");

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetPosition(id, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set position of layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetPosition(id, dimension);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set position of surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("create layer <layerid> <width> <height>")
//=============================================================================
{
    unsigned int layerid = input->getUint("layerid");
    unsigned int width = input->getUint("width");
    unsigned int height = input->getUint("height");

    ilmErrorTypes callResult = ilm_layerCreateWithDimension(&layerid, width, height);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to create layer with ID " << layerid << "\n";
        return;
    }
}

//=============================================================================
COMMAND("create surface <surfaceid> <nativehandle> <width> <height> <pixelformat>")
//=============================================================================
{
    unsigned int surfaceid = input->getUint("surfaceid");
    unsigned int nativeHandle = input->getUint("nativehandle");
    unsigned int width = input->getUint("width");
    unsigned int height = input->getUint("height");
    e_ilmPixelFormat pixelformat = (e_ilmPixelFormat)input->getUint("pixelformat");

    ilmErrorTypes callResult = ilm_surfaceCreate(nativeHandle, width, height, pixelformat, &surfaceid);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to create surface with ID " << surfaceid << "\n";
        return;
    }
}

//=============================================================================
COMMAND("destroy layer|surface <id>")
//=============================================================================
{
    if (input->contains("layer"))
    {
        unsigned int layerid = input->getUint("id");

        ilmErrorTypes callResult = ilm_layerRemove(layerid);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to remove layer with ID " << layerid << "\n";
            return;
        }
    }
    else if (input->contains("surface"))
    {
        unsigned int surfaceid = input->getUint("id");

        ilmErrorTypes callResult = ilm_surfaceRemove(surfaceid);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to remove surface with ID " << surfaceid << "\n";
            return;
        }
    }

    ilm_commitChanges();
}

//=============================================================================
COMMAND("get communicator performance")
//=============================================================================
{
    (void) input; //suppress warning: unused parameter
    getCommunicatorPerformance();
}

//=============================================================================
COMMAND("set surface <surfaceid> keyboard focus")
//=============================================================================
{
    t_ilm_surface surface = input->getUint("surfaceid");

    setSurfaceKeyboardFocus(surface);
}

//=============================================================================
COMMAND("get keyboard focus")
//=============================================================================
{
    (void) input; //suppress warning: unused parameter
    getKeyboardFocus();
}

//=============================================================================
COMMAND("set surface <surfaceid> accept <acceptance> input events from devices <kbd:pointer:touch>")
//=============================================================================
{
    t_ilm_surface surfaceId = input->getUint("surfaceid");
    t_ilm_bool acceptance = input->getBool("acceptance");
    string kbdPointerTouch = input->getString("kbd:pointer:touch");

    setSurfaceAcceptsInput(surfaceId, kbdPointerTouch, acceptance);
}

//=============================================================================
COMMAND("set layer|surface <id> chromakey <red> <green> <blue>")
//=============================================================================
{
    t_ilm_layer id = input->getUint("id");
    t_ilm_int color[3] =
    {
        input->getInt("red"),
        input->getInt("green"),
        input->getInt("blue")
    };

    if (input->contains("layer"))
    {
        ilmErrorTypes callResult = ilm_layerSetChromaKey(id, color);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set chroma key (" << color[0] << ", " << color[1] << ", " << color[2] << ") for layer with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
    else if (input->contains("surface"))
    {
        ilmErrorTypes callResult = ilm_surfaceSetChromaKey(id, color);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to set chroma key (" << color[0] << ", " << color[1] << ", " << color[2] << ") for surface with ID " << id << "\n";
            return;
        }

        ilm_commitChanges();
    }
}

//=============================================================================
COMMAND("set surface <surfaceid> chromakey disabled")
//=============================================================================
{
    t_ilm_surface surface = input->getUint("surfaceid");

    ilmErrorTypes callResult = ilm_surfaceSetChromaKey(surface, NULL);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to disable chroma key for surface with ID " << surface << "\n";
        return;
    }

    ilm_commitChanges();
}

//=============================================================================
COMMAND("test notification layer <layerid>")
//=============================================================================
{
    unsigned int layerid = input->getUint("layerid");

    testNotificationLayer(layerid);
}

//=============================================================================
COMMAND("watch layer|surface <idarray>")
//=============================================================================
{
    if (input->contains("layer"))
    {
        unsigned int* layerids = NULL;
        unsigned int layeridCount;
        input->getUintArray("idarray", &layerids, &layeridCount);

        watchLayer(layerids, layeridCount);
    }
    else if (input->contains("surface"))
    {
        unsigned int* surfaceids = NULL;
        unsigned int surfaceidCount;
        input->getUintArray("idarray", &surfaceids, &surfaceidCount);

        watchSurface(surfaceids, surfaceidCount);
    }
}

//=============================================================================
COMMAND("set optimization <id> mode <mode>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    t_ilm_uint mode = input->getUint("mode");
    setOptimization(id, mode);
}

//=============================================================================
COMMAND("get optimization <id>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("id");
    getOptimization(id);
}

//=============================================================================
COMMAND("analyze surface <surfaceid>")
//=============================================================================
{
    t_ilm_surface targetSurfaceId = (t_ilm_uint) input->getUint("surfaceid");
    analyzeSurface(targetSurfaceId);
}

//=============================================================================
COMMAND("scatter [all]")
//=============================================================================
{
    if (input->contains("all"))
    {
        scatterAll();
    }
    else
    {
        scatter();
    }
}

//=============================================================================
COMMAND("demo [<animation_mode=2>]")
//=============================================================================
{
    t_ilm_uint mode = (t_ilm_uint) input->getUint("animation_mode");
    demo(mode);
}

//=============================================================================
COMMAND("export scene to <filename>")
//=============================================================================
{
    string filename = (string) input->getString("filename");
    exportSceneToFile(filename);
}

//=============================================================================
COMMAND("export xtext to <filename> <grammar> <url>")
//=============================================================================
{
    string filename = (string) input->getString("filename");
    string grammar = (string) input->getString("grammar");
    string url = (string) input->getString("url");
    exportXtext(filename, grammar, url);
}

//=============================================================================
COMMAND("import scene from <filename>")
//=============================================================================
{
    string filename = (string) input->getString("filename");
    importSceneFromFile(filename);
}
