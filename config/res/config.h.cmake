/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 Auto-generated. Do not modify.
 Variables configured by CMake build system
*/

/*
-----------------------------------------------------------------------------
 string variables
-----------------------------------------------------------------------------
*/
/* version of the LayerManagementService */
#define ILM_VERSION          "${ILM_VERSION}"

/* CMake build type, e.g. Debug, Release */
#define CMAKE_BUILD_TYPE     "${CMAKE_BUILD_TYPE}"

/* compiler flags used to build project */
#define CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS}"

/* install prefix of target platform */
#define CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}"

/*
-----------------------------------------------------------------------------
 build flags
-----------------------------------------------------------------------------
*/
${EXPORTED_BUILD_FLAGS}

/*
-----------------------------------------------------------------------------
 human readable report
-----------------------------------------------------------------------------
*/
#define DEBUG_FLAG 1
#define INFO_FLAG  2

typedef struct
{
    int type;
    const char* description;
} BuildFlag;

const BuildFlag gBuildFlags[] =
{
    { INFO_FLAG,  "Build Version         = ${ILM_VERSION}" },
    { DEBUG_FLAG, "Build Type            = ${CMAKE_BUILD_TYPE}" },

${EXPORTED_BUILD_FLAG_ARRAY}
    { DEBUG_FLAG, "Install Prefix        = ${CMAKE_INSTALL_PREFIX}" }
};

const int gBuildFlagCount = sizeof(gBuildFlags) / sizeof(gBuildFlags[0]);

/*
-----------------------------------------------------------------------------
 manage list of statically linked plugins
-----------------------------------------------------------------------------
*/
#define REGISTER_PLUGIN(PLUGIN) \
    extern "C" IPlugin* create ## PLUGIN(ICommandExecutor& executor, Configuration& config); \
    static bool PLUGIN ## _instance = PluginManager::registerStaticPluginCreateFunction(create ## PLUGIN);

#define STATIC_PLUGIN_REGISTRATION ${STATIC_PLUGIN_REGISTRATION}

/*
-----------------------------------------------------------------------------
 define plugin entry point depending on build settings
-----------------------------------------------------------------------------
*/
#define DECLARE_LAYERMANAGEMENT_PLUGIN(name) \
extern "C" IPlugin* create ## name(ICommandExecutor& executor, Configuration& config) \
{ \
    return new name(executor, config); \
}

#endif /* __CONFIG_H__ */
