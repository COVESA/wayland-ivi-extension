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

#include <cstring>

#include <iostream>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;


#include <iomanip>
using std::dec;
using std::hex;


#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>


bool gBenchmark_running;

void benchmarkSigHandler(int sig)
{
    (void) sig;
    gBenchmark_running = false;
}

void getCommunicatorPerformance()
{
    int runs = 0;
    int runtimeInSec = 5;
    ilmScreenProperties screenProperties;
    cout << "running performance test for " << runtimeInSec << " seconds... ";
    flush(cout);

    signal(SIGALRM, benchmarkSigHandler);

    gBenchmark_running = true;

    alarm(runtimeInSec);

    while (gBenchmark_running)
    {
        t_ilm_uint screenid = 0;

        ilmErrorTypes callResult = ilm_getPropertiesOfScreen(screenid, &screenProperties);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to get properties for screen with ID " << screenid << "\n";
            return;
        }

        ++runs;
    }

    signal(SIGALRM, SIG_DFL);

    cout << (runs / runtimeInSec) << " transactions/second\n";
}

void layerNotificationCallback(t_ilm_layer layer, struct ilmLayerProperties* properties, t_ilm_notification_mask mask)
{
    cout << "\nNotification: layer " << layer << " updated properties:\n";

    if (ILM_NOTIFICATION_VISIBILITY & mask)
    {
        cout << "\tvisibility = " << properties->visibility << "\n";
    }

    if (ILM_NOTIFICATION_OPACITY & mask)
    {
        cout << "\topacity = " << properties->opacity << "\n";
    }

    if (ILM_NOTIFICATION_ORIENTATION & mask)
    {
        cout << "\torientation = " << properties->orientation << "\n";
    }

    if (ILM_NOTIFICATION_SOURCE_RECT & mask)
    {
        cout << "\tsource rect = x:" << properties->sourceX
                << ", y:" << properties->sourceY
                << ", width:" << properties->sourceWidth
                << ", height:" << properties->sourceHeight
                << "\n";
    }

    if (ILM_NOTIFICATION_DEST_RECT & mask)
    {
        cout << "\tdest rect = x:" << properties->destX
                << ", y:" << properties->destY
                << ", width:" << properties->destWidth
                << ", height:" << properties->destHeight
                << "\n";
    }
}

void testNotificationLayer(t_ilm_layer layerid)
{
    cout << "Setup notification for layer " << layerid << " \n";

    ilmErrorTypes callResult = ilm_layerAddNotification(layerid, layerNotificationCallback);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to add notification callback to layer with ID " << layerid << "\n";
        return;
    }

    for (int i = 0; i < 2; ++i)
    {
        usleep(100 * 1000);
        cout << "Set layer 1000 visbility to FALSE\n";
        ilm_layerSetVisibility(layerid, ILM_FALSE);
        ilm_commitChanges();

        usleep(100 * 1000);
        cout << "Set layer 1000 visbility to TRUE\n";
        ilm_layerSetVisibility(layerid, ILM_TRUE);

        cout << "Set layer 1000 opacity to 0.3\n";
        ilm_layerSetOpacity(layerid, 0.3);
        ilm_commitChanges();

        usleep(100 * 1000);
        cout << "Set layer 1000 opacity to 1.0\n";
        ilm_layerSetOpacity(layerid, 1.0);
        ilm_commitChanges();
    }

    ilm_commitChanges(); // make sure, app lives long enough to receive last notification
}

void watchLayer(unsigned int* layerids, unsigned int layeridCount)
{
    for (unsigned int i = 0; i < layeridCount; ++i)
    {
        unsigned int layerid = layerids[i];
        cout << "Setup notification for layer " << layerid << "\n";

        ilmErrorTypes callResult = ilm_layerAddNotification(layerid, layerNotificationCallback);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to add notification callback to layer with ID " << layerid << "\n";
            return;
        }
    }

    cout << "Waiting for notifications...\n";
    int block;
    cin >> block;

    for (unsigned int i = 0; i < layeridCount; ++i)
    {
        unsigned int layerid = layerids[i];
        cout << "Removing notification for layer " << layerid << "\n";

        ilmErrorTypes callResult = ilm_layerRemoveNotification(layerid);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to remove notification callback of layer with ID " << layerid << "\n";
            return;
        }
    }

    if (layerids)
    {
        delete[] layerids;
    }
}

void surfaceNotificationCallback(t_ilm_layer surface, struct ilmSurfaceProperties* properties, t_ilm_notification_mask mask)
{
    cout << "\nNotification: surface " << surface << " updated properties:\n";

    if (ILM_NOTIFICATION_VISIBILITY & mask)
    {
        cout << "\tvisibility = " << properties->visibility << "\n";
    }

    if (ILM_NOTIFICATION_OPACITY & mask)
    {
        cout << "\topacity = " << properties->opacity << "\n";
    }

    if (ILM_NOTIFICATION_ORIENTATION & mask)
    {
        cout << "\torientation = " << properties->orientation << "\n";
    }

    if (ILM_NOTIFICATION_SOURCE_RECT & mask)
    {
        cout << "\tsource rect = x:" << properties->sourceX
                << ", y:" << properties->sourceY
                << ", width:" << properties->sourceWidth
                << ", height:" << properties->sourceHeight
                << "\n";
    }

    if (ILM_NOTIFICATION_DEST_RECT & mask)
    {
        cout << "\tdest rect = x:" << properties->destX
                << ", y:" << properties->destY
                << ", width:" << properties->destWidth
                << ", height:" << properties->destHeight
                << "\n";
    }
}

void watchSurface(unsigned int* surfaceids, unsigned int surfaceidCount)
{
    for (unsigned int i = 0; i < surfaceidCount; ++i)
    {
        unsigned int surfaceid = surfaceids[i];
        cout << "Setup notification for surface " << surfaceid << "\n";

        ilmErrorTypes callResult = ilm_surfaceAddNotification(surfaceid, surfaceNotificationCallback);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to add notification callback to surface with ID " << surfaceid << "\n";
            return;
        }
    }

    cout << "Waiting for notifications...\n";
    int block;
    cin >> block;

    for (unsigned int i = 0; i < surfaceidCount; ++i)
    {
        unsigned int surfaceid = surfaceids[i];
        cout << "Removing notification for surface " << surfaceid << "\n";

        ilmErrorTypes callResult = ilm_surfaceRemoveNotification(surfaceid);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to remove notification callback of surface with ID " << surfaceid << "\n";
            return;
        }
    }

    if (surfaceids)
    {
        delete[] surfaceids;
    }
}
