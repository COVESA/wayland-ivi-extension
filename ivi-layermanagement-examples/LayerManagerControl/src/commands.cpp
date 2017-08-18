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
        free(array);
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
        free(array);
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
        free(array);
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
COMMAND("dump screen|surface <id> to <file>")
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
    t_ilm_int x = input->getInt("x");
    t_ilm_int y = input->getInt("y");
    t_ilm_int w = input->getInt("w");
    t_ilm_int h = input->getInt("h");

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
    t_ilm_int x = input->getInt("x");
    t_ilm_int y = input->getInt("y");
    t_ilm_int w = input->getInt("w");
    t_ilm_int h = input->getInt("h");

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
//=============================================================================
COMMAND("set surface <surfaceid> type <type>")
//=============================================================================
{
    t_ilm_uint id = input->getUint("surfaceid");
    ilmSurfaceType type = (ilmSurfaceType)input->getInt("type");

    ilmErrorTypes callResult = ilm_surfaceSetType(id, type);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to set type " << type << " for surface with ID " << id << "\n";
        return;
    }

    ilm_commitChanges();
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
            delete[] array;
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
            delete[] array;
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
COMMAND("destroy layer <id>")
//=============================================================================
{
    unsigned int layerid = input->getUint("id");

    ilmErrorTypes callResult = ilm_layerRemove(layerid);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to remove layer with ID " << layerid << "\n";
        return;
    }

    ilm_commitChanges();
}

//=============================================================================
COMMAND("add surface <sid> to layer <lid>")
//=============================================================================
{
    t_ilm_uint sid = input->getUint("sid");
    t_ilm_uint lid = input->getUint("lid");

    ilmErrorTypes callResult = ilm_layerAddSurface(lid, sid);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to add surface (" << sid << " ) to layer (" << lid << " ) " << "\n";
        return;
    }

    ilm_commitChanges();
}

//=============================================================================
COMMAND("remove surface <sid> from layer <lid>")
//=============================================================================
{
    t_ilm_uint sid = input->getUint("sid");
    t_ilm_uint lid = input->getUint("lid");

    ilmErrorTypes callResult = ilm_layerRemoveSurface(lid, sid);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to remove surface (" << sid << " ) from layer (" << lid << " ) " << "\n";
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
        delete[] layerids;
    }
    else if (input->contains("surface"))
    {
        unsigned int* surfaceids = NULL;
        unsigned int surfaceidCount;
        input->getUintArray("idarray", &surfaceids, &surfaceidCount);

        watchSurface(surfaceids, surfaceidCount);
        delete[] surfaceids;
    }
}

//=============================================================================
COMMAND("analyze surface <surfaceid>")
//=============================================================================
{
    t_ilm_surface targetSurfaceId = (t_ilm_uint) input->getUint("surfaceid");
    analyzeSurface(targetSurfaceId);
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
