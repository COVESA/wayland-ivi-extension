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

#include "TestBase.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

extern "C" {
    #include "ilm_client.h"
    #include "ilm_control.h"
}

void add_nsecs(struct timespec *tv, long nsec)
{
   assert(nsec < 1000000000);

   tv->tv_nsec += nsec;
   if (tv->tv_nsec >= 1000000000)
   {
      tv->tv_nsec -= 1000000000;
      tv->tv_sec++;
   }
}

/* Tests with callbacks
 * For each test first set the global variables to point to where parameters of the callbacks are supposed to be placed.
 */
static pthread_mutex_t notificationMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  waiterVariable = PTHREAD_COND_INITIALIZER;
static int timesCalled=0;

class NotificationTest: public TestBase, public ::testing::Test {
    class PthreadMutexLock {
    private:
        pthread_mutex_t &mutex_;

        PthreadMutexLock(const PthreadMutexLock&);
        PthreadMutexLock& operator=(const PthreadMutexLock&);

    public:
        explicit PthreadMutexLock(pthread_mutex_t &mutex): mutex_(mutex) {
            pthread_mutex_lock(&mutex_);
        }
        ~PthreadMutexLock() {
            pthread_mutex_unlock(&mutex_);
        }
    };

public:
    void SetUp()
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_initWithNativedisplay((t_ilm_nativedisplay)wlDisplay));
        ASSERT_EQ(ILM_SUCCESS, ilmClient_init((t_ilm_nativedisplay)wlDisplay));

        // set default values
        callbackLayerId = -1;
        LayerProperties = ilmLayerProperties();
        mask = static_cast<t_ilm_notification_mask>(0);
        surface = -1;
        SurfaceProperties = ilmSurfaceProperties();
        // create a layer
        layer = 345;
        ilm_layerRemove(layer);
        ilm_commitChanges();
        ilm_layerCreateWithDimension(&layer, 800, 480);
        ilm_commitChanges();
        // create a surface
        surface = 456;
        ilm_surfaceRemove(surface);
        ilm_commitChanges();
        ilm_surfaceCreate((t_ilm_nativehandle)wlSurfaces[0],10,10,ILM_PIXELFORMAT_RGBA_8888,&surface);
        ilm_commitChanges();
        timesCalled=0;

        callbackLayerId = INVALID_ID;
        callbackSurfaceId = INVALID_ID;
    }

    void TearDown()
    {
       //print_lmc_get_scene();
       t_ilm_layer* layers = NULL;
       t_ilm_int numLayer=0;
       EXPECT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&numLayer, &layers));
       for (t_ilm_int i=0; i<numLayer; i++)
       {
           EXPECT_EQ(ILM_SUCCESS, ilm_layerRemove(layers[i]));
       };
       free(layers);

       t_ilm_surface* surfaces = NULL;
       t_ilm_int numSurfaces=0;
       EXPECT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&numSurfaces, &surfaces));
       for (t_ilm_int i=0; i<numSurfaces; i++)
       {
           EXPECT_EQ(ILM_SUCCESS, ilm_surfaceRemove(surfaces[i]));
       };
       free(surfaces);

       EXPECT_EQ(ILM_SUCCESS, ilm_commitChanges());
       EXPECT_EQ(ILM_SUCCESS, ilmClient_destroy());
       EXPECT_EQ(ILM_SUCCESS, ilm_destroy());
    }

    NotificationTest(){}

    ~NotificationTest(){}

    t_ilm_uint layer;

    // Pointers where to put received values for current Test
    static t_ilm_layer callbackLayerId;
    static t_ilm_surface callbackSurfaceId;
    static struct ilmLayerProperties LayerProperties;
    static unsigned int mask;
    static t_ilm_surface surface;
    static ilmSurfaceProperties SurfaceProperties;

    static void assertCallbackcalled(int numberOfExpectedCalls=1){
        static struct timespec theTime;
        clock_gettime(CLOCK_REALTIME, &theTime);
        add_nsecs(&theTime, 500000000);
        PthreadMutexLock lock(notificationMutex);
        int status = 0;
        do {
            if (numberOfExpectedCalls!=timesCalled) {
                status = pthread_cond_timedwait( &waiterVariable, &notificationMutex, &theTime);
            }
        } while (status!=ETIMEDOUT && numberOfExpectedCalls!=timesCalled);

        // we cannot rely on a timeout as layer callbacks are always called synchronously on ilm_commitChanges()
        EXPECT_NE(ETIMEDOUT, status);
        timesCalled=0;
    }

    static void assertNoCallbackIsCalled(){
        struct timespec theTime;
        clock_gettime(CLOCK_REALTIME, &theTime);
        add_nsecs(&theTime, 500000000);
        PthreadMutexLock lock(notificationMutex);
        // assert that we have not been notified
        ASSERT_EQ(ETIMEDOUT, pthread_cond_timedwait( &waiterVariable, &notificationMutex, &theTime));
    }

    static void LayerCallbackFunction(t_ilm_layer layer, struct ilmLayerProperties* layerProperties, t_ilm_notification_mask m)
    {
        PthreadMutexLock lock(notificationMutex);

        if ((unsigned)m & ILM_NOTIFICATION_VISIBILITY)
        {
            LayerProperties.visibility = layerProperties->visibility;
        }

        if ((unsigned)m & ILM_NOTIFICATION_OPACITY)
        {
            LayerProperties.opacity = layerProperties->opacity;
        }

        if ((unsigned)m & ILM_NOTIFICATION_ORIENTATION)
        {
            LayerProperties.orientation = layerProperties->orientation;
        }

        if ((unsigned)m & ILM_NOTIFICATION_SOURCE_RECT)
        {
            LayerProperties.sourceX = layerProperties->sourceX;
            LayerProperties.sourceY = layerProperties->sourceY;
            LayerProperties.sourceWidth = layerProperties->sourceWidth;
            LayerProperties.sourceHeight = layerProperties->sourceHeight;
        }

        if ((unsigned)m & ILM_NOTIFICATION_DEST_RECT)
        {
            LayerProperties.destX = layerProperties->destX;
            LayerProperties.destY = layerProperties->destY;
            LayerProperties.destWidth = layerProperties->destWidth;
            LayerProperties.destHeight = layerProperties->destHeight;
        }

        EXPECT_TRUE(callbackLayerId == (unsigned)-1 || callbackLayerId == layer);
        callbackLayerId = layer;
        mask |= (unsigned)m;
        timesCalled++;

        pthread_cond_signal( &waiterVariable );
    }

    static void SurfaceCallbackFunction(t_ilm_surface surface, struct ilmSurfaceProperties* surfaceProperties, t_ilm_notification_mask m)
    {
        PthreadMutexLock lock(notificationMutex);

        if ((unsigned)m & ILM_NOTIFICATION_VISIBILITY)
        {
            SurfaceProperties.visibility = surfaceProperties->visibility;
        }

        if ((unsigned)m & ILM_NOTIFICATION_OPACITY)
        {
            SurfaceProperties.opacity = surfaceProperties->opacity;
        }

        if ((unsigned)m & ILM_NOTIFICATION_ORIENTATION)
        {
            SurfaceProperties.orientation = surfaceProperties->orientation;
        }

        if ((unsigned)m & ILM_NOTIFICATION_SOURCE_RECT)
        {
            SurfaceProperties.sourceX = surfaceProperties->sourceX;
            SurfaceProperties.sourceY = surfaceProperties->sourceY;
            SurfaceProperties.sourceWidth = surfaceProperties->sourceWidth;
            SurfaceProperties.sourceHeight = surfaceProperties->sourceHeight;
        }

        if ((unsigned)m & ILM_NOTIFICATION_DEST_RECT)
        {
            SurfaceProperties.destX = surfaceProperties->destX;
            SurfaceProperties.destY = surfaceProperties->destY;
            SurfaceProperties.destWidth = surfaceProperties->destWidth;
            SurfaceProperties.destHeight = surfaceProperties->destHeight;
        }

        EXPECT_TRUE(callbackSurfaceId == (unsigned)-1 || callbackSurfaceId == surface);
        callbackSurfaceId = surface;
        mask |= (unsigned)m;
        timesCalled++;

        pthread_cond_signal( &waiterVariable );
    }
};

// Pointers where to put received values for current Test
t_ilm_layer NotificationTest::callbackLayerId;
t_ilm_surface NotificationTest::callbackSurfaceId;
struct ilmLayerProperties NotificationTest::LayerProperties;
unsigned int NotificationTest::mask;
t_ilm_surface NotificationTest::surface;
ilmSurfaceProperties NotificationTest::SurfaceProperties;




TEST_F(NotificationTest, ilm_layerAddNotificationWithoutCallback)
{
    // create a layer
    t_ilm_uint layer = 89;

    ASSERT_EQ(ILM_SUCCESS,ilm_layerCreateWithDimension(&layer, 800, 480));
    ASSERT_EQ(ILM_SUCCESS,ilm_commitChanges());
    // add notification

    printf("test calling ilm_layerAddNotification\n");
    //ASSERT_EQ(ILM_SUCCESS,ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(layer,NULL));

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemove(layer));
    ASSERT_EQ(ILM_SUCCESS,ilm_commitChanges());

}

TEST_F(NotificationTest, ilm_surfaceAddNotificationWithoutCallback)
{
    // create a layer
    t_ilm_uint surface = 67;

    ilm_surfaceCreate((t_ilm_nativehandle)wlSurfaces[1],10,10,ILM_PIXELFORMAT_RGBA_8888,&surface);
    ilm_commitChanges();

    // add notification
    ilmErrorTypes status = ilm_surfaceAddNotification(surface,NULL);
    ASSERT_EQ(ILM_SUCCESS, status);

    ilm_surfaceRemove(surface);
    ilm_commitChanges();
}

TEST_F(NotificationTest, NotifyOnLayerSetDestinationRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    ilm_layerSetDestinationRectangle(layer,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(33u,LayerProperties.destX);
    EXPECT_EQ(567u,LayerProperties.destY);
    EXPECT_EQ(55u,LayerProperties.destWidth);
    EXPECT_EQ(99u,LayerProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerSetOpacity)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_NEAR(0.789, LayerProperties.opacity, 0.1);
    EXPECT_EQ(ILM_NOTIFICATION_OPACITY,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerSetOrientation)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    e_ilmOrientation orientation = ILM_ONEHUNDREDEIGHTY;
    ilm_layerSetOrientation(layer,orientation);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,LayerProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_ORIENTATION,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerSetSourceRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(33u,LayerProperties.sourceX);
    EXPECT_EQ(567u,LayerProperties.sourceY);
    EXPECT_EQ(55u,LayerProperties.sourceWidth);
    EXPECT_EQ(99u,LayerProperties.sourceHeight);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerSetVisibility)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_bool value = ILM_TRUE;
    ilm_layerSetVisibility(layer,value);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_TRUE(LayerProperties.visibility);
    EXPECT_EQ(ILM_NOTIFICATION_VISIBILITY,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerMultipleValues1)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(33u,LayerProperties.sourceX);
    EXPECT_EQ(567u,LayerProperties.sourceY);
    EXPECT_EQ(55u,LayerProperties.sourceWidth);
    EXPECT_EQ(99u,LayerProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,LayerProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerMultipleValues2)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetDestinationRectangle(layer,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(3);

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_TRUE(LayerProperties.visibility);
    EXPECT_NEAR(0.789, LayerProperties.opacity, 0.1);
    EXPECT_EQ(33u,LayerProperties.destX);
    EXPECT_EQ(567u,LayerProperties.destY);
    EXPECT_EQ(55u,LayerProperties.destWidth);
    EXPECT_EQ(99u,LayerProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_OPACITY,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerAllValues)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetDestinationRectangle(layer,133,1567,155,199);
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(5);

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(33u,LayerProperties.sourceX);
    EXPECT_EQ(567u,LayerProperties.sourceY);
    EXPECT_EQ(55u,LayerProperties.sourceWidth);
    EXPECT_EQ(99u,LayerProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,LayerProperties.orientation);

    EXPECT_TRUE(LayerProperties.visibility);
    EXPECT_NEAR(opacity, LayerProperties.opacity, 0.1);
    EXPECT_EQ(133u,LayerProperties.destX);
    EXPECT_EQ(1567u,LayerProperties.destY);
    EXPECT_EQ(155u,LayerProperties.destWidth);
    EXPECT_EQ(199u,LayerProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_OPACITY|ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, DoNotSendNotificationsAfterRemoveLayer)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // get called once
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    assertCallbackcalled();

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetDestinationRectangle(layer,133,1567,155,199);
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();

}

TEST_F(NotificationTest, MultipleRegistrationsLayer)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // get called once
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    assertCallbackcalled();

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetDestinationRectangle(layer,133,1567,155,199);
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();

    // register for notifications again
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));

    ilm_layerSetOrientation(layer,ILM_ZERO);
    ilm_commitChanges();
    assertCallbackcalled();

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, DefaultIsNotToReceiveNotificationsLayer)
{
    // get called once
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_layerSetOpacity(layer,opacity);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetDestinationRectangle(layer,133,1567,155,199);
    ilm_layerSetSourceRectangle(layer,33,567,55,99);
    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();
}

TEST_F(NotificationTest, NotifyOnSurfaceSetDestinationRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    ilm_surfaceSetDestinationRectangle(surface,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.destX);
    EXPECT_EQ(567u,SurfaceProperties.destY);
    EXPECT_EQ(55u,SurfaceProperties.destWidth);
    EXPECT_EQ(99u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetOpacity)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_NEAR(0.789, SurfaceProperties.opacity, 0.1);
    EXPECT_EQ(ILM_NOTIFICATION_OPACITY|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetOrientation)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    e_ilmOrientation orientation = ILM_ONEHUNDREDEIGHTY;
    ilm_surfaceSetOrientation(surface,orientation);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_ORIENTATION|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetSourceRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetVisibility)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_bool value = ILM_TRUE;
    ilm_surfaceSetVisibility(surface,value);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(2);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_EQ(ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceMultipleValues1)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(3);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceMultipleValues2)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetDestinationRectangle(surface,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(4);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_NEAR(0.789, SurfaceProperties.opacity, 0.1);
    EXPECT_EQ(33u,SurfaceProperties.destX);
    EXPECT_EQ(567u,SurfaceProperties.destY);
    EXPECT_EQ(55u,SurfaceProperties.destWidth);
    EXPECT_EQ(99u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|
              ILM_NOTIFICATION_OPACITY|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceAllValues)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetDestinationRectangle(surface,133,1567,155,199);
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled(6);

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);

    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_NEAR(opacity, SurfaceProperties.opacity, 0.1);
    EXPECT_EQ(133u,SurfaceProperties.destX);
    EXPECT_EQ(1567u,SurfaceProperties.destY);
    EXPECT_EQ(155u,SurfaceProperties.destWidth);
    EXPECT_EQ(199u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_OPACITY|
              ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION|ILM_NOTIFICATION_CONTENT_AVAILABLE,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, DoNotSendNotificationsAfterRemoveSurface)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // get called once
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    assertCallbackcalled(2);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetDestinationRectangle(surface,133,1567,155,199);
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();

}

TEST_F(NotificationTest, MultipleRegistrationsSurface)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // get called once
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    assertCallbackcalled(2);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetDestinationRectangle(surface,133,1567,155,199);
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();

    // register for notifications again
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));

    ilm_surfaceSetOrientation(surface,ILM_ZERO);
    ilm_commitChanges();
    assertCallbackcalled(2);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, DefaultIsNotToReceiveNotificationsSurface)
{
    // get called once
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // change a lot of things
    t_ilm_float opacity = 0.789;
    ilm_surfaceSetOpacity(surface,opacity);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetDestinationRectangle(surface,133,1567,155,199);
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();

    // assert that we have not been notified
    assertNoCallbackIsCalled();
}
