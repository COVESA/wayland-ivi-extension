/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

#ifndef __THREADBASE_H__
#define __THREADBASE_H__

#include "ilm_types.h"
#include <pthread.h>

class ThreadBase
{
public:
    ThreadBase();
    virtual ~ThreadBase();

    t_ilm_bool threadCreate();
    t_ilm_bool threadInit();
    t_ilm_bool threadStart();
    t_ilm_bool threadStop();

    pthread_t threadGetId() const;
    t_ilm_bool threadIsRunning();

    // override in inheriting class if required
    virtual t_ilm_bool threadInitInThreadContext();
    virtual t_ilm_bool threadDestroyInThreadContext();

    // implement in inheriting class
    virtual t_ilm_bool threadMainLoop() = 0;

private:
    static unsigned int mGlobalThreadCount;
    pthread_t mThreadId;
    pthread_mutex_t mInitLock;
    pthread_mutex_t mRunLock;
    t_ilm_bool mRunning;
};

#endif // __THREADBASE_H__
