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

#include "ThreadBase.h"
#include "Log.h"
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

//===========================================================================
// internal data structures
//===========================================================================
struct ThreadArgument
{
    ThreadBase* obj;
    pthread_mutex_t* initLock;
    pthread_mutex_t* runLock;
    t_ilm_bool* running;
};

//===========================================================================
// static member initialization
//===========================================================================
unsigned int ThreadBase::mGlobalThreadCount = 1; // main thread already running

//===========================================================================
// global thread cleanup function (automatically called on thread shutwon)
//===========================================================================
static void threadBaseMainLoopCleanup(void* arg)
{
    // destroy in thread context
    ((ThreadArgument*)arg)->obj->threadDestroyInThreadContext();
    pthread_mutex_unlock(((ThreadArgument*)arg)->runLock);
    *(((ThreadArgument*)arg)->running) = ILM_FALSE;
    delete((ThreadArgument*)arg);
}

//===========================================================================
// global thread loop function
//===========================================================================
static void* threadBaseMainLoop(void* arg)
{
    if (!arg)
    {
        LOG_ERROR("ThreadBase", "invalid thread argument (arg)");
        return NULL;
    }

    if (!((ThreadArgument*)arg)->obj)
    {
        LOG_ERROR("ThreadBase", "invalid thread argument (obj)");
        return NULL;
    }

    if (!((ThreadArgument*)arg)->running)
    {
        LOG_ERROR("ThreadBase", "invalid thread argument (running)");
        return NULL;
    }

    // register cleanup function
    pthread_cleanup_push(threadBaseMainLoopCleanup, arg);

    // init in thread context
    *(((ThreadArgument*)arg)->running) = ((ThreadArgument*)arg)->obj->threadInitInThreadContext();

    // indicate initialization complete
    pthread_mutex_unlock(((ThreadArgument*)arg)->initLock);

    // wait for start permit
    pthread_mutex_lock(((ThreadArgument*)arg)->runLock);

    // run thread
    while (*(((ThreadArgument*)arg)->running))
    {
        *(((ThreadArgument*)arg)->running) = ((ThreadArgument*)arg)->obj->threadMainLoop();
    }

    // execute cleanup handlers
    pthread_cleanup_pop(1);

    return NULL;
}

//===========================================================================
// class implementation
//===========================================================================
ThreadBase::ThreadBase()
: mThreadId(0)
, mRunning(ILM_FALSE)
{
    pthread_mutex_init(&mRunLock, NULL);
    pthread_mutex_lock(&mRunLock);

    pthread_mutex_init(&mInitLock, NULL);
    pthread_mutex_lock(&mInitLock);
}

ThreadBase::~ThreadBase()
{
    pthread_mutex_unlock(&mInitLock);
    pthread_mutex_destroy(&mInitLock);

    pthread_mutex_unlock(&mRunLock);
    pthread_mutex_destroy(&mRunLock);
}

pthread_t ThreadBase::threadGetId() const
{
    return mThreadId;
}

t_ilm_bool ThreadBase::threadIsRunning()
{
    if (0 == pthread_mutex_trylock(&mRunLock))
    {
        pthread_mutex_unlock(&mRunLock);
        mRunning = ILM_FALSE;
    }
    if (0 != pthread_kill(mThreadId, 0))
    {
        mRunning = ILM_FALSE;
    }
    return mRunning;
}

t_ilm_bool ThreadBase::threadCreate()
{
    pthread_attr_t notificationThreadAttributes;
    pthread_attr_init(&notificationThreadAttributes);
    pthread_attr_setdetachstate(&notificationThreadAttributes,
                                PTHREAD_CREATE_JOINABLE);

    ThreadArgument* arg = new ThreadArgument();
    arg->obj = this;
    arg->runLock = &mRunLock;
    arg->initLock = &mInitLock;
    arg->running = &mRunning;

    int ret = pthread_create(&mThreadId,
                                &notificationThreadAttributes,
                                threadBaseMainLoop,
                                (void*)arg);
    if (0 != ret)
    {
        LOG_ERROR("ThreadBase", "Failed to start thread.");
    }
    else
    {
        ++mGlobalThreadCount;
        LOG_DEBUG("ThreadBase", "Started thread (now " << mGlobalThreadCount << ")");
        mRunning = ILM_TRUE;
    }

    return threadIsRunning();
}

t_ilm_bool ThreadBase::threadInit()
{
    return (0 == pthread_mutex_lock(&mInitLock)) ? ILM_TRUE : ILM_FALSE;
}

t_ilm_bool ThreadBase::threadStart()
{
    return (0 == pthread_mutex_unlock(&mRunLock)) ? ILM_TRUE : ILM_FALSE;
}

t_ilm_bool ThreadBase::threadStop()
{
    t_ilm_bool result = ILM_TRUE;

    mRunning = threadIsRunning();

    if (mRunning)
    {
        LOG_INFO("ThreadBase", "pthread_cancel");
        if (0 != pthread_cancel(mThreadId))
        {
            LOG_ERROR("ThreadBase", "Stopping thread failed (cancel)");
            result = ILM_FALSE;
        }
        else
        {
            LOG_INFO("ThreadBase", "pthread_join");
            if (0 != pthread_join(mThreadId, NULL))
            {
                LOG_ERROR("ThreadBase", "Stopping thread failed (join)");
                result = ILM_FALSE;
            }
        }
    }

    if (result)
    {
        --mGlobalThreadCount;
        LOG_DEBUG("ThreadBase", "Stopped thread (now " << mGlobalThreadCount << ")");
    }

    return result;
}

//=====================================================================================
// base implementation for overriding in inheriting classes
//=====================================================================================
t_ilm_bool ThreadBase::threadInitInThreadContext()
{
    LOG_DEBUG("ThreadBase", "threadInitInThreadContext (default implementation)");
    return ILM_TRUE;
}

t_ilm_bool ThreadBase::threadDestroyInThreadContext()
{
    LOG_DEBUG("ThreadBase", "threadDestroyInThreadContext (default implementation)");
    return ILM_TRUE;
}
