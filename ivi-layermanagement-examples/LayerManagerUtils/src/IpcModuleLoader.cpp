/**************************************************************************
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
#include "IpcModuleLoader.h"
#include "IpcModule.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <dirent.h> /* DIR*/
#include <string.h> /* strcpy, strcat, strstr*/

/*
=============================================================================
 global variables
=============================================================================
*/
const char* gDefaultPluginLookupPath = CMAKE_INSTALL_PREFIX"/lib/layermanager";
const char* gCommunicatorPluginDirectory = "ipcmodules";

/*
=============================================================================
 plugin loading
=============================================================================
*/
t_ilm_bool loadSymbolTable(struct IpcModule* ipcModule, char* path, char* file)
{
    struct ApiFunction
    {
        const char* name;
        void** funcPtr;
    };

    struct ApiFunction ApiFunctionTable[] =
    {
        { "initClientMode", (void**)&ipcModule->initClientMode },
        { "initServiceMode", (void**)&ipcModule->initServiceMode },

        { "createMessage", (void**)&ipcModule->createMessage },
        { "createResponse", (void**)&ipcModule->createResponse },
        { "createErrorResponse", (void**)&ipcModule->createErrorResponse },
        { "createNotification", (void**)&ipcModule->createNotification },

        { "appendBool", (void**)&ipcModule->appendBool },
        { "appendDouble", (void**)&ipcModule->appendDouble },
        { "appendString", (void**)&ipcModule->appendString },
        { "appendInt", (void**)&ipcModule->appendInt },
        { "appendIntArray", (void**)&ipcModule->appendIntArray },
        { "appendUint", (void**)&ipcModule->appendUint },
        { "appendUintArray", (void**)&ipcModule->appendUintArray },

        { "sendToClients", (void**)&ipcModule->sendToClients },
        { "sendToService", (void**)&ipcModule->sendToService },

        { "receive", (void**)&ipcModule->receive },

        { "getMessageName", (void**)&ipcModule->getMessageName },
        { "getMessageType", (void**)&ipcModule->getMessageType },
        { "getSenderName", (void**)&ipcModule->getSenderName },
        { "getSenderHandle", (void**)&ipcModule->getSenderHandle },

        { "getBool", (void**)&ipcModule->getBool },
        { "getDouble", (void**)&ipcModule->getDouble },
        { "getString", (void**)&ipcModule->getString },
        { "getInt", (void**)&ipcModule->getInt },
        { "getIntArray", (void**)&ipcModule->getIntArray },
        { "getUint", (void**)&ipcModule->getUint },
        { "getUintArray", (void**)&ipcModule->getUintArray },

        { "destroyMessage", (void**)&ipcModule->destroyMessage },

        { "destroy", (void**)&ipcModule->destroy }
    };

    const unsigned int apiFunctionCount = sizeof(ApiFunctionTable) / sizeof(struct ApiFunction);
    unsigned int symbolCount = 0;
    t_ilm_bool returnValue = ILM_FALSE;
    void* pluginLibHandle = 0;
    char fullFilePath[1024];
    fullFilePath[0] = '\0';

    snprintf(fullFilePath, sizeof(fullFilePath), "%s/%s", path, file);

    pluginLibHandle = dlopen(fullFilePath, RTLD_LAZY);

    if (pluginLibHandle)
    {
        unsigned int i = 0;
        for (i = 0; i < apiFunctionCount; ++i)
        {
            struct ApiFunction* func = &ApiFunctionTable[i];

            *func->funcPtr = dlsym(pluginLibHandle, func->name);
            if (*func->funcPtr)
            {
                symbolCount++;
                /*printf("[ OK ] symbol %s\n", func->name);*/
            }
            else
            {
                /*printf("[FAIL] symbol %s\n", func->name);*/
            }
        }
    }

    if (symbolCount == apiFunctionCount)
    {
        returnValue = ILM_TRUE;
    }
    else
    {
        printf("Error in %s: found %d symbols, expected %d symbols.\n", fullFilePath, symbolCount, apiFunctionCount);
        if (0 != errno)
        {
            printf("--> error: %s\n", strerror(errno));
        }
        printf("--> not a valid ipc module\n");
        if (pluginLibHandle)
        {
            dlclose(pluginLibHandle);
        }
    }

    /*
     * Note: will break plugin. must be done during shutdown,
     * but currently there is no unloadIpcModule
     * dlclose(pluginLibHandle);
     */

    return returnValue;
}

t_ilm_bool loadIpcModule(struct IpcModule* communicator)
{
    t_ilm_bool result = ILM_FALSE;
    char path[1024];
    DIR *directory;

    /* find communicator client plugin*/
    char* pluginLookupPath = getenv("LM_PLUGIN_PATH");
    if (pluginLookupPath)
    {
        gDefaultPluginLookupPath = pluginLookupPath;
    }

    snprintf(path, sizeof(path), "%s/%s", gDefaultPluginLookupPath,
                                          gCommunicatorPluginDirectory);

     /* open directory*/
    directory = opendir(path);
    if (directory)
    {
        /* iterate content of directory*/
        struct dirent *itemInDirectory = 0;
        while ((itemInDirectory = readdir(directory)) && !result)
        {
            char* fileName = itemInDirectory->d_name;

            if (strstr(fileName, ".so"))
            {
                result = loadSymbolTable(communicator, path, fileName);
            }
        }

        closedir(directory);
    }
    else
    {
        printf("IpcModuleLoader: Error opening plugin dir %s\n", path);
    }

    return result;
}
