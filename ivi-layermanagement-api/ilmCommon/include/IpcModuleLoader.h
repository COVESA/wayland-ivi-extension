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
#ifndef __IPCMODULELOADER_H_
#define __IPCMODULELOADER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#include "ilm_types.h"

struct IpcModule
{
    t_ilm_bool (*initClientMode)();
    t_ilm_bool (*initServiceMode)();

    t_ilm_message (*createMessage)(t_ilm_const_string);
    t_ilm_message (*createResponse)(t_ilm_message);
    t_ilm_message (*createErrorResponse)(t_ilm_message);
    t_ilm_message (*createNotification)(t_ilm_const_string);

    t_ilm_bool (*appendBool)(t_ilm_message, const t_ilm_bool);
    t_ilm_bool (*appendDouble)(t_ilm_message, const double);
    t_ilm_bool (*appendString)(t_ilm_message, const char*);
    t_ilm_bool (*appendInt)(t_ilm_message, const int);
    t_ilm_bool (*appendIntArray)(t_ilm_message, const int*, int);
    t_ilm_bool (*appendUint)(t_ilm_message, const unsigned int);
    t_ilm_bool (*appendUintArray)(t_ilm_message, const unsigned int*, int);

    t_ilm_bool (*sendToClients)(t_ilm_message, t_ilm_client_handle*, int);
    t_ilm_bool (*sendToService)(t_ilm_message);

    t_ilm_message (*receive)(t_ilm_int); /* timeout in ms*/

    t_ilm_const_string (*getMessageName)(t_ilm_message);
    t_ilm_message_type (*getMessageType)(t_ilm_message);
    t_ilm_const_string (*getSenderName)(t_ilm_message);
    t_ilm_client_handle (*getSenderHandle)(t_ilm_message);

    t_ilm_bool (*getBool)(t_ilm_message, t_ilm_bool*);
    t_ilm_bool (*getDouble)(t_ilm_message, double*);
    t_ilm_bool (*getString)(t_ilm_message, char*);
    t_ilm_bool (*getInt)(t_ilm_message, int*);
    t_ilm_bool (*getIntArray)(t_ilm_message, int**, int*);
    t_ilm_bool (*getUint)(t_ilm_message, unsigned int*);
    t_ilm_bool (*getUintArray)(t_ilm_message, unsigned int**, int*);

    t_ilm_bool (*destroyMessage)(t_ilm_message);

    t_ilm_bool (*destroy)();
};

t_ilm_bool loadIpcModule(struct IpcModule* communicator);

#ifdef __cplusplus
}  /* extern "C"*/
#endif /* __cplusplus*/

#endif /* __IPCMODULELOADER_H_*/
