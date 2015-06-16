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
#include "ilm_input.h"
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
COMMAND3(50,"get input devices with pointer|keyboard|touch|all")
//=============================================================================
{
    t_ilm_uint num_seats = 0;
    t_ilm_string *seats;
    ilmInputDevice mask = 0;

    if (input->contains("pointer"))
        mask |= ILM_INPUT_DEVICE_POINTER;
    if (input->contains("keyboard"))
        mask |= ILM_INPUT_DEVICE_KEYBOARD;
    if (input->contains("touch"))
        mask |= ILM_INPUT_DEVICE_TOUCH;
    if (input->contains("all"))
        mask |= ILM_INPUT_DEVICE_ALL;

    ilmErrorTypes callResult = ilm_getInputDevices(mask, &num_seats, &seats);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get input devices for mask " << input->getUint("mask") << "\n";
        return;
    }

    for(unsigned int i = 0; i < num_seats; i++) {
        cout << seats[i] << endl;
        free(seats[i]);
    }

    free(seats);
}

//=============================================================================
COMMAND3(51,"set|unset surfaces [<idarray>] input focus pointer|keyboard|touch|all")
//=============================================================================
{
    t_ilm_surface *surfaceIDs;
    t_ilm_uint num_surfaces;
    ilmInputDevice bitmask = 0;
    t_ilm_bool is_set;

    if (input->contains("set"))
        is_set = ILM_TRUE;
    else
        is_set = ILM_FALSE;

    if (input->contains("pointer"))
        bitmask |= ILM_INPUT_DEVICE_POINTER;
    if (input->contains("keyboard"))
        bitmask |= ILM_INPUT_DEVICE_KEYBOARD;
    if (input->contains("touch"))
        bitmask |= ILM_INPUT_DEVICE_TOUCH;
    if (input->contains("all"))
        bitmask |= ILM_INPUT_DEVICE_ALL;

    input->getUintArray("idarray", &surfaceIDs, &num_surfaces);

    cout << "setting input focus in LayerManagerControl" << endl;
    ilmErrorTypes callResult =
        ilm_setInputFocus(surfaceIDs, num_surfaces, bitmask, is_set);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << endl;
        cout << "Failed to set input focus" << endl;
    }
    else
    {
        cout << "LayerManagerService succeeded" << endl;
    }
}

//=============================================================================
COMMAND3(52,"get input focus")
//=============================================================================
{
    (void) input;
    t_ilm_surface *surfaceIDs;
    ilmInputDevice *bitmasks;
    t_ilm_uint num_ids = 0;
    ilmErrorTypes callResult = ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << endl;
        cout << "Failed to get input focus" << endl;
    }
    else
    {
        for (unsigned int i = 0; i < num_ids; i++)
        {
            cout << "surface " << surfaceIDs[i] << ": "
                 << ((bitmasks[i] & ILM_INPUT_DEVICE_POINTER) ? "pointer " : "")
                 << ((bitmasks[i] & ILM_INPUT_DEVICE_KEYBOARD) ? "keyboard " : "")
                 << ((bitmasks[i] & ILM_INPUT_DEVICE_TOUCH) ? "touch" : "")
                 << endl;
        }
    }
    free(surfaceIDs);
    free(bitmasks);
}

//=============================================================================
COMMAND3(53,"get input device <name> capabilities")
//=============================================================================
{
    ilmInputDevice bitmask;
    ilmErrorTypes callResult =
        ilm_getInputDeviceCapabilities((char*)input->getString("name").c_str(), &bitmask);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get capabilities for device " << input->getString("name") << "\n";
        return;
    }
    if (bitmask & ILM_INPUT_DEVICE_POINTER)
        cout << "pointer" << endl;
    if (bitmask & ILM_INPUT_DEVICE_KEYBOARD)
        cout << "keyboard" << endl;
    if (bitmask & ILM_INPUT_DEVICE_TOUCH)
        cout << "touch" << endl;
}

//=============================================================================
COMMAND3(54,"set surface <surfaceid> input acceptance to [<namearray>]")
//=============================================================================
{
	ilmErrorTypes callResult = ILM_FAILED;
    t_ilm_string *array = NULL;
    t_ilm_uint count = 0;
    t_ilm_surface surfaceid = input->getUint("surfaceid");
    string str;
    size_t pos;
     unsigned int i;

     if (input->contains("namearray")) {
         // Generate a string array
         str = input->getString("namearray");
         count = std::count(str.begin(), str.end(), ',') + 1;
         array = (t_ilm_string *)calloc(count, sizeof *array);
         if (array == NULL) {
             cerr << "Failed to allocate memory for string array" << endl;
             return;
         }

         i = 0;
         while(true) {
             pos = str.find(",");
             string token = str.substr(0, pos);
             array[i] = strdup(token.c_str());
             if (array[i] == NULL) {
                 cerr << "Failed to duplicate string: " << token << endl;
                 for (unsigned int j = 0; j < i; j++)
                     free(array[i]);
                 free(array);
                 return;
             }
             str.erase(0, pos + 1);
             i++;
             if (pos == std::string::npos)
                 break;
         }

         callResult = ilm_setInputAcceptanceOn(surfaceid, count, array);

         for (uint i = 0; i < count; i++)
             free(array[i]);
         free(array);
     } else {
         callResult = ilm_setInputAcceptanceOn(surfaceid, 0, NULL);
     }

    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult)
             << endl;
        cout << "Failed to set acceptance for surface " << surfaceid << endl;
    }
}

//=============================================================================
COMMAND3(55,"get surface <surfaceid> input acceptance")
//=============================================================================
{
    t_ilm_string *array = NULL;
    t_ilm_uint num_seats;
    t_ilm_surface surfaceid = input->getUint("surfaceid");

    ilmErrorTypes callResult = ilm_getInputAcceptanceOn(surfaceid, &num_seats,
                                                        &array);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult)
             << endl;
        cout << "Failed to get acceptance for surface " << surfaceid << endl;
        return;
    }

    for (uint i = 0; i < num_seats; i++) {
        cout << array[i] << endl;
        free(array[i]);
    }
    free(array);
}
