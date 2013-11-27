/***************************************************************************
*
* Copyright 2013 BMW Car IT GmbH
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
#ifndef _ILM_CONFIGURATION_H_
#define _ILM_CONFIGURATION_H_

#include "IpcModuleLoader.h"

/*
 * in ms, negative value for infinite
 */
#define RECEIVE_TIMEOUT_IN_MS -1

/*
 * in ms
 */
#define RESPONSE_TIMEOUT_IN_MS 500

/*
 * must be same as GraphicalObject::INVALID_ID, but this is defined in C++
 * and can not be used here
 */
#define INVALID_ID 0xFFFFFFFF

/*
 * maximum number of registered notification callbacks
 */
#define MAX_CALLBACK_COUNT 16

/*
 * internal thread synchronized queue
 */
#define MAX_THREAD_SYNC_QUEUE_SIZE 4

/*
 * exported from ilm_common.h, shared for all client APIs
 */
extern struct IpcModule gIpcModule;

#endif /* _ILM_CONFIGURATION_H_ */
