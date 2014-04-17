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

extern "C" {
    #include "ilm_client.h"
    #include "ilm_control.h"
}



/* Tests with callbacks
 * For each test first set the global variables to point to where parameters of the callbacks are supposed to be placed.
 */
static pthread_mutex_t notificationMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  waiterVariable = PTHREAD_COND_INITIALIZER;
static int timesCalled=0;

class NotificationTest: public TestBase, public ::testing::Test {
public:

    static void SetUpTestCase() {
        ilm_init();
     }
    static void TearDownTestCase() {
        ilm_destroy();
    }

    NotificationTest()
    {
        // set default values
        callbackLayerId = -1;
        LayerProperties = ilmLayerProperties();
        mask = ILM_NOTIFICATION_ALL;
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
        ilm_surfaceCreate((t_ilm_nativehandle)wlSurface,10,10,ILM_PIXELFORMAT_RGBA_8888,&surface);
        ilm_commitChanges();
        timesCalled=0;
    }

    ~NotificationTest(){}



    t_ilm_uint layer;

    // Pointers where to put received values for current Test
    static t_ilm_layer callbackLayerId;
    static t_ilm_layer callbackSurfaceId;
    static struct ilmLayerProperties LayerProperties;
    static t_ilm_notification_mask mask;
    static t_ilm_surface surface;
    static ilmSurfaceProperties SurfaceProperties;


    static void assertCallbackcalled(int numberOfExpectedCalls=1){
        static struct timespec theTime;
        clock_gettime(CLOCK_REALTIME, &theTime);
        theTime.tv_sec += 2;
        pthread_mutex_lock( &notificationMutex );
        int status = 0;
        do {
            if (numberOfExpectedCalls!=timesCalled){
                status = pthread_cond_timedwait( &waiterVariable, &notificationMutex, &theTime);
            }
        } while (status!=ETIMEDOUT && numberOfExpectedCalls!=timesCalled);
        ASSERT_NE(ETIMEDOUT, status);
        pthread_mutex_unlock( &notificationMutex );
        timesCalled=0;
    }

    static void assertNoCallbackIsCalled(){
        struct timespec theTime;
        clock_gettime(CLOCK_REALTIME, &theTime);
        theTime.tv_sec += 1;
        pthread_mutex_lock( &notificationMutex );
        // assert that we have not been notified
        ASSERT_EQ(ETIMEDOUT, pthread_cond_timedwait( &waiterVariable, &notificationMutex, &theTime));
        pthread_mutex_unlock( &notificationMutex );
    }


    static void LayerCallbackFunction(t_ilm_layer layer, struct ilmLayerProperties* LayerProperties, t_ilm_notification_mask mask)
    {
        pthread_mutex_lock( &notificationMutex );

        NotificationTest::callbackLayerId = layer;
        NotificationTest::LayerProperties = *LayerProperties;
        NotificationTest::mask = mask;
        timesCalled++;
        pthread_cond_signal( &waiterVariable );
        pthread_mutex_unlock( &notificationMutex );
    }

    static void SurfaceCallbackFunction(t_ilm_surface surface, struct ilmSurfaceProperties* surfaceProperties, t_ilm_notification_mask mask)
    {
        pthread_mutex_lock( &notificationMutex );

        NotificationTest::callbackSurfaceId = surface;
        NotificationTest::SurfaceProperties = *surfaceProperties;
        NotificationTest::mask = mask;
        timesCalled++;
        pthread_cond_signal( &waiterVariable );
        pthread_mutex_unlock( &notificationMutex );
    }
};

// Pointers where to put received values for current Test
t_ilm_layer NotificationTest::callbackLayerId;
t_ilm_layer NotificationTest::callbackSurfaceId;
struct ilmLayerProperties NotificationTest::LayerProperties;
t_ilm_notification_mask NotificationTest::mask;
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

    ilm_surfaceCreate((t_ilm_nativehandle)wlSurface,10,10,ILM_PIXELFORMAT_RGBA_8888,&surface);
    ilm_commitChanges();

    // add notification
    ilmErrorTypes status = ilm_surfaceAddNotification(surface,NULL);
    ASSERT_EQ(ILM_SUCCESS, status);

    ilm_surfaceRemove(surface);
    ilm_commitChanges();
}

// ######### LAYERS
TEST_F(NotificationTest, NotifyOnLayerSetPosition)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_uint* pos = new t_ilm_uint[2];
    pos[0] = 7;
    pos[1] = 2;
    ilm_layerSetPosition(layer,pos);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(7u,LayerProperties.destX);
    EXPECT_EQ(2u,LayerProperties.destY);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
}

TEST_F(NotificationTest, NotifyOnLayerSetDimension)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_layerAddNotification(layer,&LayerCallbackFunction));
    // change something
    t_ilm_uint* pos = new t_ilm_uint[2];
    pos[0] = 70;
    pos[1] = 22;
    ilm_layerSetDimension(layer,pos);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(70u,LayerProperties.destWidth);
    EXPECT_EQ(22u,LayerProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_layerRemoveNotification(layer));
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
    EXPECT_FLOAT_EQ(0.789,LayerProperties.opacity);
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
    assertCallbackcalled();

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
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_TRUE(LayerProperties.visibility);
    EXPECT_FLOAT_EQ(0.789,LayerProperties.opacity);
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
    assertCallbackcalled();

    EXPECT_EQ(layer,callbackLayerId);
    EXPECT_EQ(33u,LayerProperties.sourceX);
    EXPECT_EQ(567u,LayerProperties.sourceY);
    EXPECT_EQ(55u,LayerProperties.sourceWidth);
    EXPECT_EQ(99u,LayerProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,LayerProperties.orientation);

    EXPECT_TRUE(LayerProperties.visibility);
    EXPECT_FLOAT_EQ(opacity,LayerProperties.opacity);
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

// ######## SURFACES
TEST_F(NotificationTest, NotifyOnSurfaceSetPosition)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_uint* pos = new t_ilm_uint[2];
    pos[0] = 7;
    pos[1] = 2;
    ilm_surfaceSetPosition(surface,pos);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(7u,SurfaceProperties.destX);
    EXPECT_EQ(2u,SurfaceProperties.destY);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetDimension)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    t_ilm_uint* pos = new t_ilm_uint[2];
    pos[0] = 70;
    pos[1] = 22;
    ilm_surfaceSetDimension(surface,pos);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(70u,SurfaceProperties.destWidth);
    EXPECT_EQ(22u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetDestinationRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    ilm_surfaceSetDestinationRectangle(surface,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.destX);
    EXPECT_EQ(567u,SurfaceProperties.destY);
    EXPECT_EQ(55u,SurfaceProperties.destWidth);
    EXPECT_EQ(99u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_FLOAT_EQ(0.789,SurfaceProperties.opacity);
    EXPECT_EQ(ILM_NOTIFICATION_OPACITY,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_ORIENTATION,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, NotifyOnSurfaceSetSourceRectangle)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // change something
    ilm_surfaceSetSourceRectangle(surface,33,567,55,99);
    ilm_commitChanges();

    // expect callback to have been called
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_EQ(ILM_NOTIFICATION_VISIBILITY,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);
    EXPECT_EQ(ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_FLOAT_EQ(0.789,SurfaceProperties.opacity);
    EXPECT_EQ(33u,SurfaceProperties.destX);
    EXPECT_EQ(567u,SurfaceProperties.destY);
    EXPECT_EQ(55u,SurfaceProperties.destWidth);
    EXPECT_EQ(99u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_OPACITY,mask);

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
    assertCallbackcalled();

    EXPECT_EQ(surface,callbackSurfaceId);
    EXPECT_EQ(33u,SurfaceProperties.sourceX);
    EXPECT_EQ(567u,SurfaceProperties.sourceY);
    EXPECT_EQ(55u,SurfaceProperties.sourceWidth);
    EXPECT_EQ(99u,SurfaceProperties.sourceHeight);
    EXPECT_EQ(ILM_ONEHUNDREDEIGHTY,SurfaceProperties.orientation);

    EXPECT_TRUE(SurfaceProperties.visibility);
    EXPECT_FLOAT_EQ(opacity,SurfaceProperties.opacity);
    EXPECT_EQ(133u,SurfaceProperties.destX);
    EXPECT_EQ(1567u,SurfaceProperties.destY);
    EXPECT_EQ(155u,SurfaceProperties.destWidth);
    EXPECT_EQ(199u,SurfaceProperties.destHeight);
    EXPECT_EQ(ILM_NOTIFICATION_DEST_RECT|ILM_NOTIFICATION_VISIBILITY|ILM_NOTIFICATION_OPACITY|ILM_NOTIFICATION_SOURCE_RECT|ILM_NOTIFICATION_ORIENTATION,mask);

    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceRemoveNotification(surface));
}

TEST_F(NotificationTest, DoNotSendNotificationsAfterRemoveSurface)
{
    ASSERT_EQ(ILM_SUCCESS,ilm_surfaceAddNotification(surface,&SurfaceCallbackFunction));
    // get called once
    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    assertCallbackcalled();

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
    assertCallbackcalled();

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
    assertCallbackcalled();

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
