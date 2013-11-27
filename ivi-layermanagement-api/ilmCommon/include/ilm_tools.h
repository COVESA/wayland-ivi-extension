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
#ifndef _ILM_TOOLS_H_
#define _ILM_TOOLS_H_

#include "ilm_configuration.h"
#include "ilm_types.h"
#include <pthread.h>
#include <semaphore.h>

/*
 *=============================================================================
 * Implementation of thread-safe circular queue for local use
 *=============================================================================
 */
typedef struct
{
    pthread_mutex_t queueMutex;
    sem_t readBlockSemaphore;

    t_ilm_uint size;
    t_ilm_uint maxSize;
    t_ilm_uint readPos;
    t_ilm_uint writePos;

    t_ilm_message* messages;
} t_ilm_msg_queue;

extern t_ilm_msg_queue notificationQueue;
extern t_ilm_msg_queue incomingQueue;
extern pthread_mutex_t gSendReceiveLock;

void init_msg_queue(t_ilm_msg_queue* pQueue, t_ilm_uint maxSize);
t_ilm_bool msg_enqueue(t_ilm_msg_queue* pQueue, t_ilm_message message);
t_ilm_message msg_dequeue(t_ilm_msg_queue* pQueue);
void destroy_msg_queue(t_ilm_msg_queue* pQueue);

/*
 *=============================================================================
 * notification callback management
 *=============================================================================
 */
struct LayerCallback
{
    t_ilm_uint id;
    layerNotificationFunc callback;
};

struct SurfaceCallback
{
    t_ilm_uint id;
    surfaceNotificationFunc callback;
};

void initNotificationCallbacks();
layerNotificationFunc getLayerNotificationCallback(t_ilm_layer layer);
surfaceNotificationFunc getSurfaceNotificationCallback(t_ilm_surface surface);
t_ilm_bool findLayerCallback(t_ilm_layer layer);
t_ilm_bool addLayerCallback(t_ilm_layer layer, layerNotificationFunc func);
t_ilm_bool findSurfaceCallback(t_ilm_surface surface);
t_ilm_bool addSurfaceCallback(t_ilm_surface surface, surfaceNotificationFunc func);
void removeLayerCallback(t_ilm_layer layer);
void removeSurfaceCallback(t_ilm_surface layer);

/*
 *=============================================================================
 * internal thread management
 *=============================================================================
 */
void* notificationThreadLoop(void* param);
void* receiveThreadLoop(void* param);

/*
 *=============================================================================
 * receive with timeout
 *=============================================================================
 */
void calculateTimeout(struct timeval* currentTime, int giventimeout, struct timespec* timeout);
t_ilm_bool sendAndWaitForResponse(t_ilm_message command, t_ilm_message* response, int timeoutInMs, ilmErrorTypes* error);

#endif /* _ILM_TOOLS_H_ */
