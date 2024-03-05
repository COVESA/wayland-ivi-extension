/***************************************************************************
 *
 * Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
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

#include <gtest/gtest.h>
#include "wayland-util.h"
#include "ilm_control.h"
#include "ilm_control_platform.h"
#include "client_api_fake.h"
#include "ivi-wm-client-protocol.h"
#include "ivi-input-client-protocol.h"

extern "C"{
const struct wl_interface ivi_screenshot_interface, \
                          ivi_wm_screen_interface, \
                          ivi_wm_interface, \
                          ivi_input_interface, \
                          wl_registry_interface, \
                          wl_buffer_interface, \
                          wl_shm_pool_interface, \
                          wl_shm_interface, \
                          wl_output_interface;

FAKE_VALUE_FUNC(int, save_as_png, const char *, const char *, int32_t , int32_t , uint32_t );
FAKE_VALUE_FUNC(int, save_as_bitmap, const char *, const char *, int32_t , int32_t , uint32_t );

#include "ilm_control_wayland_platform.c"
}

struct screenshot_data_t {
    std::atomic<int32_t> fd;
    std::atomic<uint32_t> error;
    ilmErrorTypes result = ILM_SUCCESS;
};

enum ilmControlStatus
{
    CREATE_LAYER    = 0,
    DESTROY_LAYER   = 1,
    CREATE_SURFACE  = 2,
    DESTROY_SURFACE = 3,
    NONE = 4
};

static constexpr uint8_t MAX_NUMBER = 5;

static ilmControlStatus g_ilmControlStatus = NONE;
static t_ilm_notification_mask g_ilm_notification_mask;

static void notificationCallback(ilmObjectType object, t_ilm_uint id, t_ilm_bool created, void *user_data)
{
    if (object == ILM_SURFACE)
        g_ilmControlStatus = created ? CREATE_SURFACE : DESTROY_SURFACE;
    else if (object == ILM_LAYER)
        g_ilmControlStatus = created ? CREATE_LAYER : DESTROY_LAYER;
}

static void surfaceCallbackFunction(t_ilm_surface surface, struct ilmSurfaceProperties* surfaceProperties, t_ilm_notification_mask mask)
{
    g_ilm_notification_mask = mask;
}

static void layerCallbackFunction(t_ilm_layer layer, struct ilmLayerProperties* layerProperties, t_ilm_notification_mask mask)
{
    g_ilm_notification_mask = mask;
}

class IlmControlTest : public ::testing::Test
{
public:
    void SetUp()
    {
        init_ctx_list_content();
        CLIENT_API_FAKE_LIST(RESET_FAKE);
        g_ilmControlStatus = NONE;
        g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    }

    void TearDown()
    {
        deinit_ctx_list_content();
    }

    void init_ctx_list_content()
    {
        memset(&ilm_context, 0, sizeof(ilm_context));
        ilm_context.wl.controller = (struct ivi_wm*)&m_iviWmControllerFakePointer;
        ilm_context.wl.error_flag = ILM_SUCCESS;

        custom_wl_list_init(&ilm_context.wl.list_seat);
        custom_wl_list_init(&ilm_context.wl.list_surface);
        custom_wl_list_init(&ilm_context.wl.list_screen);
        custom_wl_list_init(&ilm_context.wl.list_layer);

        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            /*prepare the list_seat*/
            mp_ctxSeat[i] = (struct seat_context*)calloc(1, sizeof(struct seat_context));
            mp_ctxSeat[i]->seat_name = strdup(mp_ilmSeatNames[i]);
            mp_ctxSeat[i]->capabilities = ILM_INPUT_DEVICE_ALL;
            custom_wl_list_insert(&ilm_context.wl.list_seat, &mp_ctxSeat[i]->link);

            /*prepare the list_surface, all surface accept to "default" seat */
            mp_ctxSurface[i] = (struct surface_context*)calloc(1, sizeof(struct surface_context));
            mp_ctxSurface[i]->id_surface = mp_ilmSurfaceIds[i];
            mp_ctxSurface[i]->ctx = &ilm_context.wl;
            mp_ctxSurface[i]->prop = mp_surfaceProps[i];
            mp_ctxSurface[i]->notification = NULL;
            custom_wl_list_init(&mp_ctxSurface[i]->list_accepted_seats);
            mp_accepted_seat[i] = (struct accepted_seat*)calloc(1, sizeof(struct accepted_seat));
            mp_accepted_seat[i]->seat_name = strdup("default");
            custom_wl_list_insert(&mp_ctxSurface[i]->list_accepted_seats, &mp_accepted_seat[i]->link);
            custom_wl_list_insert(&ilm_context.wl.list_surface, &mp_ctxSurface[i]->link);

            /*prepare the list_layer, they don't contain any surface */
            mp_ctxLayer[i] = (struct layer_context*)calloc(1, sizeof(struct layer_context));
            mp_ctxLayer[i]->id_layer = mp_ilmLayerIds[i];
            mp_ctxLayer[i]->ctx = &ilm_context.wl;
            mp_ctxLayer[i]->prop = mp_layerProps[i];
            mp_ctxLayer[i]->notification = NULL;
            custom_wl_list_insert(&ilm_context.wl.list_layer, &mp_ctxLayer[i]->link);
            custom_wl_array_init(&mp_ctxLayer[i]->render_order);

            /*prepare the list_screen, they don't contain any layer */
            mp_ctxScreen[i] = (struct screen_context*)calloc(1, sizeof(struct screen_context));
            mp_ctxScreen[i]->id_screen = mp_ilmScreenIds[i];
            mp_ctxScreen[i]->name = i;
            mp_ctxScreen[i]->ctx = &ilm_context.wl;
            mp_ctxScreen[i]->prop = mp_screenProps[i];
            custom_wl_list_insert(&ilm_context.wl.list_screen, &mp_ctxScreen[i]->link);
            custom_wl_array_init(&mp_ctxScreen[i]->render_order);
        }
    }

    void deinit_ctx_list_content()
    {
        {
            struct surface_context *l, *n;
            wl_list_for_each_safe(l, n, &ilm_context.wl.list_surface, link)
                custom_wl_list_remove(&l->link);
        }
        {
            struct layer_context *l, *n;
            wl_list_for_each_safe(l, n, &ilm_context.wl.list_layer, link)
                custom_wl_list_remove(&l->link);
        }
        {
            struct screen_context *l, *n;
            wl_list_for_each_safe(l, n, &ilm_context.wl.list_screen, link)
                custom_wl_list_remove(&l->link);
        }
        {
            struct seat_context *l, *n;
            wl_list_for_each_safe(l, n, &ilm_context.wl.list_seat, link)
                custom_wl_list_remove(&l->link);
        }

        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            if(mp_ctxSurface[i] != nullptr)
                free(mp_ctxSurface[i]);
            if(mp_ctxLayer[i] != nullptr)
                free(mp_ctxLayer[i]);
            if(mp_ctxScreen[i] != nullptr)
                free(mp_ctxScreen[i]);
            if(mp_accepted_seat[i] != nullptr) {
                free(mp_accepted_seat[i]->seat_name);
                free(mp_accepted_seat[i]);
            }
            if(mp_ctxSeat[i] != nullptr) {
                free(mp_ctxSeat[i]->seat_name);
                free(mp_ctxSeat[i]);
            }
        }

        ilm_context.wl.controller = nullptr;
        ilm_context.wl.notification = nullptr;
        ilm_context.initialized = false;
    }

    t_ilm_surface mp_ilmSurfaceIds[MAX_NUMBER] = {1, 2, 3, 4, 5};
    t_ilm_surface mp_ilmScreenIds[MAX_NUMBER] = {10, 20, 30, 40, 50};
    t_ilm_surface mp_ilmLayerIds[MAX_NUMBER] = {100, 200, 300, 400, 500};

    char *mp_ilmSeatNames[MAX_NUMBER] = {(char*)"default", (char*)"seat1",
            (char*)"seat2", (char*)"seat3", (char*)"seat4"};

    struct ilmSurfaceProperties mp_surfaceProps[MAX_NUMBER] = {
        {0.6, 0, 0, 500, 500, 500, 500, 0, 0, 500, 500, ILM_TRUE, 10, 100, ILM_INPUT_DEVICE_ALL},
        {0.7, 10, 50, 600, 400, 600, 400, 50, 40, 200, 1000, ILM_FALSE, 30, 300, ILM_INPUT_DEVICE_POINTER|ILM_INPUT_DEVICE_KEYBOARD},
        {0.8, 20, 60, 700, 300, 700, 300, 60, 30, 300, 900, ILM_FALSE, 60, 1230, ILM_INPUT_DEVICE_KEYBOARD},
        {0.9, 30, 70, 800, 200, 800, 200, 70, 20, 400, 800, ILM_TRUE, 90, 4561, ILM_INPUT_DEVICE_KEYBOARD|ILM_INPUT_DEVICE_TOUCH},
        {1.0, 40, 80, 900, 100, 900, 100, 80, 10, 600, 700, ILM_TRUE, 100, 5646, ILM_INPUT_DEVICE_TOUCH},
    };

    struct ilmLayerProperties mp_layerProps[MAX_NUMBER] = {
        {0.1, 0, 0, 1280, 720, 0, 0, 1920, 1080, ILM_TRUE},
        {0.2, 10, 80, 1380, 520, 80, 10, 2920, 9080, ILM_FALSE},
        {0.3, 20, 70, 1480, 420, 70, 20, 3920, 8080, ILM_FALSE},
        {0.4, 30, 60, 1580, 320, 60, 30, 4920, 7080, ILM_FALSE},
        {0.5, 40, 50, 1680, 220, 50, 40, 5920, 6080, ILM_TRUE},
    };

    struct ilmScreenProperties mp_screenProps[MAX_NUMBER] = {
        {0, nullptr, 1920, 1080, "screen_1"},
        {0, nullptr, 3000, 10000, "screen_2"},
        {0, nullptr, 4000, 9000, "screen_3"},
        {0, nullptr, 5000, 8000, "screen_4"},
        {0, nullptr, 6000, 7000, "screen_5"},
    };

    struct surface_context *mp_ctxSurface[MAX_NUMBER] = {nullptr};
    struct accepted_seat *mp_accepted_seat[MAX_NUMBER] = {nullptr};
    struct layer_context *mp_ctxLayer[MAX_NUMBER] = {nullptr};
    struct screen_context *mp_ctxScreen[MAX_NUMBER] = {nullptr};
    struct seat_context *mp_ctxSeat[MAX_NUMBER] = {nullptr};

    uint8_t m_iviWmControllerFakePointer = 0;
    int mp_successResult[1] = {0};
    int mp_failureResult[1] = {-1};
    ilmErrorTypes mp_ilmErrorType[1];
    void *mp_fakePointer = (void*)0xFFFFFFFF;

    static ilmErrorTypes ScreenshotDoneCallbackFunc(void *user_data, t_ilm_int fd, t_ilm_uint width, t_ilm_uint height, t_ilm_uint stride, t_ilm_uint format, t_ilm_uint timestamp)
    {
        screenshot_data_t *screenshotData = static_cast<screenshot_data_t*>(user_data);
        screenshotData->fd.store(fd);
        return screenshotData->result;
    }

    static void ScreenshotErrorCallbackFunc(void *user_data, t_ilm_uint error, const char *message)
    {
        screenshot_data_t *screenshotData = static_cast<screenshot_data_t*>(user_data);
        screenshotData->error.store(error);
    }

};

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfSurface_invalidInput
 * @brief               Test case of ilm_getPropertiesOfSurface() where input pSurfaceProperties is null object
 * @test_procedure Steps:
 *                      -# Calling the ilm_getPropertiesOfSurface() with input pSurfaceProperties is null object
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfSurface() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfSurface_invalidInput)
{
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfSurface(MAX_NUMBER + 1, nullptr));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfSurface_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getPropertiesOfSurface() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getPropertiesOfSurface()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfSurface() must return ILM_FAILED
 *                         +# A sequence with 3 external functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfSurface_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    struct ilmSurfaceProperties *l_surfaceProp = (struct ilmSurfaceProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfSurface(1, l_surfaceProp));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfSurface_cannotGetSurface
 * @brief               Test case of ilm_getPropertiesOfSurface() where wl_display_roundtrip_queue() success, return 0
 *                      but invalid surface id {MAX_NUMBER + 1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_getPropertiesOfSurface()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfSurface() must return ILM_FAILED
 *                         +# A sequence with 3 external functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfSurface_cannotGetSurface)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    struct ilmSurfaceProperties *l_surfaceProp = (struct ilmSurfaceProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfSurface(MAX_NUMBER + 1, l_surfaceProp));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfSurface_success
 * @brief               Test case of ilm_getPropertiesOfSurface() where wl_display_roundtrip_queue() success, return 0
 *                      and valid surface id {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_getPropertiesOfSurface()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfSurface() must return ILM_SUCCESS
 *                         +# Surface properties output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfSurface_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    struct ilmSurfaceProperties *l_surfaceProp = (struct ilmSurfaceProperties *)calloc(1, sizeof(struct ilmSurfaceProperties));
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfSurface(1, l_surfaceProp));

    EXPECT_EQ(l_surfaceProp->opacity, mp_surfaceProps[0].opacity);
    EXPECT_EQ(l_surfaceProp->sourceX, mp_surfaceProps[0].sourceX);
    EXPECT_EQ(l_surfaceProp->sourceY, mp_surfaceProps[0].sourceY);
    EXPECT_EQ(l_surfaceProp->sourceWidth, mp_surfaceProps[0].sourceWidth);
    EXPECT_EQ(l_surfaceProp->sourceHeight, mp_surfaceProps[0].sourceHeight);
    EXPECT_EQ(l_surfaceProp->origSourceWidth, mp_surfaceProps[0].origSourceWidth);
    EXPECT_EQ(l_surfaceProp->origSourceHeight, mp_surfaceProps[0].origSourceHeight);
    EXPECT_EQ(l_surfaceProp->destX, mp_surfaceProps[0].destX);
    EXPECT_EQ(l_surfaceProp->destY, mp_surfaceProps[0].destY);
    EXPECT_EQ(l_surfaceProp->destWidth, mp_surfaceProps[0].destWidth);
    EXPECT_EQ(l_surfaceProp->destHeight, mp_surfaceProps[0].destHeight);
    EXPECT_EQ(l_surfaceProp->visibility, mp_surfaceProps[0].visibility);
    EXPECT_EQ(l_surfaceProp->frameCounter, mp_surfaceProps[0].frameCounter);
    EXPECT_EQ(l_surfaceProp->creatorPid, mp_surfaceProps[0].creatorPid);
    EXPECT_EQ(l_surfaceProp->focus, mp_surfaceProps[0].focus);

    free(l_surfaceProp);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfLayer_invalidInput
 * @brief               Test case of ilm_getPropertiesOfLayer() where input pLayerProperties is null object
 * @test_procedure Steps:
 *                      -# Calling the ilm_getPropertiesOfLayer() with input pLayerProperties is null object
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfLayer() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() not be called
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfLayer_invalidInput)
{
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfLayer((MAX_NUMBER + 1) *100, nullptr));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfLayer_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getPropertiesOfLayer() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getPropertiesOfLayer()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfLayer() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfLayer_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    struct ilmLayerProperties *l_layerProp = (struct ilmLayerProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfLayer(100, l_layerProp));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfLayer_cannotGetLayer
 * @brief               Test case of ilm_getPropertiesOfLayer() where wl_display_roundtrip_queue() success, return 0
 *                      but invalid layer id {(MAX_NUMBER + 1) * 100}
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_getPropertiesOfLayer()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfLayer() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfLayer_cannotGetLayer)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    struct ilmLayerProperties *l_layerProp = (struct ilmLayerProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfLayer((MAX_NUMBER + 1) *100, l_layerProp));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfLayer_success
 * @brief               Test case of ilm_getPropertiesOfLayer() where wl_display_roundtrip_queue() success, return 0
 *                      and valid surface id {100}
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_getPropertiesOfLayer()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfLayer() must return ILM_SUCCESS
 *                         +# Surface properties output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfLayer_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    struct ilmLayerProperties *l_layerProp = (struct ilmLayerProperties *)calloc(1, sizeof(struct ilmLayerProperties));
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfLayer(100, l_layerProp));

    EXPECT_EQ(l_layerProp->opacity, mp_layerProps[0].opacity);
    EXPECT_EQ(l_layerProp->sourceX, mp_layerProps[0].sourceX);
    EXPECT_EQ(l_layerProp->sourceY, mp_layerProps[0].sourceY);
    EXPECT_EQ(l_layerProp->sourceWidth, mp_layerProps[0].sourceWidth);
    EXPECT_EQ(l_layerProp->sourceHeight, mp_layerProps[0].sourceHeight);
    EXPECT_EQ(l_layerProp->destX, mp_layerProps[0].destX);
    EXPECT_EQ(l_layerProp->destY, mp_layerProps[0].destY);
    EXPECT_EQ(l_layerProp->destWidth, mp_layerProps[0].destWidth);
    EXPECT_EQ(l_layerProp->destHeight, mp_layerProps[0].destHeight);
    EXPECT_EQ(l_layerProp->visibility, mp_layerProps[0].visibility);

    free(l_layerProp);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfScreen_invalidInput
 * @brief               Test case of ilm_getPropertiesOfScreen() where input param wrong
 * @test_procedure Steps:
 *                      -# Calling the ilm_getPropertiesOfScreen() time 1 with input pScreenProperties is null object
 *                      -# Calling the ilm_getPropertiesOfScreen() time 2 with invalid input screen id {(MAX_NUMBER + 1) *10}
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfScreen() time 1 must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# ilm_getPropertiesOfScreen() time 2 must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfScreen_invalidInput)
{
    struct ilmScreenProperties *l_ScreenProp = (struct ilmScreenProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_getPropertiesOfScreen(MAX_NUMBER, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfScreen((MAX_NUMBER + 1) *10, l_ScreenProp));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfScreen_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getPropertiesOfScreen() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getPropertiesOfScreen()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfScreen() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SCREEN_GET
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfScreen_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    struct ilmScreenProperties *l_ScreenProp = (struct ilmScreenProperties *)mp_fakePointer;
    ASSERT_EQ(ILM_FAILED, ilm_getPropertiesOfScreen(10, l_ScreenProp));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SCREEN_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getPropertiesOfScreen_success
 * @brief               Test case of ilm_getPropertiesOfScreen() where wl_display_roundtrip_queue() success, return 0
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Prepare the data output of ilm_getPropertiesOfScreen
 *                      -# Calling the ilm_getPropertiesOfScreen()
 *                      -# Verification point:
 *                         +# ilm_getPropertiesOfScreen() must return ILM_SUCCESS
 *                         +# Properties screen output should same with preapre data
 */
TEST_F(IlmControlTest, ilm_getPropertiesOfScreen_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    uint32_t *l_addLayer = (uint32_t*)custom_wl_array_add(&mp_ctxScreen[0]->render_order, sizeof(uint32_t));
    *l_addLayer = 100;
    l_addLayer = (uint32_t*)custom_wl_array_add(&mp_ctxScreen[0]->render_order, sizeof(uint32_t));
    *l_addLayer = 200;

    struct ilmScreenProperties *l_ScreenProp = (struct ilmScreenProperties *)calloc(1, sizeof(struct ilmScreenProperties));
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfScreen(10, l_ScreenProp));

    EXPECT_EQ(l_ScreenProp->layerCount, 2);
    EXPECT_EQ(l_ScreenProp->layerIds[0], 100);
    EXPECT_EQ(l_ScreenProp->layerIds[1], 200);
    EXPECT_EQ(l_ScreenProp->screenWidth, mp_ctxScreen[0]->prop.screenWidth);
    EXPECT_EQ(l_ScreenProp->screenHeight, mp_ctxScreen[0]->prop.screenHeight);
    EXPECT_EQ(0, strcmp(l_ScreenProp->connectorName, mp_ctxScreen[0]->prop.connectorName));

    free(l_ScreenProp->layerIds);
    free(l_ScreenProp);
    custom_wl_array_release(&mp_ctxScreen[0]->render_order);
}

/** ================================================================================================
 * @test_id             ilm_getScreenIDs_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getScreenIDs() where ilm context initialized is false, not ready init
 *                      and wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilm_getScreenIDs() time 1
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getScreenIDs() time 2
 *                      -# Verification point:
 *                         +# Both of ilm_getScreenIDs() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_getScreenIDs_cannotSyncAcquireInstance)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    t_ilm_uint l_numberIds = 0;
    t_ilm_uint *lp_listIds = nullptr;

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilm_getScreenIDs(&l_numberIds, &lp_listIds));

    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_getScreenIDs(&l_numberIds, &lp_listIds));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getScreenIDs_invaildInput
 * @brief               Test case of ilm_getScreenIDs() where input param wrong
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getScreenIDs() time 1 with pNumberOfIDs is null pointer
 *                      -# Calling the ilm_getScreenIDs() time 2 with ppIDs is null pointer
 *                      -# Verification point:
 *                         +# Both of ilm_getScreenIDs() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_getScreenIDs_invaildInput)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_uint l_numberIds = 0;
    t_ilm_uint *lp_listIds = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getScreenIDs(nullptr, &lp_listIds));
    ASSERT_EQ(ILM_FAILED, ilm_getScreenIDs(&l_numberIds, nullptr));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getScreenIDs_success
 * @brief               Test case of ilm_getScreenIDs() where wl_display_roundtrip_queue() success, return 0
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getScreenIDs()
 *                      -# Verification point:
 *                         +# ilm_getScreenIDs() must return ILM_SUCCESS
 *                         +# The result output should same with the prepare input
 */
TEST_F(IlmControlTest, ilm_getScreenIDs_success)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_uint l_numberIds = 0;
    t_ilm_uint *lp_listIds = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&l_numberIds, &lp_listIds));

    EXPECT_EQ(MAX_NUMBER, l_numberIds);
    for(uint8_t i = 0; i< l_numberIds; i++) {
        EXPECT_EQ(lp_listIds[i], mp_ilmScreenIds[i]);
    }

    free(lp_listIds);
}

/** ================================================================================================
 * @test_id             ilm_getScreenResolution_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getScreenResolution() where ilm context initialized is false, not ready init
 *                      and wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilm_getScreenResolution() time 1
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getScreenResolution() time 2
 *                      -# Verification point:
 *                         +# Both of ilm_getScreenResolution() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_getScreenResolution_cannotSyncAcquireInstance)
{
    t_ilm_uint l_screenWidth = 0, l_screenHeight = 0;

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilm_getScreenResolution(10, &l_screenWidth, &l_screenHeight));

    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_getScreenResolution(10, &l_screenWidth, &l_screenHeight));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getScreenResolution_invaildInput
 * @brief               Test case of ilm_getScreenResolution() where input param wrong
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getScreenResolution() time 1 with pWidth is null pointer
 *                      -# Calling the ilm_getScreenResolution() time 2 with pHeight is null pointer
 *                      -# Calling the ilm_getScreenResolution() time 3 with invalid screen id
 *                      -# Verification point:
 *                         +# ilm_getScreenResolution() time 1, time 2 and time 3 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_getScreenResolution_invaildInput)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_uint l_screenWidth = 0, l_screenHeight = 0;
    ASSERT_EQ(ILM_FAILED, ilm_getScreenResolution(10, nullptr, &l_screenHeight));
    ASSERT_EQ(ILM_FAILED, ilm_getScreenResolution(10, &l_screenWidth, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getScreenResolution(1, &l_screenWidth, &l_screenHeight));
}

/** ================================================================================================
 * @test_id             ilm_getScreenResolution_success
 * @brief               Test case of ilm_getScreenResolution() where wl_display_roundtrip_queue() success, return 0
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getScreenResolution()
 *                      -# Verification point:
 *                         +# ilm_getScreenResolution() must return ILM_SUCCESS
 *                         +# The result output should same with the prepare input
 */
TEST_F(IlmControlTest, ilm_getScreenResolution_success)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_uint l_screenWidth = 0, l_screenHeight = 0;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenResolution(10, &l_screenWidth, &l_screenHeight));

    EXPECT_EQ(1920, l_screenWidth);
    EXPECT_EQ(1080, l_screenHeight);
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDs_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getLayerIDs() where ilm context initialized is false, not ready init
 *                      and wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilm_getLayerIDs() time 1
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getLayerIDs() time 2
 *                      -# Verification point:
 *                         +# Both of ilm_getLayerIDs() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_getLayerIDs_cannotSyncAcquireInstance)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDs(&l_numberLayers, &lp_listLayers));

    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDs(&l_numberLayers, &lp_listLayers));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDs_invaildInput
 * @brief               Test case of ilm_getLayerIDs() where input param wrong
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getLayerIDs() time 1 with pLength is null pointer
 *                      -# Calling the ilm_getLayerIDs() time 2 with ppArray is null pointer
 *                      -# Verification point:
 *                         +# ilm_getLayerIDs() time 1 and time 2 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_getLayerIDs_invaildInput)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDs(nullptr, &lp_listLayers));
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDs(&l_numberLayers, nullptr));
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDs_success
 * @brief               Test case of ilm_getLayerIDs() where wl_display_roundtrip_queue() success, return 0
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getLayerIDs()
 *                      -# Verification point:
 *                         +# ilm_getLayerIDs() must return ILM_SUCCESS
 *                         +# The result output should same with the prepare input
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IlmControlTest, ilm_getLayerIDs_success)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&l_numberLayers, &lp_listLayers));

    EXPECT_EQ(MAX_NUMBER, l_numberLayers);
    for(uint8_t i = 0; i< l_numberLayers; i++) {
        EXPECT_EQ(lp_listLayers[i], mp_ilmLayerIds[i]);
    }

    free(lp_listLayers);
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDsOnScreen_invalidInput
 * @brief               Test case of ilm_getLayerIDsOnScreen() where input param wrong
 * @test_procedure Steps:
 *                      -# Calling the ilm_getLayerIDsOnScreen() time 1 with ppArray is null pointer
 *                      -# Calling the ilm_getLayerIDsOnScreen() time 2 with pLength is null pointer
 *                      -# Calling the ilm_getLayerIDsOnScreen() time 3 with invalid screen id
 *                      -# Verification point:
 *                         +# ilm_getLayerIDsOnScreen() time 1, time 2 and time 3 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_getLayerIDsOnScreen_invalidInput)
{
    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDsOnScreen(10, &l_numberLayers, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDsOnScreen(10, nullptr, &lp_listLayers));
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDsOnScreen(1, &l_numberLayers, &lp_listLayers));
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDsOnScreen_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getLayerIDsOnScreen() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getLayerIDsOnScreen()
 *                      -# Verification point:
 *                         +# ilm_getLayerIDsOnScreen() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SCREEN_GET
 */
TEST_F(IlmControlTest, ilm_getLayerIDsOnScreen_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getLayerIDsOnScreen(10, &l_numberLayers, &lp_listLayers));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SCREEN_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getLayerIDsOnScreen_success
 * @brief               Test case of ilm_getLayerIDsOnScreen() where wl_display_roundtrip_queue() success, return 0
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Prepare the data output of ilm_getLayerIDsOnScreen
 *                      -# Calling the ilm_getLayerIDsOnScreen()
 *                      -# Verification point:
 *                         +# ilm_getLayerIDsOnScreen() must return ILM_SUCCESS
 *                         +# The result output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_getLayerIDsOnScreen_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    uint32_t *l_addLayer = (uint32_t*)custom_wl_array_add(&mp_ctxScreen[0]->render_order, sizeof(uint32_t));
    *l_addLayer = 100;
    l_addLayer = (uint32_t*)custom_wl_array_add(&mp_ctxScreen[0]->render_order, sizeof(uint32_t));
    *l_addLayer = 200;

    t_ilm_int l_numberLayers = 0;
    t_ilm_layer *lp_listLayers = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDsOnScreen(10, &l_numberLayers, &lp_listLayers));

    EXPECT_EQ(l_numberLayers, 2);
    EXPECT_EQ(lp_listLayers[0], 100);
    EXPECT_EQ(lp_listLayers[1], 200);

    free(lp_listLayers);
    custom_wl_array_release(&mp_ctxScreen[0]->render_order);
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDs_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getSurfaceIDs() where ilm context initialized is false, not ready init
 *                      and wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilm_getSurfaceIDs() time 1
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getSurfaceIDs() time 2
 *                      -# Verification point:
 *                         +# Both of ilm_getSurfaceIDs() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDs_cannotSyncAcquireInstance)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    t_ilm_int l_numberSurfaces = 0;
    t_ilm_surface *lp_listSurfaces = nullptr;

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDs(&l_numberSurfaces, &lp_listSurfaces));

    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDs(&l_numberSurfaces, &lp_listSurfaces));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDs_invaildInput
 * @brief               Test case of ilm_getSurfaceIDs() where input param wrong
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getSurfaceIDs() time 1 with pLength is null pointer
 *                      -# Calling the ilm_getSurfaceIDs() time 2 with ppArray is null pointer
 *                      -# Verification point:
 *                         +# ilm_getLayerIDsOnScreen() time 1 and time 2 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDs_invaildInput)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_int l_numberSurfaces = 0;
    t_ilm_surface *lp_listSurfaces = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDs(nullptr, &lp_listSurfaces));
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDs(&l_numberSurfaces, nullptr));
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDs_success
 * @brief               Test case of ilm_getSurfaceIDs() where wl_display_roundtrip_queue() success, return 0
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_getSurfaceIDs()
 *                      -# Verification point:
 *                         +# ilm_getSurfaceIDs() must return ILM_SUCCESS
 *                         +# The result output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDs_success)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_int l_numberSurfaces = 0;
    t_ilm_surface *lp_listSurfaces = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&l_numberSurfaces, &lp_listSurfaces));

    EXPECT_EQ(MAX_NUMBER, l_numberSurfaces);
    for(uint8_t i = 0; i< l_numberSurfaces; i++) {
        EXPECT_EQ(lp_listSurfaces[i], mp_ilmSurfaceIds[i]);
    }

    free(lp_listSurfaces);
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDsOnLayer_invalidInput
 * @brief               Test case of ilm_getSurfaceIDsOnLayer() where input param wrong
 * @test_procedure Steps:
 *                      -# Calling the ilm_getSurfaceIDsOnLayer() time 1 with ppArray is null pointer
 *                      -# Calling the ilm_getSurfaceIDsOnLayer() time 2 with pLength is null pointer
 *                      -# Calling the ilm_getSurfaceIDsOnLayer() time 3 with invalid layer id
 *                      -# Verification point:
 *                         +# ilm_getSurfaceIDsOnLayer() time 1, time 2 and time 3 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDsOnLayer_invalidInput)
{
    t_ilm_int l_numberSurfaces = 0;
    t_ilm_layer *lp_listSurfaces = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDsOnLayer(100, &l_numberSurfaces, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDsOnLayer(100, nullptr, &lp_listSurfaces));
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDsOnLayer(1, &l_numberSurfaces, &lp_listSurfaces));
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDsOnLayer_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getSurfaceIDsOnLayer() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getSurfaceIDsOnLayer()
 *                      -# Verification point:
 *                         +# ilm_getSurfaceIDsOnLayer() must return ILM_FAILED
 *                         +# A sequence with 5 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDsOnLayer_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_int l_numberSurfaces = 0;
    t_ilm_layer *lp_listSurfaces = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_getSurfaceIDsOnLayer(100, &l_numberSurfaces, &lp_listSurfaces));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], (void*)wl_array_release);
    ASSERT_EQ(fff.call_history[4], (void*)wl_array_init);
    ASSERT_EQ(fff.call_history[5], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getSurfaceIDsOnLayer_success
 * @brief               Test case of ilm_getSurfaceIDsOnLayer() where wl_display_roundtrip_queue() success, return 0
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Prepare the data output
 *                      -# Calling the ilm_getSurfaceIDsOnLayer()
 *                      -# Verification point:
 *                         +# ilm_getSurfaceIDsOnLayer() must return ILM_SUCCESS
 *                         +# The result output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_getSurfaceIDsOnLayer_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    uint32_t *l_addSurface = (uint32_t*)custom_wl_array_add(&mp_ctxLayer[0]->render_order, sizeof(uint32_t));
    *l_addSurface = 1;
    l_addSurface = (uint32_t*)custom_wl_array_add(&mp_ctxLayer[0]->render_order, sizeof(uint32_t));
    *l_addSurface = 2;

    t_ilm_int l_numberSurfaces = 0;
    t_ilm_layer *lp_listSurfaces = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(100, &l_numberSurfaces, &lp_listSurfaces));

    EXPECT_EQ(l_numberSurfaces, 2);
    EXPECT_EQ(lp_listSurfaces[0], 1);
    EXPECT_EQ(lp_listSurfaces[1], 2);

    free(lp_listSurfaces);
    custom_wl_array_release(&mp_ctxLayer[0]->render_order);
}

/** ================================================================================================
 * @test_id             ilm_layerCreateWithDimension_cannotSyncAcquireInstance
 * @brief               Test case of ilm_layerCreateWithDimension() where ilm context initialized is false, not ready init
 *                      and wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilm_layerCreateWithDimension() time 1
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_layerCreateWithDimension() time 2
 *                      -# Verification point:
 *                         +# Both of ilm_layerCreateWithDimension() time 1 and time 2 must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_layerCreateWithDimension_cannotSyncAcquireInstance)
{
    t_ilm_layer l_layerId = 600;

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilm_layerCreateWithDimension(&l_layerId, 640, 480));

    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_layerCreateWithDimension(&l_layerId, 640, 480));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerCreateWithDimension_invaildInput
 * @brief               Test case of ilm_layerCreateWithDimension() where input param wrong
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_layerCreateWithDimension() time 1 with pLayerId is null pointer
 *                      -# Calling the ilm_layerCreateWithDimension() time 2 with valid layer id
 *                      -# Verification point:
 *                         +# ilm_layerCreateWithDimension() time 1 and time 2 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_layerCreateWithDimension_invaildInput)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_FAILED, ilm_layerCreateWithDimension(nullptr, 640, 480));
    ASSERT_EQ(ILM_FAILED, ilm_layerCreateWithDimension(&l_layerId, 640, 480));
}

/** ================================================================================================
 * @test_id             ilm_layerCreateWithDimension_success
 * @brief               Test case of ilm_layerCreateWithDimension() where wl_display_roundtrip_queue() success, return 0
 *                      and invalid input param
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_layerCreateWithDimension()
 *                      -# Verification point:
 *                         +# ilm_layerCreateWithDimension() must return ILM_SUCCESS
 *                         +# The result output should same with prepare data
 */
TEST_F(IlmControlTest, ilm_layerCreateWithDimension_success)
{
    wl_list_length_fake.custom_fake = custom_wl_list_length;
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = INVALID_ID;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&l_layerId, 640, 480));

    ASSERT_EQ(0, l_layerId);
}

/** ================================================================================================
 * @test_id             ilm_layerRemove_wrongCtx
 * @brief               Test case of ilm_layerRemove() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerRemove()
 *                      -# Verification point:
 *                         +# ilm_layerRemove() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerRemove_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_FAILED, ilm_layerRemove(l_layerId));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerRemove_removeOne
 * @brief               Test case of ilm_layerRemove() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerRemove()
 *                      -# Verification point:
 *                         +# ilm_layerRemove() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_DESTROY_LAYOUT_LAYER
 */
TEST_F(IlmControlTest, ilm_layerRemove_removeOne)
{
    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemove(l_layerId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_DESTROY_LAYOUT_LAYER, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerAddSurface_wrongCtx
 * @brief               Test case of ilm_layerAddSurface() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerAddSurface()
 *                      -# Verification point:
 *                         +# ilm_layerAddSurface() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerAddSurface_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerAddSurface(l_layerId, l_surfaceId));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerAddSurface_addOne
 * @brief               Test case of ilm_layerAddSurface() where ilm context is initilized and valid layer id, surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerAddSurface()
 *                      -# Verification point:
 *                         +# ilm_layerAddSurface() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_ADD_SURFACE
 */
TEST_F(IlmControlTest, ilm_layerAddSurface_addOne)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddSurface(l_layerId, l_surfaceId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_ADD_SURFACE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveSurface_wrongCtx
 * @brief               Test case of ilm_layerRemoveSurface() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerRemoveSurface()
 *                      -# Verification point:
 *                         +# ilm_layerRemoveSurface() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerRemoveSurface_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerRemoveSurface(l_layerId, l_surfaceId));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveSurface_removeOne
 * @brief               Test case of ilm_layerRemoveSurface() where ilm context is initilized and valid layer id, surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerRemoveSurface()
 *                      -# Verification point:
 *                         +# ilm_layerRemoveSurface() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_REMOVE_SURFACE
 */
TEST_F(IlmControlTest, ilm_layerRemoveSurface_removeOne)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemoveSurface(l_layerId, l_surfaceId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_REMOVE_SURFACE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerSetVisibility_wrongCtx
 * @brief               Test case of ilm_layerSetVisibility() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerSetVisibility()
 *                      -# Verification point:
 *                         +# ilm_layerSetVisibility() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerSetVisibility_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    t_ilm_bool l_newVisibility = ILM_FALSE;
    ASSERT_EQ(ILM_FAILED, ilm_layerSetVisibility(l_layerId, l_newVisibility));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerSetVisibility_success
 * @brief               Test case of ilm_layerSetVisibility() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerSetVisibility()
 *                      -# Verification point:
 *                         +# ilm_layerSetVisibility() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_LAYER_VISIBILITY
 */
TEST_F(IlmControlTest, ilm_layerSetVisibility_success)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_bool l_newVisibility = ILM_TRUE;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(l_layerId, l_newVisibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_LAYER_VISIBILITY, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetVisibility_wrongVisibility
 * @brief               Test case of ilm_layerGetVisibility() where input pVisibility is null pointer
 * @test_procedure Steps:
 *                      -# Set pVisibility is null pointer
 *                      -# Calling the ilm_layerGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_layerGetVisibility() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerGetVisibility_wrongVisibility)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_bool* p_Visibility = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetVisibility(l_layerId, p_Visibility));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerGetVisibility_cannotGetRoundTripQueue
 * @brief               Test case of ilm_layerGetVisibility() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_layerGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_layerGetVisibility() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_layerGetVisibility_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_layer l_layerId = 100;
    t_ilm_bool l_visibility = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetVisibility(l_layerId, &l_visibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetVisibility_invalidLayerId
 * @brief               Test case of ilm_layerGetVisibility() where invalid layer id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_layerGetVisibility() with invalid layer id
 *                      -# Verification point:
 *                         +# ilm_layerGetVisibility() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_layerGetVisibility_invalidLayerId)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = 600;
    t_ilm_bool l_visibility = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetVisibility(l_layerId, &l_visibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetVisibility_success
 * @brief               Test case of ilm_layerGetVisibility() where wl_display_roundtrip_queue() success, return 0
 *                      and valid layer id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_layerGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_layerGetVisibility() must return ILM_SUCCESS
 *                         +# The result data must same with prepration
 */
TEST_F(IlmControlTest, ilm_layerGetVisibility_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = 100;
    t_ilm_bool l_visibility = 0;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetVisibility(l_layerId, &l_visibility));

    ASSERT_EQ(mp_layerProps[0].visibility, l_visibility);
}

/** ================================================================================================
 * @test_id             ilm_layerSetOpacity_wrongCtx
 * @brief               Test case of ilm_layerSetOpacity() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerSetOpacity()
 *                      -# Verification point:
 *                         +# ilm_layerSetOpacity() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerSetOpacity_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    t_ilm_float l_opacity = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerSetOpacity(l_layerId, l_opacity));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerSetOpacity_success
 * @brief               Test case of ilm_layerSetOpacity() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerSetOpacity()
 *                      -# Verification point:
 *                         +# ilm_layerSetOpacity() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_LAYER_OPACITY
 */
TEST_F(IlmControlTest, ilm_layerSetOpacity_success)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_float l_opacity = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOpacity(l_layerId, l_opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_LAYER_OPACITY, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetOpacity_invalidArg
 * @brief               Test case of ilm_layerGetOpacity() where input pOpacity is null pointer
 * @test_procedure Steps:
 *                      -# Set pOpacity is null pointer
 *                      -# Calling the ilm_layerGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_layerGetOpacity() must return ILM_FAILED
 *                         +# No external functions is called
 */
TEST_F(IlmControlTest, ilm_layerGetOpacity_invalidArg)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_float *p_opacity = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetOpacity(l_layerId, p_opacity));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerGetOpacity_cannotGetRoundTripQueue
 * @brief               Test case of ilm_layerGetOpacity() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_layerGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_layerGetOpacity() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_layerGetOpacity_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_layer l_layerId = 100;
    t_ilm_float l_opacity = 0.5;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetOpacity(l_layerId, &l_opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetOpacity_invalidLayerId
 * @brief               Test case of ilm_layerGetOpacity() where invalid layer id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_layerGetOpacity() with invalid layer id
 *                      -# Verification point:
 *                         +# ilm_layerGetOpacity() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_GET
 */
TEST_F(IlmControlTest, ilm_layerGetOpacity_invalidLayerId)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = 600;
    t_ilm_float l_opacity = 0.5;
    ASSERT_EQ(ILM_FAILED, ilm_layerGetOpacity(l_layerId, &l_opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerGetOpacity_success
 * @brief               Test case of ilm_layerGetOpacity() where wl_display_roundtrip_queue() success, return 0
 *                      and valid layer id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_layerGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_layerGetOpacity() must return ILM_SUCCESS
 *                         +# The result data must same with prepration
 */
TEST_F(IlmControlTest, ilm_layerGetOpacity_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_layer l_layerId = 100;
    t_ilm_float l_opacity = 0.0;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOpacity(l_layerId, &l_opacity));

    ASSERT_EQ(mp_layerProps[0].opacity, l_opacity);
}

/** ================================================================================================
 * @test_id             ilm_layerSetSourceRectangle_wrongCtx
 * @brief               Test case of ilm_layerSetSourceRectangle() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerSetSourceRectangle()
 *                      -# Verification point:
 *                         +# ilm_layerSetSourceRectangle() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_layerSetSourceRectangle_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_FAILED, ilm_layerSetSourceRectangle(l_layerId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerSetSourceRectangle_success
 * @brief               Test case of ilm_layerSetSourceRectangle() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerSetSourceRectangle()
 *                      -# Verification point:
 *                         +# ilm_layerSetSourceRectangle() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_LAYER_SOURCE_RECTANGLE
 */
TEST_F(IlmControlTest, ilm_layerSetSourceRectangle_success)
{
    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetSourceRectangle(l_layerId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_LAYER_SOURCE_RECTANGLE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerSetDestinationRectangle_wrongCtx
 * @brief               Test case of ilm_layerSetDestinationRectangle() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerSetDestinationRectangle()
 *                      -# Verification point:
 *                         +# ilm_layerSetDestinationRectangle() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_layerSetDestinationRectangle_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_FAILED, ilm_layerSetDestinationRectangle(l_layerId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerSetDestinationRectangle_success
 * @brief               Test case of ilm_layerSetDestinationRectangle() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerSetDestinationRectangle()
 *                      -# Verification point:
 *                         +# ilm_layerSetDestinationRectangle() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_LAYER_DESTINATION_RECTANGLE
 */
TEST_F(IlmControlTest, ilm_layerSetDestinationRectangle_success)
{
    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetDestinationRectangle(l_layerId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_LAYER_DESTINATION_RECTANGLE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_layerSetRenderOrder_wrongCtx
 * @brief               Test case of ilm_layerSetRenderOrder() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_layerSetRenderOrder()
 *                      -# Verification point:
 *                         +# ilm_layerSetRenderOrder() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_layerSetRenderOrder_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_layer l_layerId = 100;
    t_ilm_int l_number = 1;
    ASSERT_EQ(ILM_FAILED, ilm_layerSetRenderOrder(l_layerId, mp_ilmSurfaceIds, l_number));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerSetRenderOrder_success
 * @brief               Test case of ilm_layerSetRenderOrder() where ilm context is initilized and valid layer id
 * @test_procedure Steps:
 *                      -# Calling the ilm_layerSetRenderOrder()
 *                      -# Verification point:
 *                         +# ilm_layerSetRenderOrder() must return ILM_SUCCESS
 *                         +# wl_proxy_get_version() must be called {1+l_number} times
 *                         +# wl_proxy_marshal_flags() must be called {1+l_number} times
 *                         +# wl_display_flush() must be called once time
 */
TEST_F(IlmControlTest, ilm_layerSetRenderOrder_success)
{
    t_ilm_layer l_layerId = 100;
    t_ilm_int l_number = MAX_NUMBER;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(l_layerId, mp_ilmSurfaceIds, l_number));

    ASSERT_EQ(1 + l_number, wl_proxy_get_version_fake.call_count);
    ASSERT_EQ(1 + l_number, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(1, wl_display_flush_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetVisibility_wrongCtx
 * @brief               Test case of ilm_surfaceSetVisibility() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_surfaceSetVisibility()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetVisibility() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetVisibility_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_surface l_surfaceId = 1;
    t_ilm_bool l_newVisibility = ILM_FALSE;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetVisibility(l_surfaceId, l_newVisibility));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetVisibility_success
 * @brief               Test case of ilm_surfaceSetVisibility() where ilm context is initilized and valid surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetVisibility()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetVisibility() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_SURFACE_VISIBILITY
 */
TEST_F(IlmControlTest, ilm_surfaceSetVisibility_success)
{
    t_ilm_surface l_surfaceId = 1;
    t_ilm_bool l_newVisibility = ILM_TRUE;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(l_surfaceId, l_newVisibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_SURFACE_VISIBILITY, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetVisibility_wrongVisibility
 * @brief               Test case of ilm_surfaceGetVisibility() where input pVisibility is null pointer
 * @test_procedure Steps:
 *                      -# Set pVisibility is null pointer
 *                      -# Calling the ilm_surfaceGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetVisibility() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceGetVisibility_wrongVisibility)
{
    t_ilm_surface l_surfaceId = 1;
    t_ilm_bool* p_visibility = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetVisibility(l_surfaceId, p_visibility));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetVisibility_cannotGetRoundTripQueue
 * @brief               Test case of ilm_surfaceGetVisibility() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_surfaceGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetVisibility() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_surfaceGetVisibility_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_surface l_surfaceId = 1;
    t_ilm_bool l_visibility = ILM_FALSE;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetVisibility(l_surfaceId, &l_visibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetVisibility_invalidSurfaceId
 * @brief               Test case of ilm_surfaceGetVisibility() where invalid surface id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_surfaceGetVisibility() with invalid surface id
 *                      -# Verification point:
 *                         +# ilm_surfaceGetVisibility() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_surfaceGetVisibility_invalidSurfaceId)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_surface l_surfaceId = 6;
    t_ilm_bool l_visibility = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetVisibility(l_surfaceId, &l_visibility));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetVisibility_success
 * @brief               Test case of ilm_surfaceGetVisibility() where wl_display_roundtrip_queue() success, return 0
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_surfaceGetVisibility()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetVisibility() must return ILM_SUCCESS
 *                         +# The result should same with prepare data
 */
TEST_F(IlmControlTest, ilm_surfaceGetVisibility_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_surface l_surfaceId = 1;
    t_ilm_bool l_visibility = ILM_FALSE;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetVisibility(l_surfaceId, &l_visibility));

    ASSERT_EQ(l_visibility, mp_surfaceProps[0].visibility);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetOpacity_wrongCtx
 * @brief               Test case of ilm_surfaceSetOpacity() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_surfaceSetOpacity()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetOpacity() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetOpacity_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_surface l_surfaceId = 1;
    t_ilm_float opacity = 0.5;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetOpacity(l_surfaceId, opacity));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetOpacity_success
 * @brief               Test case of ilm_surfaceSetOpacity() where ilm context is initilized and valid surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetOpacity()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetOpacity() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_SURFACE_OPACITY
 */
TEST_F(IlmControlTest, ilm_surfaceSetOpacity_success)
{
    t_ilm_surface l_surfaceId = 1;
    t_ilm_float opacity = 0.5;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOpacity(l_surfaceId, opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_SURFACE_OPACITY, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetOpacity_invalidInput
 * @brief               Test case of ilm_surfaceGetOpacity() where input pOpacity is null pointer
 * @test_procedure Steps:
 *                      -# Set pOpacity is null pointer
 *                      -# Calling the ilm_surfaceGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetOpacity() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceGetOpacity_invalidInput)
{
    t_ilm_surface l_surfaceId = 1;
    t_ilm_float *p_opacity = nullptr;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetOpacity(l_surfaceId, p_opacity));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetOpacity_cannotGetRoundTripQueue
 * @brief               Test case of ilm_surfaceGetOpacity() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_surfaceGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetOpacity() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_surfaceGetOpacity_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_surface l_surfaceId = 1;
    t_ilm_float l_opacity = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetOpacity(l_surfaceId, &l_opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetOpacity_invalidSurfaceId
 * @brief               Test case of ilm_surfaceGetOpacity() where invalid surface id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_surfaceGetOpacity() with invalid layer id
 *                      -# Verification point:
 *                         +# ilm_surfaceGetOpacity() must return ILM_FAILED
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_GET
 */
TEST_F(IlmControlTest, ilm_surfaceGetOpacity_invalidSurfaceId)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_surface l_surfaceId = 6;
    t_ilm_float l_opacity = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceGetOpacity(l_surfaceId, &l_opacity));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_GET, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceGetOpacity_success
 * @brief               Test case of ilm_surfaceGetOpacity() where wl_display_roundtrip_queue() success, return 0
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilm_surfaceGetOpacity()
 *                      -# Verification point:
 *                         +# ilm_surfaceGetOpacity() must return ILM_SUCCESS
 *                         +# The result must same with data preparation
 */
TEST_F(IlmControlTest, ilm_surfaceGetOpacity_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    t_ilm_surface l_surfaceId = 1;
    t_ilm_float l_opacity = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOpacity(l_surfaceId, &l_opacity));

    ASSERT_EQ(l_opacity, mp_surfaceProps[0].opacity);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetSourceRectangle_wrongCtx
 * @brief               Test case of ilm_surfaceSetSourceRectangle() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_surfaceSetSourceRectangle()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetSourceRectangle() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetSourceRectangle_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetSourceRectangle(l_surfaceId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetSourceRectangle_success
 * @brief               Test case of ilm_surfaceSetSourceRectangle() where ilm context is initilized
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetSourceRectangle()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetSourceRectangle() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_SURFACE_SOURCE_RECTANGLE
 */
TEST_F(IlmControlTest, ilm_surfaceSetSourceRectangle_success)
{
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetSourceRectangle(l_surfaceId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_SURFACE_SOURCE_RECTANGLE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetDestinationRectangle_wrongCtx
 * @brief               Test case of ilm_surfaceSetDestinationRectangle() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_surfaceSetDestinationRectangle()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetDestinationRectangle() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetDestinationRectangle_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetDestinationRectangle(l_surfaceId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetDestinationRectangle_success
 * @brief               Test case of ilm_surfaceSetDestinationRectangle() where ilm context is initilized
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetDestinationRectangle()
 *                      -# Verification point:
 *                         +# ilm_surfaceSetDestinationRectangle() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SET_SURFACE_DESTINATION_RECTANGLE
 */
TEST_F(IlmControlTest, ilm_surfaceSetDestinationRectangle_success)
{
    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetDestinationRectangle(l_surfaceId, 0, 0, 640, 480));

    ASSERT_EQ(fff.call_history[0], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[2], (void*)wl_display_flush);
    ASSERT_EQ(fff.call_history[3], nullptr);
    ASSERT_EQ(IVI_WM_SET_SURFACE_DESTINATION_RECTANGLE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetType_wrongType
 * @brief               Test case of ilm_surfaceSetType() where input ilmSurfaceType is wrong
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetType() with invalid input ilmSurfaceType
 *                      -# Verification point:
 *                         +# ilm_surfaceSetType() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetType_wrongType)
{
    t_ilm_surface l_surfaceId = 1;
    ilmSurfaceType l_type = (ilmSurfaceType)3;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_surfaceSetType(l_surfaceId, l_type));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetType_wrongCtx
 * @brief               Test case of ilm_surfaceSetType() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_surfaceSetType() time 1 with input ilmSurfaceType is ILM_SURFACETYPE_RESTRICTED
 *                      -# Calling the ilm_surfaceSetType() time 2 with input ilmSurfaceType is ILM_SURFACETYPE_DESKTOP
 *                      -# Verification point:
 *                         +# Both of ilm_surfaceSetType() time 1 and time 2 must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_surfaceSetType_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_surface l_surfaceId = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetType(l_surfaceId, ILM_SURFACETYPE_RESTRICTED));
    ASSERT_EQ(ILM_FAILED, ilm_surfaceSetType(l_surfaceId, ILM_SURFACETYPE_DESKTOP));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceSetType_success
 * @brief               Test case of ilm_surfaceSetType() where ilm context is initilized
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Calling the ilm_surfaceSetType() time 1 with input ilmSurfaceType is ILM_SURFACETYPE_RESTRICTED
 *                      -# Calling the ilm_surfaceSetType() time 2 with input ilmSurfaceType is ILM_SURFACETYPE_DESKTOP
 *                      -# Verification point:
 *                         +# Both of ilm_surfaceSetType() time 1 and time 2 must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags() must be called 2 times
 *                         +# wl_display_flush() must be called 2 times
 *                         +# Second arg input of wl_proxy_marshal_flags should is IVI_WM_SET_SURFACE_TYPE
 */
TEST_F(IlmControlTest, ilm_surfaceSetType_success)
{
    t_ilm_surface l_surfaceId = 1;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetType(l_surfaceId, ILM_SURFACETYPE_RESTRICTED));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetType(l_surfaceId, ILM_SURFACETYPE_DESKTOP));

    ASSERT_EQ(2, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(2, wl_display_flush_fake.call_count);
    ASSERT_EQ(IVI_WM_SET_SURFACE_TYPE, wl_proxy_marshal_flags_fake.arg1_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_displaySetRenderOrder_wrongCtx
 * @brief               Test case of ilm_displaySetRenderOrder() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_displaySetRenderOrder()
 *                      -# Verification point:
 *                         +# ilm_displaySetRenderOrder() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_displaySetRenderOrder_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    t_ilm_display l_displayId = 10;
    t_ilm_int l_number = 1;
    ASSERT_EQ(ILM_FAILED, ilm_displaySetRenderOrder(l_displayId, mp_ilmLayerIds, l_number));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_displaySetRenderOrder_invalidDisplayId
 * @brief               Test case of ilm_displaySetRenderOrder() where input display invalid
 * @test_procedure Steps:
 *                      -# Calling the ilm_displaySetRenderOrder() with input display invalid
 *                      -# Verification point:
 *                         +# ilm_displaySetRenderOrder() must return ILM_FAILED
 *                         +# No external function is called
 */
TEST_F(IlmControlTest, ilm_displaySetRenderOrder_invalidDisplayId)
{
    t_ilm_display l_displayId = 11;
    t_ilm_int l_number = 1;
    ASSERT_EQ(ILM_FAILED, ilm_displaySetRenderOrder(l_displayId, mp_ilmLayerIds, l_number));

    ASSERT_EQ(fff.call_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_displaySetRenderOrder_success
 * @brief               Test case of ilm_displaySetRenderOrder() where ilm context is initilized
 *                      and valid display id
 * @test_procedure Steps:
 *                      -# Calling the ilm_displaySetRenderOrder()
 *                      -# Verification point:
 *                         +# ilm_displaySetRenderOrder() must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags() must be called {1+l_number} time
 *                         +# wl_display_flush() must be called once time
 */
TEST_F(IlmControlTest, ilm_displaySetRenderOrder_success)
{
    t_ilm_layer l_displayId = 10;
    t_ilm_int l_number = MAX_NUMBER;
    ASSERT_EQ(ILM_SUCCESS, ilm_displaySetRenderOrder(l_displayId, mp_ilmLayerIds, l_number));

    ASSERT_EQ(1 + l_number, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(1, wl_display_flush_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_layerAddNotification_cannotGetRoundTripQueue
 * @brief               Test case of ilm_layerAddNotification() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_layerAddNotification()
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_layerAddNotification_cannotGetRoundTripQueue)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_layer l_layerId = 10;
    ASSERT_EQ(ILM_FAILED, ilm_layerAddNotification(l_layerId, nullptr));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerAddNotification_invalidLayerId
 * @brief               Test case of ilm_layerAddNotification() where input layer invalid
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_layerAddNotification() with input layer invalid
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# wl_proxy_marshal_flags() not be called
 *                         +# wl_display_roundtrip_queue() must be called once time
 */
TEST_F(IlmControlTest, ilm_layerAddNotification_invalidLayerId)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 10;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_layerAddNotification(l_layerId, nullptr));

    ASSERT_EQ(0, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_layerAddNotification_success
 * @brief               Test case of ilm_layerAddNotification() where ilm context is initilized
 *                      and valid layer id
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_layerAddNotification()
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_SUCCESS
 *                         +# A sequence with 4 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_SYNC
 *                         +# Notification callback of layer 0 should be change
 */
TEST_F(IlmControlTest, ilm_layerAddNotification_success)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 100;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(l_layerId, mp_fakePointer));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[2], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[3], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[4], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_SYNC, wl_proxy_marshal_flags_fake.arg1_history[0]);
    ASSERT_EQ(mp_ctxLayer[0]->notification, mp_fakePointer);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveNotification_invalidLayer
 * @brief               Test case of ilm_layerRemoveNotification() where input layer invalid
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_layerRemoveNotification() with input layer invalid
 *                      -# Verification point:
 *                         +# ilm_layerRemoveNotification() must return ILM_FAILED
 *                         +# Only wl_display_roundtrip_queue() is called
 */
TEST_F(IlmControlTest, ilm_layerRemoveNotification_invalidLayer)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 10;
    ASSERT_EQ(ILM_FAILED, ilm_layerRemoveNotification(l_layerId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveNotification_cannotGetRoundTripQueue
 * @brief               Test case of ilm_layerRemoveNotification() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_layerRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_layerRemoveNotification() must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_layerRemoveNotification_cannotGetRoundTripQueue)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 100;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    ASSERT_EQ(ILM_FAILED, ilm_layerRemoveNotification(l_layerId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveNotification_noNotification
 * @brief               Test case of ilm_layerRemoveNotification() where input callback is null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set notification of layer 0 to nullptr
 *                      -# Calling the ilm_layerRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_layerRemoveNotification() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# Only wl_display_roundtrip_queue() is called
 */
TEST_F(IlmControlTest, ilm_layerRemoveNotification_noNotification)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 100;
    mp_ctxLayer[0]->notification = nullptr;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_layerRemoveNotification(l_layerId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_layerRemoveNotification_success
 * @brief               Test case of ilm_layerRemoveNotification() where and ilm context is initilized
 *                      input callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set notification of layer 0 is valid pointer
 *                      -# Calling the ilm_layerRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_layerRemoveNotification() must return ILM_SUCCESS
 *                         +# A sequence with 4 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_LAYER_SYNC
 *                         +# Notification of layer 0 should be nullptr
 */
TEST_F(IlmControlTest, ilm_layerRemoveNotification_success)
{
    ilm_context.initialized = true;

    t_ilm_layer l_layerId = 100;
    mp_ctxLayer[0]->notification = mp_fakePointer;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemoveNotification(l_layerId));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[2], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[3], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[4], nullptr);
    ASSERT_EQ(IVI_WM_LAYER_SYNC, wl_proxy_marshal_flags_fake.arg1_history[0]);
    ASSERT_EQ(mp_ctxLayer[0]->notification, nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceAddNotification_addNewSurface
 * @brief               Test case of ilm_surfaceAddNotification() where ilm context is initilized
 *                      and valid callback, invalid surface
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# A sequence with 3 extenal functions is called
 *                         +# Id of new surface must be same with preparation
 */
TEST_F(IlmControlTest, ilm_surfaceAddNotification_addNewSurface)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 6;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface, &surfaceCallbackFunction));

    EXPECT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    EXPECT_EQ(fff.call_history[1], (void*)wl_list_insert);
    EXPECT_EQ(fff.call_history[2], (void*)wl_list_init);
    EXPECT_EQ(fff.call_history[3], nullptr);

    struct surface_context *lp_createSurface = 
            (struct surface_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct surface_context, link));
    ASSERT_EQ(lp_createSurface->id_surface, l_surface);
    free(lp_createSurface);
}

/** ================================================================================================
 * @test_id             ilm_surfaceAddNotification_noAddNewSurface
 * @brief               Test case of ilm_surfaceAddNotification() where ilm context is initilized
 *                      and invalid callback, invalid surface
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# Only wl_display_roundtrip_queue() is called
 */
TEST_F(IlmControlTest, ilm_surfaceAddNotification_noAddNewSurface)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 6;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_surfaceAddNotification(l_surface, nullptr));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceAddNotification_cannotGetRoundTripQueue
 * @brief               Test case of ilm_surfaceAddNotification() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_surfaceAddNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_surfaceAddNotification_cannotGetRoundTripQueue)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_surface l_surface = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceAddNotification(l_surface, nullptr));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceAddNotification_nullNotification
 * @brief               Test case of ilm_surfaceAddNotification() where input callback is null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# Only wl_display_roundtrip_queue() is called
 *                         +# Notification of surface 0 should be nullptr
 */
TEST_F(IlmControlTest, ilm_surfaceAddNotification_nullNotification)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 1;
    mp_ctxSurface[0]->notification = mp_fakePointer;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface, nullptr));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
    ASSERT_EQ(mp_ctxSurface[0]->notification, nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceAddNotification_success
 * @brief               Test case of ilm_surfaceAddNotification() where input callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# A sequence with 4 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_SYNC
 *                         +# Notification of layer 0 should be same with preparation
 */
TEST_F(IlmControlTest, ilm_surfaceAddNotification_success)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 1;
    mp_ctxSurface[0]->notification = nullptr;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface, &surfaceCallbackFunction));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[2], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[3], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[4], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_SYNC, wl_proxy_marshal_flags_fake.arg1_history[0]);
    ASSERT_EQ(mp_ctxSurface[0]->notification, &surfaceCallbackFunction);
}

/** ================================================================================================
 * @test_id             ilm_surfaceRemoveNotification_cannotGetRoundTripQueue
 * @brief               Test case of ilm_surfaceRemoveNotification() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_surfaceRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceRemoveNotification() must return ILM_FAILED
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_surfaceRemoveNotification_cannotGetRoundTripQueue)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    t_ilm_surface l_surface = 1;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceRemoveNotification(l_surface));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_display_get_error);
    ASSERT_EQ(fff.call_history[2], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceRemoveNotification_nullNotification
 * @brief               Test case of ilm_surfaceRemoveNotification() where callback is null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceRemoveNotification() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# Only wl_display_roundtrip_queue is called
 */
TEST_F(IlmControlTest, ilm_surfaceRemoveNotification_nullNotification)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 1;
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilm_surfaceRemoveNotification(l_surface));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceRemoveNotification_invalidSurface
 * @brief               Test case of ilm_surfaceRemoveNotification() where invalid input surface id
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceRemoveNotification() with invalid surface id
 *                      -# Verification point:
 *                         +# ilm_surfaceRemoveNotification() must return ILM_FAILED
 *                         +# Only wl_display_roundtrip_queue is called
 */
TEST_F(IlmControlTest, ilm_surfaceRemoveNotification_invalidSurface)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 6;
    ASSERT_EQ(ILM_FAILED, ilm_surfaceRemoveNotification(l_surface));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_surfaceRemoveNotification_success
 * @brief               Test case of ilm_surfaceRemoveNotification() where callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set notification of surface 0 is valid pointer
 *                      -# Calling the ilm_surfaceRemoveNotification()
 *                      -# Verification point:
 *                         +# ilm_surfaceRemoveNotification() must return ILM_SUCCESS
 *                         +# A sequence with 4 extenal functions is called
 *                         +# Second arg input of wl_proxy_marshal_flags should be IVI_WM_SURFACE_SYNC
 *                         +# Notification of surface 0 should be nullptr
 */
TEST_F(IlmControlTest, ilm_surfaceRemoveNotification_success)
{
    ilm_context.initialized = true;

    t_ilm_surface l_surface = 1;
    mp_ctxSurface[0]->notification = mp_fakePointer;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceRemoveNotification(l_surface));

    ASSERT_EQ(fff.call_history[0], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void*)wl_proxy_get_version);
    ASSERT_EQ(fff.call_history[2], (void*)wl_proxy_marshal_flags);
    ASSERT_EQ(fff.call_history[3], (void*)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[4], nullptr);
    ASSERT_EQ(IVI_WM_SURFACE_SYNC, wl_proxy_marshal_flags_fake.arg1_history[0]);
    ASSERT_EQ(mp_ctxSurface[0]->notification, nullptr);
}

/** ================================================================================================
 * @test_id             ilm_registerNotification_cannotGetRoundTripQueue
 * @brief               Test case of ilm_registerNotification() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_registerNotification()
 *                      -# Verification point:
 *                         +# ilm_registerNotification() must return ILM_FAILED
 *                         +# wl_display_roundtrip_queue() and wl_display_get_error() must be called once time
 *                         +# A sequence with 2 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_registerNotification_cannotGetRoundTripQueue)
{
    ilm_context.initialized = true;
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    ASSERT_EQ(ILM_FAILED, ilm_registerNotification(&notificationCallback, nullptr));

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(1, wl_display_get_error_fake.call_count);
    ASSERT_EQ(fff.call_history[0], (void *)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void *)wl_display_get_error);
}

/** ================================================================================================
 * @test_id             ilm_registerNotification_success
 * @brief               Test case of ilm_registerNotification() where callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_registerNotification()
 *                      -# Verification point:
 *                         +# ilm_registerNotification() must return ILM_SUCCESS
 *                         +# wl_display_roundtrip_queue() must be called once time
 *                         +# wl_display_get_error() should not be called
 *                         +# A sequence with 1 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_registerNotification_success)
{
    ilm_context.initialized = true;

    ASSERT_EQ(ILM_SUCCESS, ilm_registerNotification(&notificationCallback, nullptr));

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(0, wl_display_get_error_fake.call_count);
    ASSERT_EQ(fff.call_history[0], (void *)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void *)nullptr);
}

/** ================================================================================================
 * @test_id             ilm_unregisterNotification_success
 * @brief               Test case of ilm_unregisterNotification() where ilm context is initilized
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_unregisterNotification()
 *                      -# Verification point:
 *                         +# ilm_unregisterNotification() must return ILM_SUCCESS
 *                         +# wl_display_roundtrip_queue() must be called once time
 *                         +# wl_display_get_error() should not be called
 *                         +# A sequence with 1 extenal functions is called
 */
TEST_F(IlmControlTest, ilm_unregisterNotification_success)
{
    ilm_context.initialized = true;
    ASSERT_EQ(ILM_SUCCESS, ilm_unregisterNotification());
    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(0, wl_display_get_error_fake.call_count);
    ASSERT_EQ(fff.call_history[0], (void *)wl_display_roundtrip_queue);
    ASSERT_EQ(fff.call_history[1], (void *)nullptr);
}

/** ================================================================================================
 * @test_id             ilm_commitChanges_wrongCtx
 * @brief               Test case of ilm_commitChanges() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_commitChanges()
 *                      -# Verification point:
 *                         +# ilm_commitChanges() must return ILM_FAILED
 *                         +# wl_display_roundtrip_queue() not be called
 */
TEST_F(IlmControlTest, ilm_commitChanges_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    ASSERT_EQ(ILM_FAILED, ilm_commitChanges());

    ASSERT_EQ(0, wl_display_roundtrip_queue_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_commitChanges_cannotGetRoundTripQueue
 * @brief               Test case of ilm_commitChanges() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_commitChanges()
 *                      -# Verification point:
 *                         +# ilm_commitChanges() must return ILM_FAILED
 *                         +# wl_display_roundtrip_queue() must be called once time and return -1
 *                         +# wl_display_get_error() must be called once time
 *                         +# wl_proxy_marshal_flags() must be called once time with opcode IVI_WM_COMMIT_CHANGES
 */
TEST_F(IlmControlTest, ilm_commitChanges_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    ASSERT_EQ(ILM_FAILED, ilm_commitChanges());

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(mp_failureResult[0], wl_display_roundtrip_queue_fake.return_val_history[0]);
    ASSERT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_COMMIT_CHANGES);
}

/** ================================================================================================
 * @test_id             ilm_commitChanges_success
 * @brief               Test case of ilm_commitChanges() where wl_display_roundtrip_queue() success, return 0
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_commitChanges()
 *                      -# Verification point:
 *                         +# ilm_commitChanges() must return ILM_SUCCESS
 *                         +# wl_display_roundtrip_queue() must be called once time and return 0
 *                         +# wl_proxy_marshal_flags() must be called once time with opcode IVI_WM_COMMIT_CHANGES
 */
TEST_F(IlmControlTest, ilm_commitChanges_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(mp_successResult[0], wl_display_roundtrip_queue_fake.return_val_history[0]);
    ASSERT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_COMMIT_CHANGES);
}

/** ================================================================================================
 * @test_id             ilm_getError_wrongCtx
 * @brief               Test case of ilm_getError() where ilm context wl.controller is null object
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null object
 *                      -# Calling the ilm_getError()
 *                      -# Verification point:
 *                         +# ilm_getError() must return ILM_FAILED
 *                         +# wl_display_roundtrip_queue() not be called
 */
TEST_F(IlmControlTest, ilm_getError_wrongCtx)
{
    ilm_context.wl.controller = nullptr;

    ASSERT_EQ(ILM_FAILED, ilm_getError());

    ASSERT_EQ(0, wl_display_roundtrip_queue_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_getError_cannotGetRoundTripQueue
 * @brief               Test case of ilm_getError() where wl_display_roundtrip_queue() fails, return -1
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1
 *                      -# Calling the ilm_getError()
 *                      -# Verification point:
 *                         +# ilm_getError() must return ILM_FAILED
 *                         +# wl_display_roundtrip_queue() must be called once time and return -1
 */
TEST_F(IlmControlTest, ilm_getError_cannotGetRoundTripQueue)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    ASSERT_EQ(ILM_FAILED, ilm_getError());

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(mp_failureResult[0], wl_display_roundtrip_queue_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getError_success
 * @brief               Test case of ilm_getError() where wl_display_roundtrip_queue() success, return 0
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0
 *                      -# Calling the ilm_getError()
 *                      -# Verification point:
 *                         +# ilm_getError() must return ILM_SUCCESS
 *                         +# wl_display_roundtrip_queue() must be called once time and return 0
 */
TEST_F(IlmControlTest, ilm_getError_success)
{
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_getError());

    ASSERT_EQ(1, wl_display_roundtrip_queue_fake.call_count);
    ASSERT_EQ(mp_successResult[0], wl_display_roundtrip_queue_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_created_sameLayerId
 * @brief               Test case of wm_listener_layer_created() where valid layer id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_created() with layer id exist
 *                      -# Verification point:
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, wm_listener_layer_created_sameLayerId)
{
    wm_listener_layer_created(&ilm_context.wl, nullptr, 100);

    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_created_addNewOne
 * @brief               Test case of wm_listener_layer_created() where invalid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_created() time 1 with invalid layer id and null notication callback
 *                      -# Set notication callback
 *                      -# Calling the wm_listener_layer_created() time 2 with invalid layer id and a notication callback
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called
 *                         +# when invoke callback function, g_ilmControlStatus must be seted to CREATE_LAYER
 *                      -# Free resources are allocated when running the test time 1 and time 2
 */
TEST_F(IlmControlTest, wm_listener_layer_created_addNewOne)
{
    wm_listener_layer_created(&ilm_context.wl, nullptr, 600);

    EXPECT_EQ(1, wl_list_insert_fake.call_count);

    struct layer_context *lp_createLayer = (struct layer_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct layer_context, link));
    free(lp_createLayer);

    ilm_context.wl.notification = notificationCallback;
    wm_listener_layer_created(&ilm_context.wl, nullptr, 600);

    EXPECT_EQ(2, wl_list_insert_fake.call_count);
    EXPECT_EQ(CREATE_LAYER, g_ilmControlStatus);

    lp_createLayer = (struct layer_context*)(uintptr_t(wl_list_insert_fake.arg1_history[1]) - offsetof(struct layer_context, link));
    free(lp_createLayer);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_destroyed_wrongLayerId
 * @brief               Test case of wm_listener_layer_destroyed() where invalid layer id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_destroyed() with invalid layer id
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(IlmControlTest, wm_listener_layer_destroyed_wrongLayerId)
{
    wm_listener_layer_destroyed(&ilm_context.wl, nullptr, 1);
    ASSERT_EQ(0, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_destroyed_removeOne
 * @brief               Test case of wm_listener_layer_destroyed() where valid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the wm_listener_layer_destroyed() time 1 with valid layer id and null callback
 *                      -# Calling the wm_listener_layer_destroyed() time 2 with valid layer id and a callback
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called
 *                         +# When invoke callback function, g_ilmControlStatus must be seted to DESTROY_LAYER
 *                      -# set ctx_layer to nullptr
 */
TEST_F(IlmControlTest, wm_listener_layer_destroyed_removeOne)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    wm_listener_layer_destroyed(&ilm_context.wl, nullptr, 100);

    ASSERT_EQ(1, wl_list_remove_fake.call_count);
    mp_ctxLayer[0] = nullptr;

    ilm_context.wl.notification = notificationCallback;
    wm_listener_layer_destroyed(&ilm_context.wl, nullptr, 200);

    ASSERT_EQ(2, wl_list_remove_fake.call_count);
    ASSERT_EQ(DESTROY_LAYER, g_ilmControlStatus);
    mp_ctxLayer[1] = nullptr;
}

/** ================================================================================================
 * @test_id             wm_listener_layer_surface_added_wrongLayerId
 * @brief               Test case of wm_listener_layer_surface_added() where invalid layer id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_surface_added() with invalid layer id
 *                      -# Verification point:
 *                         +# wl_array_add() not be called
 */
TEST_F(IlmControlTest, wm_listener_layer_surface_added_wrongLayerId)
{
    wm_listener_layer_surface_added(&ilm_context.wl, nullptr, 1, 1);
    ASSERT_EQ(0, wl_array_add_fake.call_count);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_surface_added_addNewOne
 * @brief               Test case of wm_listener_layer_surface_added() where valid layer id
 * @test_procedure Steps:
 *                      -# Mocking the wl_array_add() does return a add_id
 *                      -# Calling the wm_listener_layer_surface_added() with valid layer id
 *                      -# Verification point:
 *                         +# wl_array_add() must be called once time
 *                         +# l_dataSurface must be seted same input surface id
 */
TEST_F(IlmControlTest, wm_listener_layer_surface_added_addNewOne)
{
    uint32_t l_dataSurface = 0;
    void *lp_elemData = &l_dataSurface;
    SET_RETURN_SEQ(wl_array_add, &lp_elemData, 1);

    wm_listener_layer_surface_added(&ilm_context.wl, nullptr, 100, 1);

    ASSERT_EQ(1, wl_array_add_fake.call_count);
    ASSERT_EQ(1, l_dataSurface);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_created_sameSurfaceId
 * @brief               Test case of wm_listener_surface_created() where valid surface id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_created() with valid surface id
 *                      -# Verification point:
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, wm_listener_surface_created_sameSurfaceId)
{
    wm_listener_surface_created(&ilm_context.wl, nullptr, 1);
    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_created_addNewOne
 * @brief               Test case of wm_listener_surface_created() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_created() time 1 with valid surface id and null callback
 *                      -# Calling the wm_listener_surface_created() time 2 with valid surface id and a callback
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called
 *                         +# When invoke callback function, g_ilmControlStatus must be seted to CREATE_SURFACE
 *                      -# Free resources are allocated when running the test time 1 and time 2
 */
TEST_F(IlmControlTest, wm_listener_surface_created_addNewOne)
{
    wm_listener_surface_created(&ilm_context.wl, nullptr, MAX_NUMBER + 1);

    EXPECT_EQ(1, wl_list_insert_fake.call_count);

    struct surface_context *lp_createSurface = (struct surface_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct surface_context, link));
    free(lp_createSurface);

    ilm_context.wl.notification = notificationCallback;
    wm_listener_surface_created(&ilm_context.wl, nullptr, MAX_NUMBER + 1);

    EXPECT_EQ(2, wl_list_insert_fake.call_count);
    EXPECT_EQ(CREATE_SURFACE, g_ilmControlStatus);

    lp_createSurface = (struct surface_context*)(uintptr_t(wl_list_insert_fake.arg1_history[1]) - offsetof(struct surface_context, link));
    free(lp_createSurface);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destroyed_wrongSurfaceId
 * @brief               Test case of wm_listener_surface_destroyed() where invalid surface id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_destroyed() with invalid surface id
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(IlmControlTest, wm_listener_surface_destroyed_wrongSurfaceId)
{
    wm_listener_surface_destroyed(&ilm_context.wl, nullptr, MAX_NUMBER + 1);
    ASSERT_EQ(0, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destroyed_removeOnewithNullNotification
 * @brief               Test case of wm_listener_surface_destroyed() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the wm_listener_surface_destroyed() with valid surface id and null callback
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 2 times
 *                      -# set ctx_layer to nullptr
 */
TEST_F(IlmControlTest, wm_listener_surface_destroyed_removeOnewithNullNotification)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    wm_listener_surface_destroyed(&ilm_context.wl, nullptr, 1);

    ASSERT_EQ(2, wl_list_remove_fake.call_count);
    mp_ctxSurface[0] = nullptr;
    mp_accepted_seat[0] = nullptr;
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destroyed_removeOnewithCtxNotification
 * @brief               Test case of wm_listener_surface_destroyed() where valid surface id
 *                      and notication callback added from ilm_surfaceAddNotification()
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification() to add callback
 *                      -# Calling the wm_listener_surface_destroyed() with valid surface id
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# wl_list_remove() must be called 2 times
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_CONTENT_REMOVED
 *                      -# set ctx_layer to nullptr
 */
TEST_F(IlmControlTest, wm_listener_surface_destroyed_removeOnewithCtxNotification)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;
    ilm_context.initialized = true;

    EXPECT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(2, &surfaceCallbackFunction));
    wm_listener_surface_destroyed(&ilm_context.wl, nullptr, 2);

    EXPECT_EQ(2, wl_list_remove_fake.call_count);
    EXPECT_EQ(ILM_NOTIFICATION_CONTENT_REMOVED, g_ilm_notification_mask);
    mp_ctxSurface[1] = nullptr;
    mp_accepted_seat[1] = nullptr;
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destroyed_removeOnewithCallbackNotification
 * @brief               Test case of wm_listener_surface_destroyed() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Set the ilm context wl.notification to callback function
 *                      -# Calling the wm_listener_surface_destroyed() with valid surface id
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 2 times
 *                         +# When invoke callback function, g_ilmControlStatus must be seted to DESTROY_SURFACE
 *                      -# set ctx_layer to nullptr
 */
TEST_F(IlmControlTest, wm_listener_surface_destroyed_removeOnewithCallbackNotification)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;
    ilm_context.wl.notification = notificationCallback;

    wm_listener_surface_destroyed(&ilm_context.wl, nullptr, 3);

    EXPECT_EQ(2, wl_list_remove_fake.call_count);
    EXPECT_EQ(DESTROY_SURFACE, g_ilmControlStatus);
    mp_ctxSurface[2] = nullptr;

    mp_accepted_seat[2] = nullptr;
}

/** ================================================================================================
 * @test_id             wm_listener_surface_visibility_invalidSurface
 * @brief               Test case of wm_listener_surface_visibility() where invalid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_visibility()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_visibility_invalidSurface)
{
    uint32_t l_surface_id = 6;
    wm_listener_surface_visibility(&ilm_context.wl, nullptr, l_surface_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_visibility_nullNotification
 * @brief               Test case of wm_listener_surface_visibility() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_visibility()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_visibility_nullNotification)
{
    uint32_t l_surface_id = 5;
    wm_listener_surface_visibility(&ilm_context.wl, nullptr, l_surface_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_visibility_success
 * @brief               Test case of wm_listener_surface_visibility() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'visibility' to default value
 *                      -# Calling the ilm_surfaceAddNotification() to add a callback
 *                      -# Calling the wm_listener_surface_visibility() time 1 with visibility is ILM_TRUE
 *                      -# Calling the wm_listener_surface_visibility() time 2 with visibility is ILM_FALSE
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_VISIBILITY and property visibility of surface has ID 1 should be changed
 */
TEST_F(IlmControlTest, wm_listener_surface_visibility_success)
{
    ilm_context.initialized = true;
    uint32_t l_surface_id = 1;
    // set to default value
    mp_ctxSurface[0]->prop.visibility = ILM_TRUE;
    // add a callback
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface_id, &surfaceCallbackFunction));
    wm_listener_surface_visibility(&ilm_context.wl, nullptr, l_surface_id, ILM_TRUE);
    ASSERT_EQ(ILM_NOTIFICATION_CONTENT_AVAILABLE, g_ilm_notification_mask);
    // set to default value
    mp_ctxSurface[0]->prop.visibility = ILM_TRUE;
    wm_listener_surface_visibility(&ilm_context.wl, nullptr, l_surface_id, ILM_FALSE);
    ASSERT_EQ(ILM_NOTIFICATION_VISIBILITY, g_ilm_notification_mask);
    ASSERT_EQ(mp_ctxSurface[0]->prop.visibility, ILM_FALSE);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_visibility_invalidLayer
 * @brief               Test case of wm_listener_layer_visibility() where invalid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.controller is null pointer
 *                      -# Calling the wm_listener_layer_visibility()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_visibility_invalidLayer)
{
    ilm_context.wl.controller = nullptr;
    uint32_t l_layer_id = 1;
    wm_listener_layer_visibility(&ilm_context.wl, nullptr, l_layer_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_visibility_nullNotification
 * @brief               Test case of wm_listener_layer_visibility() where valid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_visibility()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_visibility_nullNotification)
{
    uint32_t l_layer_id = 500;
    wm_listener_layer_visibility(&ilm_context.wl, nullptr, l_layer_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_visibility_success
 * @brief               Test case of wm_listener_layer_visibility() where valid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'visibility' to default value
 *                      -# Calling the ilm_layerAddNotification() to add a callback
 *                      -# Calling the wm_listener_layer_visibility() time 1 with visibility is ILM_TRUE
 *                      -# Calling the wm_listener_layer_visibility() time 2 with visibility is ILM_FALSE
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_VISIBILITY and property visibility of layer has ID 100 should be changed
 */
TEST_F(IlmControlTest, wm_listener_layer_visibility_success)
{
    ilm_context.initialized = true;
    // set to default value
    mp_ctxLayer[0]->prop.visibility = ILM_TRUE;
    uint32_t l_layer_id = 100;
    // add callback
    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(l_layer_id, &layerCallbackFunction));
    wm_listener_layer_visibility(&ilm_context.wl, nullptr, l_layer_id, ILM_TRUE);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
    // set to default value
    mp_ctxLayer[0]->prop.visibility = ILM_TRUE;
    wm_listener_layer_visibility(&ilm_context.wl, nullptr, l_layer_id, ILM_FALSE);
    ASSERT_EQ(ILM_NOTIFICATION_VISIBILITY, g_ilm_notification_mask);
    ASSERT_EQ(mp_ctxLayer[0]->prop.visibility, ILM_FALSE);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_opacity_invalidSurface
 * @brief               Test case of wm_listener_surface_opacity() where invalid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_opacity()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_opacity_invalidSurface)
{
    uint32_t l_surface_id = 6;
    wm_listener_surface_opacity(&ilm_context.wl, nullptr, l_surface_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_opacity_nullNotification
 * @brief               Test case of wm_listener_surface_opacity() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_opacity()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_opacity_nullNotification)
{
    uint32_t l_surface_id = 5;
    wm_listener_surface_opacity(&ilm_context.wl, nullptr, l_surface_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_opacity_success
 * @brief               Test case of wm_listener_surface_opacity() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'opacity' to default value
 *                      -# Calling the ilm_surfaceAddNotification() to add a callback
 *                      -# Calling the wm_listener_surface_opacity() time 1 with opacity is 0.6
 *                      -# Calling the wm_listener_surface_opacity() time 2 with opacity is 6.0
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_OPACITY, otherwise should be setted to ILM_NOTIFICATION_CONTENT_AVAILABLE
 */
TEST_F(IlmControlTest, wm_listener_surface_opacity_success)
{
    ilm_context.initialized = true;
    // set to default value
    mp_ctxSurface[0]->prop.opacity = (wl_fixed_t)0.6;
    uint32_t l_surface_id = 1;
    //add callback
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface_id, &surfaceCallbackFunction));
    wm_listener_surface_opacity(&ilm_context.wl, nullptr, l_surface_id, (wl_fixed_t)0.6);
    ASSERT_EQ(ILM_NOTIFICATION_CONTENT_AVAILABLE, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.opacity = (wl_fixed_t)0.6;
    wm_listener_surface_opacity(&ilm_context.wl, nullptr, l_surface_id, (wl_fixed_t)6.0);
    ASSERT_EQ(ILM_NOTIFICATION_OPACITY, g_ilm_notification_mask);
    ASSERT_NE(mp_ctxSurface[0]->prop.opacity, (wl_fixed_t)0.6);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_opacity_invalidLayer
 * @brief               Test case of wm_listener_layer_opacity() where invalid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_opacity()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_opacity_invalidLayer)
{
    uint32_t l_layer_id = 1;
    wm_listener_layer_opacity(&ilm_context.wl, nullptr, l_layer_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_opacity_nullNotification
 * @brief               Test case of wm_listener_layer_opacity() where valid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_opacity()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_opacity_nullNotification)
{
    uint32_t l_layer_id = 500;
    wm_listener_layer_opacity(&ilm_context.wl, nullptr, l_layer_id, 0.5);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_opacity_success
 * @brief               Test case of wm_listener_layer_opacity() where valid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'opacity' to default value
 *                      -# Calling the ilm_layerAddNotification() to add a callback
 *                      -# Calling the wm_listener_layer_opacity() time 1 with opacity is 0.1
 *                      -# Calling the wm_listener_layer_opacity() time 2 with opacity is 6.0
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_OPACITY, otherwise should be setted to ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_opacity_success)
{
    ilm_context.initialized = true;
    // set to default value
    mp_ctxLayer[0]->prop.opacity = (wl_fixed_t)0.1;
    uint32_t l_layer_id = 100;
    //add callback
    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(l_layer_id, &layerCallbackFunction));
    wm_listener_layer_opacity(&ilm_context.wl, nullptr, l_layer_id, (wl_fixed_t)0.1);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.opacity = (wl_fixed_t)0.1;
    wm_listener_layer_opacity(&ilm_context.wl, nullptr, l_layer_id, (wl_fixed_t)6.0);
    ASSERT_EQ(ILM_NOTIFICATION_OPACITY, g_ilm_notification_mask);
    ASSERT_NE(mp_ctxLayer[0]->prop.opacity, (wl_fixed_t)0.1);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_source_rectangle_invalidSurface
 * @brief               Test case of wm_listener_surface_source_rectangle() where invalid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_source_rectangle_invalidSurface)
{
    uint32_t l_surface_id = 6;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 450, 450);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_source_rectangle_nullNotification
 * @brief               Test case of wm_listener_surface_source_rectangle() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_source_rectangle_nullNotification)
{
    uint32_t l_surface_id = 5;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 1, 0, 500, 500);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_source_rectangle_success
 * @brief               Test case of wm_listener_surface_source_rectangle() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'sourceX', 'sourceY', 'sourceWidth', 'sourceHeight' to default value
 *                      -# Calling the ilm_surfaceAddNotification() to add a callback
 *                      -# Calling the wm_listener_surface_source_rectangle() with input param x, y, width, height same with property of surface has ID 1
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_SOURCE_RECT
 */
TEST_F(IlmControlTest, wm_listener_surface_source_rectangle_success)
{
    ilm_context.initialized = true;
    uint32_t l_surface_id = 1;
    // set to default value
    mp_ctxSurface[0]->prop.sourceX = 0;
    mp_ctxSurface[0]->prop.sourceY = 0;
    mp_ctxSurface[0]->prop.sourceWidth = 500;
    mp_ctxSurface[0]->prop.sourceHeight = 500;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface_id, &surfaceCallbackFunction));
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 1, 0, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.sourceX = 0;
    mp_ctxSurface[0]->prop.sourceY = 0;
    mp_ctxSurface[0]->prop.sourceWidth = 500;
    mp_ctxSurface[0]->prop.sourceHeight = 500;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 1, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.sourceX = 0;
    mp_ctxSurface[0]->prop.sourceY = 0;
    mp_ctxSurface[0]->prop.sourceWidth = 500;
    mp_ctxSurface[0]->prop.sourceHeight = 500;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 1,   500);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.sourceX = 0;
    mp_ctxSurface[0]->prop.sourceY = 0;
    mp_ctxSurface[0]->prop.sourceWidth = 500;
    mp_ctxSurface[0]->prop.sourceHeight = 500;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 500, 1);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.sourceX = 0;
    mp_ctxSurface[0]->prop.sourceY = 0;
    mp_ctxSurface[0]->prop.sourceWidth = 500;
    mp_ctxSurface[0]->prop.sourceHeight = 500;
    wm_listener_surface_source_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_source_rectangle_invalidLayer
 * @brief               Test case of wm_listener_layer_source_rectangle() where invalid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_source_rectangle_invalidLayer)
{
    uint32_t l_layer_id = 1;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 450, 450);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_source_rectangle_nullNotification
 * @brief               Test case of wm_listener_layer_source_rectangle() where valid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_source_rectangle_nullNotification)
{
    uint32_t l_layer_id = 500;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1280, 0);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_source_rectangle_success
 * @brief               Test case of wm_listener_layer_source_rectangle() where valid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'sourceX', 'sourceY', 'sourceWidth', 'sourceHeight' to default value
 *                      -# Calling the ilm_layerAddNotification() to add a callback
 *                      -# Calling the wm_listener_layer_source_rectangle() with input param x, y, width, height same with property of layer has ID 100
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_SOURCE_RECT
 */
TEST_F(IlmControlTest, wm_listener_layer_source_rectangle_success)
{
    ilm_context.initialized = true;
    uint32_t l_layer_id = 100;
    // set to default value
    mp_ctxLayer[0]->prop.sourceX = 0;
    mp_ctxLayer[0]->prop.sourceY = 0;
    mp_ctxLayer[0]->prop.sourceWidth = 1280;
    mp_ctxLayer[0]->prop.sourceHeight = 720;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(l_layer_id, &layerCallbackFunction));
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 1, 0, 1280, 720);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.sourceX = 0;
    mp_ctxLayer[0]->prop.sourceY = 0;
    mp_ctxLayer[0]->prop.sourceWidth = 1280;
    mp_ctxLayer[0]->prop.sourceHeight = 720;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 1, 1280, 720);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.sourceX = 0;
    mp_ctxLayer[0]->prop.sourceY = 0;
    mp_ctxLayer[0]->prop.sourceWidth = 1280;
    mp_ctxLayer[0]->prop.sourceHeight = 720;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1,    720);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.sourceX = 0;
    mp_ctxLayer[0]->prop.sourceY = 0;
    mp_ctxLayer[0]->prop.sourceWidth = 1280;
    mp_ctxLayer[0]->prop.sourceHeight = 720;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1280, 1);
    ASSERT_EQ(ILM_NOTIFICATION_SOURCE_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.sourceX = 0;
    mp_ctxLayer[0]->prop.sourceY = 0;
    mp_ctxLayer[0]->prop.sourceWidth = 1280;
    mp_ctxLayer[0]->prop.sourceHeight = 720;
    wm_listener_layer_source_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1280, 720);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destination_rectangle_invalidSurface
 * @brief               Test case of wm_listener_surface_destination_rectangle() where invalid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_destination_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_destination_rectangle_invalidSurface)
{
    uint32_t l_surface_id = 6;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 450, 450);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destination_rectangle_nullNotification
 * @brief               Test case of wm_listener_surface_destination_rectangle() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_destination_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_destination_rectangle_nullNotification)
{
    uint32_t l_surface_id = 5;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 500, 0);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_destination_rectangle_success
 * @brief               Test case of wm_listener_surface_destination_rectangle() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'destX', 'destY', 'destWidth', 'destHeight' to default value
 *                      -# Calling the ilm_surfaceAddNotification() to add a callback
 *                      -# Calling the wm_listener_surface_destination_rectangle() with input param x, y, width, height 
 *                          same with property of surface has ID 1
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_DEST_RECT
 */
TEST_F(IlmControlTest, wm_listener_surface_destination_rectangle_success)
{
    ilm_context.initialized = true;
    uint32_t l_surface_id = 1;
    // set to default value
    mp_ctxSurface[0]->prop.destX = 0;
    mp_ctxSurface[0]->prop.destY = 0;
    mp_ctxSurface[0]->prop.destWidth = 500;
    mp_ctxSurface[0]->prop.destHeight = 500;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface_id, &surfaceCallbackFunction));
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 1, 0, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.destX = 0;
    mp_ctxSurface[0]->prop.destY = 0;
    mp_ctxSurface[0]->prop.destWidth = 500;
    mp_ctxSurface[0]->prop.destHeight = 500;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 1, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.destX = 0;
    mp_ctxSurface[0]->prop.destY = 0;
    mp_ctxSurface[0]->prop.destWidth = 500;
    mp_ctxSurface[0]->prop.destHeight = 500;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 1,   500);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.destX = 0;
    mp_ctxSurface[0]->prop.destY = 0;
    mp_ctxSurface[0]->prop.destWidth = 500;
    mp_ctxSurface[0]->prop.destHeight = 500;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 500, 1);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxSurface[0]->prop.destX = 0;
    mp_ctxSurface[0]->prop.destY = 0;
    mp_ctxSurface[0]->prop.destWidth = 500;
    mp_ctxSurface[0]->prop.destHeight = 500;
    wm_listener_surface_destination_rectangle(&ilm_context.wl, nullptr, l_surface_id, 0, 0, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_destination_rectangle_invalidLayer
 * @brief               Test case of wm_listener_layer_destination_rectangle() where invalid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_destination_rectangle_invalidLayer)
{
    uint32_t l_layer_id = 1;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 450, 450);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_destination_rectangle_nullNotification
 * @brief               Test case of wm_listener_layer_destination_rectangle() where valid layer id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_layer_destination_rectangle_nullNotification)
{
    uint32_t l_layer_id = 500;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1280, 0);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_destination_rectangle_success
 * @brief               Test case of wm_listener_layer_destination_rectangle() where valid layer id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Set property 'destX', 'destY', 'destWidth', 'destHeight' to default value
 *                      -# Calling the ilm_layerAddNotification() to add a callback
 *                      -# Calling the wm_listener_layer_destination_rectangle() with input param x, y, width, height same with property 
 *                          of layer has ID 100
 *                      -# Verification point:
 *                         +# ilm_layerAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_DEST_RECT
 */
TEST_F(IlmControlTest, wm_listener_layer_destination_rectangle_success)
{
    ilm_context.initialized = true;
    uint32_t l_layer_id = 100;
    // set to default value
    mp_ctxLayer[0]->prop.destX = 0;
    mp_ctxLayer[0]->prop.destY = 0;
    mp_ctxLayer[0]->prop.destWidth = 1920;
    mp_ctxLayer[0]->prop.destHeight = 1080;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddNotification(l_layer_id, &layerCallbackFunction));
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 1, 0, 1920, 1080);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.destX = 0;
    mp_ctxLayer[0]->prop.destY = 0;
    mp_ctxLayer[0]->prop.destWidth = 1920;
    mp_ctxLayer[0]->prop.destHeight = 1080;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 1, 1920, 1080);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.destX = 0;
    mp_ctxLayer[0]->prop.destY = 0;
    mp_ctxLayer[0]->prop.destWidth = 1920;
    mp_ctxLayer[0]->prop.destHeight = 1080;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1,    1080);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.destX = 0;
    mp_ctxLayer[0]->prop.destY = 0;
    mp_ctxLayer[0]->prop.destWidth = 1920;
    mp_ctxLayer[0]->prop.destHeight = 1080;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1920, 1);
    ASSERT_EQ(ILM_NOTIFICATION_DEST_RECT, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    // set to default value
    mp_ctxLayer[0]->prop.destX = 0;
    mp_ctxLayer[0]->prop.destY = 0;
    mp_ctxLayer[0]->prop.destWidth = 1920;
    mp_ctxLayer[0]->prop.destHeight = 1080;
    wm_listener_layer_destination_rectangle(&ilm_context.wl, nullptr, l_layer_id, 0, 0, 1920, 1080);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_error_success
 * @brief               Test case of wm_listener_surface_error() where ilm context wl.error_flag is ILM_SUCCESS
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.error_flag is ILM_SUCCESS
 *                      -# Calling the wm_listener_surface_error() multiple times with different input param error type
 *                      -# Verification point:
 *                          +# ilm_context.wl.error_flag is setted with correspond error code
 */
TEST_F(IlmControlTest, wm_listener_surface_error_success)
{
    wm_listener_surface_error(&ilm_context.wl, nullptr, 0, IVI_WM_SURFACE_ERROR_NO_SURFACE, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_RESOURCE_NOT_FOUND);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_surface_error(&ilm_context.wl, nullptr, 0, IVI_WM_SURFACE_ERROR_BAD_PARAM, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_INVALID_ARGUMENTS);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_surface_error(&ilm_context.wl, nullptr, 0, IVI_WM_SURFACE_ERROR_NOT_SUPPORTED, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_NOT_IMPLEMENTED);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_surface_error(&ilm_context.wl, nullptr, 0, 3, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_ON_CONNECTION);
}

/** ================================================================================================
 * @test_id             wm_listener_layer_error_success
 * @brief               Test case of wm_listener_layer_error() where ilm context wl.error_flag is ILM_SUCCESS
 * @test_procedure Steps:
 *                      -# Set the ilm context wl.error_flag is ILM_SUCCESS
 *                      -# Calling the wm_listener_layer_error() multiple times with different input param error type
 */
TEST_F(IlmControlTest, wm_listener_layer_error_success)
{
    wm_listener_layer_error(&ilm_context.wl, nullptr, 0, IVI_WM_LAYER_ERROR_NO_SURFACE, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_RESOURCE_NOT_FOUND);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_layer_error(&ilm_context.wl, nullptr, 0, IVI_WM_LAYER_ERROR_NO_LAYER, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_RESOURCE_NOT_FOUND);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_layer_error(&ilm_context.wl, nullptr, 0, IVI_WM_LAYER_ERROR_BAD_PARAM, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_INVALID_ARGUMENTS);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_listener_layer_error(&ilm_context.wl, nullptr, 0, 3, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_ON_CONNECTION);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_size_invalidSurface
 * @brief               Test case of wm_listener_surface_size() where invalid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_size()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_size_invalidSurface)
{
    uint32_t l_surface_id = 6;
    wm_listener_surface_size(&ilm_context.wl, nullptr, l_surface_id, 450, 450);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_size_nullNotification
 * @brief               Test case of wm_listener_surface_size() where valid surface id
 *                      and notication callback is null pointer
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_size()
 *                      -# Verification point:
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to default ILM_NOTIFICATION_ALL
 */
TEST_F(IlmControlTest, wm_listener_surface_size_nullNotification)
{
    uint32_t l_surface_id = 5;
    wm_listener_surface_size(&ilm_context.wl, nullptr, l_surface_id, 0, 500);

    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_size_success
 * @brief               Test case of wm_listener_surface_size() where valid surface id
 *                      and notication callback is not null pointer
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilm_surfaceAddNotification() to add a callback
 *                      -# Calling the wm_listener_surface_size() multiple times with different input param width, height
 *                      -# Verification point:
 *                         +# ilm_surfaceAddNotification() must return ILM_SUCCESS
 *                         +# When invoke callback function, g_ilm_notification_mask must be seted to ILM_NOTIFICATION_CONFIGURED
 */
TEST_F(IlmControlTest, wm_listener_surface_size_success)
{
    ilm_context.initialized = true;

    uint32_t l_surface_id = 1;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceAddNotification(l_surface_id, &surfaceCallbackFunction));
    wm_listener_surface_size(&ilm_context.wl, nullptr, l_surface_id, 0, 500);
    ASSERT_EQ(ILM_NOTIFICATION_CONFIGURED, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    mp_ctxSurface[0]->prop.origSourceWidth = 500;
    mp_ctxSurface[0]->prop.origSourceWidth = 500;
    wm_listener_surface_size(&ilm_context.wl, nullptr, l_surface_id, 500, 0);
    ASSERT_EQ(ILM_NOTIFICATION_CONFIGURED, g_ilm_notification_mask);
    g_ilm_notification_mask = ILM_NOTIFICATION_ALL;
    mp_ctxSurface[0]->prop.origSourceWidth = 500;
    mp_ctxSurface[0]->prop.origSourceHeight = 500;
    wm_listener_surface_size(&ilm_context.wl, nullptr, l_surface_id, 500, 500);
    ASSERT_EQ(ILM_NOTIFICATION_ALL, g_ilm_notification_mask);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_stats_invalidSurface
 * @brief               Test case of wm_listener_surface_stats() where invalid surface id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_stats()
 *                      -# Verification point:
 *                          +# property frameCounter and creatorPid of surface have ID 1 are not changed
 */
TEST_F(IlmControlTest, wm_listener_surface_stats_invalidSurface)
{
    uint32_t lSurfaceId{6};
    uint32_t lFrameCount{100};
    uint32_t lPid{3110};
    wm_listener_surface_stats(&ilm_context.wl, nullptr, lSurfaceId, lFrameCount, lPid);
    ASSERT_NE(mp_ctxSurface[0]->prop.frameCounter, lFrameCount);
    ASSERT_NE(mp_ctxSurface[0]->prop.creatorPid, lPid);
}

/** ================================================================================================
 * @test_id             wm_listener_surface_stats_success
 * @brief               Test case of wm_listener_surface_stats() where valid surface id
 * @test_procedure Steps:
 *                      -# Calling the wm_listener_surface_stats()
 *                      -# Verification point:
 *                          +# property frameCounter and creatorPid of surface have ID 1 are changed
 */
TEST_F(IlmControlTest, wm_listener_surface_stats_success)
{
    uint32_t lSurfaceId{1};
    uint32_t lFrameCount{100};
    uint32_t lPid{3110};
    wm_listener_surface_stats(&ilm_context.wl, nullptr, lSurfaceId, lFrameCount, lPid);
    ASSERT_EQ(mp_ctxSurface[0]->prop.frameCounter, lFrameCount);
    ASSERT_EQ(mp_ctxSurface[0]->prop.creatorPid, lPid);
}

/** ================================================================================================
 * @test_id             output_listener_geometry_success
 * @brief               Test case of output_listener_geometry() where passing valid input params
 * @test_procedure Steps:
 *                      -# Calling the output_listener_geometry()
 *                      -# Verification point:
 *                          +# element transform is not changed
 */
TEST_F(IlmControlTest, output_listener_geometry_success)
{
    int32_t lTransform{3110};
    output_listener_geometry(mp_ctxScreen[0], nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, lTransform);
    ASSERT_EQ(mp_ctxScreen[0]->transform, lTransform);
}

/** ================================================================================================
 * @test_id             output_listener_mode_failure
 * @brief               Test case of output_listener_mode() where passing valid input params
 *                      and input flag is false {0}
 * @test_procedure Steps:
 *                      -# Calling the output_listener_mode()
 *                      -# Verification point:
 *                          +# property screenWidth and screenHeight are not changed
 */
TEST_F(IlmControlTest, output_listener_mode_failure)
{
    uint32_t lFlags{0};
    int32_t lWidth{1920};
    int32_t lHeight{1080};

    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, 0);
}

/** ================================================================================================
 * @test_id             output_listener_mode_success
 * @brief               Test case of output_listener_mode() where passing valid input params
 *                      and input flag is true {1}
 * @test_procedure Steps:
 *                      -# Calling the output_listener_mode() multiples times with different input transform types
 *                      -# Verification point:
 *                          +# property screenWidth and screenHeight are changed
 */
TEST_F(IlmControlTest, output_listener_mode_success)
{
    uint32_t lFlags{1};
    int32_t lWidth{1920};
    int32_t lHeight{1080};

    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, lWidth);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, lHeight);
    // case WL_OUTPUT_TRANSFORM_90
    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    mp_ctxScreen[0]->transform = WL_OUTPUT_TRANSFORM_90;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, lHeight);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, lWidth);
    // case WL_OUTPUT_TRANSFORM_270
    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    mp_ctxScreen[0]->transform = WL_OUTPUT_TRANSFORM_270;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, lHeight);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, lWidth);
    // case WL_OUTPUT_TRANSFORM_FLIPPED_90
    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    mp_ctxScreen[0]->transform = WL_OUTPUT_TRANSFORM_FLIPPED_90;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, lHeight);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, lWidth);
    // case WL_OUTPUT_TRANSFORM_FLIPPED_90
    mp_ctxScreen[0]->prop.screenWidth = 0;
    mp_ctxScreen[0]->prop.screenHeight = 0;
    mp_ctxScreen[0]->transform = WL_OUTPUT_TRANSFORM_FLIPPED_270;
    output_listener_mode(mp_ctxScreen[0], nullptr, lFlags, lWidth, lHeight, 0);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenWidth, lHeight);
    ASSERT_EQ(mp_ctxScreen[0]->prop.screenHeight, lWidth);
}

/** ================================================================================================
 * @test_id             wm_screen_listener_screen_id_success
 * @brief               Test case of wm_screen_listener_screen_id() where passing valid input params
 * @test_procedure Steps:
 *                      -# Calling the wm_screen_listener_screen_id()
 *                      -# Verification point:
 *                         +# id_screen should be changed
 */
TEST_F(IlmControlTest, wm_screen_listener_screen_id_success)
{
    uint32_t screen_id{3110};
    wm_screen_listener_screen_id(mp_ctxScreen[0], nullptr, screen_id);
    ASSERT_EQ(mp_ctxScreen[0]->id_screen, screen_id);
}

/** ================================================================================================
 * @test_id             wm_screen_listener_layer_added_success
 * @brief               Test case of wm_screen_listener_layer_added() where wl_array_add() success, return a variable
 * @test_procedure Steps:
 *                      -# Mocking the wl_array_add() does return a variable
 *                      -# Calling the wm_screen_listener_layer_added() with input layer id is 100
 *                      -# Verification point:
 *                         +# wl_array_add() must be called once time
 *                         +# The variable must same input layer id
 *                      -# Free resources are allocated when running the test
 */
TEST_F(IlmControlTest, wm_screen_listener_layer_added_success)
{
    uint32_t l_dataScreen = 0;
    void *lp_elemData = &l_dataScreen;
    SET_RETURN_SEQ(wl_array_add, &lp_elemData, 100);

    wm_screen_listener_layer_added(&mp_ctxScreen[0], nullptr, 100);

    EXPECT_EQ(1, wl_array_add_fake.call_count);
    EXPECT_EQ(100, l_dataScreen);

    custom_wl_array_release(&mp_ctxScreen[0]->render_order);
}

/** ================================================================================================
 * @test_id             wm_screen_listener_connector_name_success
 * @brief               Test case of wm_screen_listener_connector_name() where passing valid input params
 * @test_procedure Steps:
 *                      -# Calling the wm_screen_listener_connector_name()
 */
TEST_F(IlmControlTest, wm_screen_listener_connector_name_success)
{
    char connector_name[] = "name_of_connector";
    wm_screen_listener_connector_name(mp_ctxScreen[0], nullptr, connector_name);
    ASSERT_EQ(strcmp(mp_ctxScreen[0]->prop.connectorName, connector_name), 0);
}

/** ================================================================================================
 * @test_id             wm_screen_listener_error_success
 * @brief               Test case of wm_screen_listener_error() where passing valid input params
 * @test_procedure Steps:
 *                      -# Set input data id_screen is 1 and ctx->error_flag is ILM_SUCCESS
 *                      -# Calling the wm_screen_listener_error() multiples times with different input error types
 */
TEST_F(IlmControlTest, wm_screen_listener_error_success)
{
    mp_ctxScreen[0]->id_screen = 1;
    wm_screen_listener_error(mp_ctxScreen[0], nullptr, IVI_WM_SCREEN_ERROR_NO_LAYER, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_RESOURCE_NOT_FOUND);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_screen_listener_error(mp_ctxScreen[0], nullptr, IVI_WM_SCREEN_ERROR_NO_SCREEN, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_RESOURCE_NOT_FOUND);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_screen_listener_error(mp_ctxScreen[0], nullptr, IVI_WM_SCREEN_ERROR_BAD_PARAM, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_INVALID_ARGUMENTS);
    ilm_context.wl.error_flag = ILM_SUCCESS;
    wm_screen_listener_error(mp_ctxScreen[0], nullptr, 3, "");
    ASSERT_EQ(ilm_context.wl.error_flag, ILM_ERROR_ON_CONNECTION);
}

/** ================================================================================================
 * @test_id             input_listener_seat_created_validSeat
 * @brief               Test case of input_listener_seat_created() where valid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_seat_created()
 *                      -# Verification point:
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, input_listener_seat_created_validSeat)
{
    input_listener_seat_created(&ilm_context.wl, nullptr, "seat2", 0, 1);

    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_listener_seat_created_newOne
 * @brief               Test case of input_listener_seat_created() where invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_seat_created()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(IlmControlTest, input_listener_seat_created_newOne)
{
    input_listener_seat_created(&ilm_context.wl, nullptr, "", 0, 1);

    EXPECT_EQ(1, wl_list_insert_fake.call_count);

    struct seat_context *lp_createSeat = (struct seat_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct seat_context, link));
    free(lp_createSeat->seat_name);
    free(lp_createSeat);
}

/** ================================================================================================
 * @test_id             input_listener_seat_capabilities_invalidSeat
 * @brief               Test case of input_listener_seat_capabilities() where invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_seat_capabilities()
 *                      -# Verification point:
 *                         +# element capabilities should be not changed
 */
TEST_F(IlmControlTest, input_listener_seat_capabilities_invalidSeat)
{
    char lSeatName[] = "invalid_name";
    uint32_t lCapabilities{10};
    mp_ctxSeat[0]->capabilities = 31;
    input_listener_seat_capabilities(&ilm_context.wl, nullptr, lSeatName, lCapabilities);
    ASSERT_EQ(mp_ctxSeat[0]->capabilities, 31);
}

/** ================================================================================================
 * @test_id             input_listener_seat_capabilities_success
 * @brief               Test case of input_listener_seat_capabilities() where valid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_seat_capabilities()
 *                      -# Verification point:
 *                         +# element capabilities should be changed
 */
TEST_F(IlmControlTest, input_listener_seat_capabilities_success)
{
    char lSeatName[] = "default";
    uint32_t lCapabilities{10};
    mp_ctxSeat[0]->capabilities = 31;
    input_listener_seat_capabilities(&ilm_context.wl, nullptr, lSeatName, lCapabilities);
    ASSERT_EQ(mp_ctxSeat[0]->capabilities, lCapabilities);
}

/** ================================================================================================
 * @test_id             input_listener_seat_destroyed_invalidSeat
 * @brief               Test case of input_listener_seat_destroyed() where invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_seat_destroyed()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(IlmControlTest, input_listener_seat_destroyed_invalidSeat)
{
    input_listener_seat_destroyed(&ilm_context.wl, nullptr, "");

    ASSERT_EQ(0, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_listener_seat_destroyed_success
 * @brief               Test case of input_listener_seat_destroyed() where valid input seat
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the input_listener_seat_destroyed()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(IlmControlTest, input_listener_seat_destroyed_success)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    input_listener_seat_destroyed(&ilm_context.wl, nullptr, "default");

    EXPECT_EQ(1, wl_list_remove_fake.call_count);

    mp_ctxSeat[0] = nullptr;
}

/** ================================================================================================
 * @test_id             input_listener_input_focus_invalidSurface
 * @brief               Test case of input_listener_input_focus() where invalid input surface id
 *                      and input enabled is false {0}
 * @test_procedure Steps:
 *                      -# Set mp_ctxSurface[0]->prop.focus to default value
 *                      -# Calling the input_listener_input_focus()
 *                      -# Verification point:
 *                         +# mp_ctxSurface[0]->prop.focus should be not changed
 */
TEST_F(IlmControlTest, input_listener_input_focus_invalidSurface)
{
    uint32_t l_surface_id = 6;
    int32_t l_enabled = 0;
    uint32_t l_device = 0xFFFFFFFF;
    mp_ctxSurface[0]->prop.focus = 0xFFFF;
    input_listener_input_focus(&ilm_context.wl, nullptr, l_surface_id, l_device, l_enabled);
    ASSERT_EQ(mp_ctxSurface[0]->prop.focus, 0xFFFF);
}

/** ================================================================================================
 * @test_id             input_listener_input_focus_unenabled
 * @brief               Test case of input_listener_input_focus() where valid input surface id
 *                      and input enabled is false {0}
 * @test_procedure Steps:
 *                      -# Set mp_ctxSurface[0]->prop.focus to default value
 *                      -# Calling the input_listener_input_focus()
 *                      -# Verification point:
 *                         +# mp_ctxSurface[0]->prop.focus should be changed
 */
TEST_F(IlmControlTest, input_listener_input_focus_unenabled)
{
    uint32_t l_surface_id = 1;
    int32_t l_enabled = 0;
    uint32_t l_device = 0xFFFFFFFF;
    mp_ctxSurface[0]->prop.focus = 0xFFFF;
    input_listener_input_focus(&ilm_context.wl, nullptr, l_surface_id, l_device, l_enabled);
    ASSERT_NE(mp_ctxSurface[0]->prop.focus, 0xFFFF);
}

/** ================================================================================================
 * @test_id             input_listener_input_focus_enabled
 * @brief               Test case of input_listener_input_focus() where valid input surface id
 *                      and input enabled is true {1}
 * @test_procedure Steps:
 *                      -# Set mp_ctxSurface[0]->prop.focus to default value
 *                      -# Calling the input_listener_input_focus()
 *                      -# Verification point:
 *                         +# mp_ctxSurface[0]->prop.focus should be changed
 */
TEST_F(IlmControlTest, input_listener_input_focus_enabled)
{
    uint32_t l_surface_id = 1;
    int32_t l_enabled = 1;
    uint32_t l_device = 0xFFFFFFFF;
    mp_ctxSurface[0]->prop.focus = 0xFFFF;
    input_listener_input_focus(&ilm_context.wl, nullptr, l_surface_id, l_device, l_enabled);
    ASSERT_NE(mp_ctxSurface[0]->prop.focus, 0xFFFF);
}

/** ================================================================================================
 * @test_id             input_listener_input_acceptance_invalidSurface
 * @brief               Test case of input_listener_input_acceptance() where invalid input surface id
 *                      and input accepted is true {1} and invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, input_listener_input_acceptance_invalidSurface)
{
    uint32_t l_surface_id = 6;
    int32_t l_accepted = 1;
    input_listener_input_acceptance(&ilm_context.wl, nullptr, l_surface_id, "", l_accepted);

    ASSERT_EQ(0, wl_list_remove_fake.call_count);
    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_listener_input_acceptance_invalidSeatAndAccepted
 * @brief               Test case of input_listener_input_acceptance() where valid input surface id
 *                      and input accepted is true {1} and invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 *                         +# wl_list_insert() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IlmControlTest, input_listener_input_acceptance_invalidSeatAndAccepted)
{
    uint32_t l_surface_id = 1;
    int32_t l_accepted = 1;
    input_listener_input_acceptance(&ilm_context.wl, nullptr, l_surface_id, "", l_accepted);

    EXPECT_EQ(0, wl_list_remove_fake.call_count);
    EXPECT_EQ(1, wl_list_insert_fake.call_count);

    struct seat_context *lp_accepted_seat = (struct seat_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct seat_context, link));
    free(lp_accepted_seat->seat_name);
    free(lp_accepted_seat);
}

/** ================================================================================================
 * @test_id             input_listener_input_acceptance_invalidSeatAndUnaccepted
 * @brief               Test case of input_listener_input_acceptance() where valid input surface id
 *                      and input accepted is false {0} and invalid input seat
 * @test_procedure Steps:
 *                      -# Calling the input_listener_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, input_listener_input_acceptance_invalidSeatAndUnaccepted)
{
    uint32_t l_surface_id = 1;
    int32_t l_accepted = 0;
    input_listener_input_acceptance(&ilm_context.wl, nullptr, l_surface_id, "", l_accepted);

    ASSERT_EQ(0, wl_list_remove_fake.call_count);
    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_listener_input_acceptance_validSeatAndAccepted
 * @brief               Test case of input_listener_input_acceptance() where valid input surface id
 *                      and input accepted is true {1} and valid input seat
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the input_listener_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 *                         +# wl_list_insert() not be called
 */
TEST_F(IlmControlTest, input_listener_input_acceptance_validSeatAndAccepted)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    uint32_t l_surface_id = 1;
    int32_t l_accepted = 1;
    input_listener_input_acceptance(&ilm_context.wl, nullptr, l_surface_id, "default", l_accepted);

    ASSERT_EQ(0, wl_list_remove_fake.call_count);
    ASSERT_EQ(0, wl_list_insert_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_listener_input_acceptance_validSeatAndUnaccepted
 * @brief               Test case of input_listener_input_acceptance() where valid input surface id
 *                      and input accepted is false {0} and valid input seat
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the input_listener_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 *                         +# wl_list_insert() not be called
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(IlmControlTest, input_listener_input_acceptance_validSeatAndUnaccepted)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    uint32_t surface_id = 1;
    int32_t accepted = 0;
    input_listener_input_acceptance(&ilm_context.wl, nullptr, surface_id, "default", accepted);

    EXPECT_EQ(1, wl_list_remove_fake.call_count);
    EXPECT_EQ(0, wl_list_insert_fake.call_count);

    mp_accepted_seat[0] = nullptr;
}

/** ================================================================================================
 * @test_id             registry_handle_control_success
 * @brief               Test case of registry_handle_control() where passing different input params
 * @test_procedure Steps:
 *                      -# Calling the registry_handle_control() time 1 with null interface
 *                      -# Calling the registry_handle_control() time 2 with interface is ivi_wm
 *                      -# Calling the registry_handle_control() time 3 with interface is ivi_input
 *                      -# Calling the registry_handle_control() time 4 with interface is wl_output
 *                      -# Mocking the wl_proxy_marshal_flags() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return -1 (failure)
 *                      -# Calling the registry_handle_control() time 5 with interface is wl_output
 *                      -# Mocking the wl_proxy_marshal_flags() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return 0 (success)
 *                      -# Calling the registry_handle_control() time 6 with interface is wl_output
 *                      -# Verification point:
 *                         +# wl_proxy_marshal_flags() not be called when calling time 1
 *                         +# wl_proxy_marshal_flags() must be called when calling time 2, time 3, time 4, time 5, time 6 with opcode WL_REGISTRY_BIND
 *                         +# wl_proxy_add_listener() not be called when calling time 1, time 2, time 3, time 4
 *                         +# wl_proxy_add_listener() must be called when calling time 5, time 6
 *                         +# wl_list_insert() must be called when calling time 6
 *                      -# Free resources are allocated when running the test
 * 
 */
TEST_F(IlmControlTest, registry_handle_control_success)
{
    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"", 0);
    EXPECT_EQ(0, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(0, wl_proxy_add_listener_fake.call_count);
    RESET_FAKE(wl_proxy_marshal_flags);

    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"ivi_wm", 0);
    EXPECT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_REGISTRY_BIND);
    EXPECT_EQ(0, wl_proxy_add_listener_fake.call_count);
    RESET_FAKE(wl_proxy_marshal_flags);

    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"ivi_input", 0);
    EXPECT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_REGISTRY_BIND);
    EXPECT_EQ(0, wl_proxy_add_listener_fake.call_count);
    RESET_FAKE(wl_proxy_marshal_flags);

    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"wl_output", 0);
    EXPECT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_REGISTRY_BIND);
    EXPECT_EQ(0, wl_proxy_add_listener_fake.call_count);
    RESET_FAKE(wl_proxy_marshal_flags);

    struct wl_proxy *id[1] = {(struct wl_proxy *)0xFFFFFFFF};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, id, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_failureResult, 1);
    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"wl_output", 0);
    EXPECT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_REGISTRY_BIND);
    EXPECT_EQ(1, wl_proxy_add_listener_fake.call_count);
    RESET_FAKE(wl_proxy_marshal_flags);
    RESET_FAKE(wl_proxy_add_listener);

    SET_RETURN_SEQ(wl_proxy_marshal_flags, id, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_successResult, 1);
    registry_handle_control(&ilm_context.wl, nullptr, 0 ,"wl_output", 0);
    EXPECT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_REGISTRY_BIND);
    EXPECT_EQ(1, wl_proxy_add_listener_fake.call_count);
    EXPECT_EQ(1, wl_list_insert_fake.call_count);
    struct screen_context *lp_createScreen = (struct screen_context*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct screen_context, link));
    free(lp_createScreen);
}

/** ================================================================================================
 * @test_id             registry_handle_control_remove_invalidName
 * @brief               Test case of registry_handle_control_remove() where invalid input name
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the registry_handle_control_remove()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(IlmControlTest, registry_handle_control_remove_invalidName)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    registry_handle_control_remove(&ilm_context.wl, nullptr, 10);

    ASSERT_EQ(0, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             registry_handle_control_remove_oneNode
 * @brief               Test case of registry_handle_control_remove() where valid input name
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_remove(), to remove real object
 *                      -# Calling the registry_handle_control_remove() time 1
 *                      -# Set input data controller and output to null pointer
 *                      -# Calling the registry_handle_control_remove() time 2
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(IlmControlTest, registry_handle_control_remove_oneNode)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;

    registry_handle_control_remove(&ilm_context.wl, nullptr, 0);
    EXPECT_EQ(1, wl_list_remove_fake.call_count);

    mp_ctxScreen[1]->controller = nullptr;
    mp_ctxScreen[1]->output = nullptr;
    registry_handle_control_remove(&ilm_context.wl, nullptr, 1);
    EXPECT_EQ(2, wl_list_remove_fake.call_count);

    mp_ctxScreen[0] = nullptr;
    mp_ctxScreen[1] = nullptr;
}

/** ================================================================================================
 * @test_id             ilmControl_init_failed
 * @brief               Test case of ilmControl_init() where do not mock for some functions
 *                      and ilm context initialized is false
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is true, ready init
 *                      -# Calling the ilmControl_init() time 1
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Calling the ilmControl_init() time 2
 *                      -# Verification point:
 *                         +# ilmControl_init() time 1 must return ILM_FAILED and wl_display_create_queue() should not be run
 *                         +# ilmControl_init() time 2 must return ILM_FAILED and wl_display_create_queue() should run 1 time
 */
TEST_F(IlmControlTest, ilmControl_init_failed)
{
    ilm_context.initialized = true;
    ASSERT_EQ(ILM_FAILED, ilmControl_init(1));
    ASSERT_EQ(0, wl_display_create_queue_fake.call_count);

    ilm_context.initialized = false;
    ASSERT_EQ(ILM_FAILED, ilmControl_init(1));
    ASSERT_EQ(1, wl_display_create_queue_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilmControl_init_error
 * @brief               Test case of ilmControl_init() where input nativedisplay is 0
 * @test_procedure Steps:
 *                      -# Calling the ilmControl_init()
 *                      -# Verification point:
 *                         +# ilmControl_init() must return ILM_ERROR_INVALID_ARGUMENTS
 *                         +# wl_list_init() should not be called
 */
TEST_F(IlmControlTest, ilmControl_init_error)
{
    ASSERT_EQ(ILM_ERROR_INVALID_ARGUMENTS, ilmControl_init(0));
    ASSERT_EQ(0, wl_list_init_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilmControl_init_failedByInitControl
 * @brief               Test case of ilmControl_init() where init_control() return false {-1}
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Mocking the wl_display_create_queue() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return -1 (failure)
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilmControl_init() time 1
 *                      -# Mocking the wl_proxy_marshal_flags() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return -1 (failure)
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilmControl_init() time 2
 *                      -# Mocking the wl_proxy_marshal_flags() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return 0 (success)
 *                      -# Mocking the wl_display_roundtrip_queue() does return -1 (failure)
 *                      -# Calling the ilmControl_init() time 3
 *                      -# Verification point:
 *                         +# ilmControl_init() time 1 and time 2 and time 3 must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilmControl_init_failedByInitControl)
{
    ilm_context.initialized = false;

    uint64_t l_wlEventQueueFakePointer = 0, l_wlProxyFakePointer = 0;
    struct wl_event_queue *lpp_wlEventQueue[] = {(struct wl_event_queue*)&l_wlEventQueueFakePointer};
    struct wl_proxy *lpp_wlProxy[] = {(struct wl_proxy*)&l_wlProxyFakePointer};
    SET_RETURN_SEQ(wl_display_create_queue, lpp_wlEventQueue, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    ASSERT_EQ(ILM_FAILED, ilmControl_init(1));

    SET_RETURN_SEQ(wl_proxy_marshal_flags, lpp_wlProxy, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    ASSERT_EQ(ILM_FAILED, ilmControl_init(1));

    SET_RETURN_SEQ(wl_proxy_marshal_flags, lpp_wlProxy, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_successResult, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);

    ASSERT_EQ(ILM_FAILED, ilmControl_init(1));
}

/** ================================================================================================
 * @test_id             ilmControl_init_success
 * @brief               Test case of ilmControl_init() where init_control() return success {0}
 * @test_procedure Steps:
 *                      -# Set the ilm context initialized is false, not ready init
 *                      -# Mocking the wl_display_create_queue() does return an object
 *                      -# Mocking the wl_proxy_marshal_flags() does return an object
 *                      -# Mocking the wl_proxy_add_listener() does return 0 (success)
 *                      -# Mocking the wl_display_roundtrip_queue() does return 0 (success)
 *                      -# Calling the ilmControl_init()
 *                      -# Verification point:
 *                         +# ilmControl_init() must return ILM_SUCCESS
 */
TEST_F(IlmControlTest, ilmControl_init_success)
{
    ilm_context.initialized = false;

    uint64_t l_wlEventQueueFakePointer = 0, l_wlProxyFakePointer = 0;
    struct wl_event_queue *lpp_wlEventQueue[] = {(struct wl_event_queue*)&l_wlEventQueueFakePointer};
    struct wl_proxy *lpp_wlProxy[] = {(struct wl_proxy*)&l_wlProxyFakePointer};
    ilm_context.wl.controller = (struct ivi_wm*)&m_iviWmControllerFakePointer;
    SET_RETURN_SEQ(wl_display_create_queue, lpp_wlEventQueue, 1);
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lpp_wlProxy, 1);
    SET_RETURN_SEQ(wl_proxy_add_listener, mp_successResult, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);

    ASSERT_EQ(ILM_SUCCESS, ilmControl_init(1));
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_invalidScreen
 * @brief               Test case of ilm_takeScreenshot() where invalid input screen ID
 * @test_procedure Steps:
 *                      -# Calling the ilm_takeScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_invalidScreen)
{
    t_ilm_uint lScreenID{1};
    t_ilm_const_string filename{"name_test"};
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_validScreen
 * @brief               ilm_takeScreenshot() is called with valid screen ID and valid/invalid file name
 * @test_procedure Steps:
 *                      -# Calling the ilm_takeScreenshot() time 1 with valid screen ID and invalid file name
 *                      -# Calling the ilm_takeScreenshot() time 2 with valid screen ID and valid file name
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() time 1 should return ILM_SUCCESS
 *                         +# ilm_takeScreenshot() time 2 should return ILM_FAILED
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_validScreen)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename(NULL);
    /*- valid screen ID
      - invalid file name*/
    ASSERT_EQ(ILM_SUCCESS, ilm_takeScreenshot(lScreenID, filename));
    /*- valid screen ID
      - valid file name*/
    filename = "name_test";
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_SupportArgb8888AndUnsupportWlshmGlobal
 * @brief               ilm_takeScreenshot() is called when argb8888 is supported and wl_shm global is unsupported
 * @test_procedure Steps:
 *                      -# make argb8888 is supported
 *                      -# point wl_shm to NULL
 *                      -# call ilm_takeScreenshot with valid screenID and file name
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should not be called
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_SupportArgb8888AndUnsupportWlshmGlobal)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};

    ilm_context.wl.wl_shm = NULL;
    ilm_context.wl.has_argb8888 = true;
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_SupportWlshmGlobalAndUnsupportArgb8888
 * @brief               ilm_takeScreenshot() is called when wl_shm global is supported and argb8888 is unsupported
 * @test_procedure Steps:
 *                      -# make wl_shm global is supported
 *                      -# make argb8888 is unsupported
 *                      -# call ilm_takeScreenshot with valid screenID and file name
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should not be called
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_SupportWlshmGlobalAndUnsupportArgb8888)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = false;
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_SendScreenshotRequestFailed
 * @brief               Send request take screenshot to server failed when ilm_takeScreenshot() is called
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return NULL object when ivi_wm_screen_screenshot() 
 *                         is called to send request to server
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should be called 5 times with corresponding opcode
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_SendScreenshotRequestFailed)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[3] = {(void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_DESTROY);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], IVI_WM_SCREEN_SCREENSHOT);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], WL_BUFFER_DESTROY);
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_CreateBufferFailed
 * @brief               Create buffer in sharemem pool failed when ilm_takeScreenshot() is called
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return NULL object when wl_shm_pool_create_buffer() 
 *                         is called to create buffer in sharemem pool
 *                      -# Call ilm_takeScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should be called 3 times with corresponding opcode
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_CreateBufferFailed)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[3] = {(void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 2);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 3);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_DESTROY);
}

/** ================================================================================================
 * @test_id             ilm_takeScreenshot_Successfully
 * @brief               ilm_takeScreenshot() is called with all valid data
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return success object
 *                      -# Mocking the wl_display_dispatch_queue() to return failure object
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED - Attually this function is done 
 *                            with successfully status, but need to assert with ILM_FAILED because in unittest 
 *                            context, client side does not get event from server, so need to make wl_display_dispatch_queue() 
 *                            return failure object to finish ilm_takeScreenshot() function.
 *                         +# wl_proxy_marshal_flags should be called 5 times with corresponding opcode
 *                         +# wl_proxy_add_listener should be called 1 times
 */
TEST_F(IlmControlTest, ilm_takeScreenshot_Successfully)
{
    t_ilm_uint lScreenID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[1] = {(void *)lValidAddress};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 1);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeScreenshot(lScreenID, filename));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    ASSERT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_DESTROY);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], IVI_WM_SCREEN_SCREENSHOT);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], WL_BUFFER_DESTROY);
}

/** ================================================================================================
 * @test_id             ilm_takeAsyncScreenshot_Successfully
 * @brief               ilm_takeAsyncScreenshot() is called with all valid data
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return success object
 *                      -# Call ilm_takeAsyncScreenshot() with valid screenID, callback_done, callback_error, user_data
 *                      -# Verification point:
 *                         +# ilm_takeAsyncScreenshot() must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags should be called 4 times with corresponding opcode
 *                         +# wl_proxy_add_listener should be called 1 times
 *                         +# wl_display_flush should be called 1 times
 *                         +# All callback of screenshot_context should be setted
 */
TEST_F(IlmControlTest, ilm_takeAsyncScreenshot_Successfully)
{
    t_ilm_uint lScreenID{10};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[1] = {(void *)lValidAddress};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // call and verify data
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncScreenshot(lScreenID, lValidAddress, lValidAddress, lValidAddress));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 4);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], IVI_WM_SCREEN_SCREENSHOT);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, lValidAddress);
    EXPECT_EQ(ctx_scrshot->callback_error, lValidAddress);
    EXPECT_EQ(ctx_scrshot->callback_priv, lValidAddress);
    // release resource
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_FileNameNull
 * @brief               ilm_takeSurfaceScreenshot() is called with file name is NULL
 * @test_procedure Steps:
 *                      -# Calling the ilm_takeSurfaceScreenshot() with file name is NULL
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_SUCCESS
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_FileNameNull)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{NULL};
    ASSERT_EQ(ILM_SUCCESS, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_InvalidSurfaceID
 * @brief               ilm_takeSurfaceScreenshot() is called with invalid surfaceID
 * @test_procedure Steps:
 *                      -# Mocking the wl_proxy_marshal_flags() return an success object
 *                      -# Call ilm_takeSurfaceScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should be called 1 time with opcode IVI_WM_SURFACE_GET
 *                         +# wl_display_roundtrip_queue should be called 1 time
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_InvalidSurfaceID)
{
    t_ilm_uint lInvalidSurfaceID{10};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    struct wl_proxy *lScreenshotRetList[1] = {(struct wl_proxy *)lValidAddress};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lInvalidSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_controllderIsNull
 * @brief               ilm_takeSurfaceScreenshot() is called when controller is point to NULL
 * @test_procedure Steps:
 *                      -# MaKe controller point to NULL
 *                      -# Call ilm_takeSurfaceScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags should not be called
 *                         +# wl_display_roundtrip_queue should not be called
 */

TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_controllderIsNull)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    // prepare data
    ilm_context.wl.controller = NULL;
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 0);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_wl_display_roundtrip_queue_Failed
 * @brief               Client wait server process all pending request when ilm_takeSurfaceScreenshot() is called
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_roundtrip_queue() return failure object
 *                      -# Call ilm_takeSurfaceScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() should be called 1 time with opcode IVI_WM_SURFACE_GET
 *                         +# wl_display_roundtrip_queue() should be called 1 time
 */

TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_wl_display_roundtrip_queue_Failed)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_failureResult, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_SupportArgb8888AndUnsupportWlshmGlobal
 * @brief               ilm_takeSurfaceScreenshot() is called when argb8888 is supported and wl_shm global is unsupported
 * @test_procedure Steps:
 *                      -# make argb8888 is supported
 *                      -# point wl_shm to NULL
 *                      -# call ilm_takeSurfaceScreenshot() with valid screenID and file name
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() should be called 1 time with opcode IVI_WM_SURFACE_GET
 *                         +# wl_display_roundtrip_queue() should be called 1 time
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_SupportArgb8888AndUnsupportWlshmGlobal)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = NULL;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[1] = {(void *)lValidAddress};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_SupportWlshmGlobalAndUnsupportArgb8888
 * @brief               ilm_takeSurfaceScreenshot() is called when wl_shm global is supported and argb8888 is unsupported
 * @test_procedure Steps:
 *                      -# make wl_shm global is supported
 *                      -# call ilm_takeSurfaceScreenshot() with valid screenID and file name
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() should be called 1 time with opcode IVI_WM_SURFACE_GET
 *                         +# wl_display_roundtrip_queue() should be called 1 time
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_SupportWlshmGlobalAndUnsupportArgb8888)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = false;
    void *lScreenshotRetList[1] = {(void *)lValidAddress};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 1);
    SET_RETURN_SEQ(wl_display_roundtrip_queue, mp_successResult, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_CreateBufferFailed
 * @brief               Create buffer in sharemem pool failed when ilm_takeSurfaceScreenshot() is called
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_display_dispatch_queue() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return NULL object when wl_shm_pool_create_buffer() 
 *                         is called to create buffer in sharemem pool
 *                      -# Call ilm_takeSurfaceScreenshot()
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() should be called 4 times with corresponding opcode
 *                         +# wl_display_roundtrip_queue() should be called 1 time
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_CreateBufferFailed)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[3] = {(void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_successResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 4);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_SendSurfaceScreenShotFailed
 * @brief               Send request take surface screenshot to server failed when ilm_takeSurfaceScreenshot() is called
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_display_dispatch_queue() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return NULL object when ivi_wm_surface_screenshot() 
 *                         is called to send request to server
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED
 *                         +# wl_proxy_marshal_flags() should be called 6 times with corresponding opcode
 *                         +# wl_display_roundtrip_queue() should be called 1 time
 *                         +# wl_proxy_add_listener() should not be called
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_SendSurfaceScreenShotFailed)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 4);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_successResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 6);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[5], WL_BUFFER_DESTROY);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_add_listener_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_takeSurfaceScreenshot_Successfully
 * @brief               ilm_takeSurfaceScreenshot() is called with all valid data
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return success object
 *                      -# Mocking the wl_display_dispatch_queue() to return failure object
 *                      -# Verification point:
 *                         +# ilm_takeSurfaceScreenshot() must return ILM_FAILED - Attually this function is done 
 *                            with successfully status, but need to assert with ILM_FAILED because in unittest 
 *                            context, client side does not get event from server, so need to make wl_display_dispatch_queue() 
 *                            return failure object to finish ilm_takeSurfaceScreenshot() function.
 *                         +# wl_proxy_marshal_flags should be called 6 times with corresponding opcode
 *                         +# wl_display_roundtrip_queue should be called 1 time
 *                         +# wl_proxy_add_listener should be called 1 time
 *                         +# wl_display_dispatch_queue should be called 1 time
 *                         +# wl_display_flush should not be called
 */
TEST_F(IlmControlTest, ilm_takeSurfaceScreenshot_Successfully)
{
    t_ilm_uint lSurfaceID{1};
    t_ilm_const_string filename{"name_test"};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // call and verify data
    ASSERT_EQ(ILM_FAILED, ilm_takeSurfaceScreenshot(filename, lSurfaceID));
    ASSERT_EQ(wl_proxy_marshal_flags_fake.call_count, 6);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    ASSERT_EQ(wl_proxy_marshal_flags_fake.arg1_history[5], WL_BUFFER_DESTROY);
    ASSERT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    ASSERT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    ASSERT_EQ(wl_display_flush_fake.call_count, 0);
    ASSERT_EQ(wl_display_dispatch_queue_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             ilm_takeAsyncSurfaceScreenshot_Successfully
 * @brief               ilm_takeAsyncScreenshot() is called with all valid data
 * @test_procedure Steps:
 *                      -# make wl_shm global and argb8888 are supported
 *                      -# Mocking the wl_proxy_get_version() to return success object
 *                      -# Mocking the wl_proxy_marshal_flags() to return success object
 *                      -# Call ilm_takeAsyncScreenshot() with valid screenID, callback_done, callback_error, user_data
 *                      -# Verification point:
 *                         +# ilm_takeAsyncScreenshot() must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags should be called 4 times with corresponding opcode
 *                         +# wl_proxy_add_listener should be called 1 times
 *                         +# wl_display_flush should be called 1 times
 *                         +# All callback of screenshot_context should be setted
 */
TEST_F(IlmControlTest, ilm_takeAsyncSurfaceScreenshot_Successfully)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    // mock wayland APIs
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // call and verify data
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, lValidAddress, lValidAddress, lValidAddress));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, lValidAddress);
    EXPECT_EQ(ctx_scrshot->callback_error, lValidAddress);
    EXPECT_EQ(ctx_scrshot->callback_priv, lValidAddress);
    // release resource
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_done_UnknownFileFormat
 * @brief               screenshot_done() process data when file format is not png or bmp
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# Setup file format is .txt format
 *                      -# call screenshot_done() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotDoneCallbackFunc() should be called
 */
TEST_F(IlmControlTest, screenshot_done_UnknownFileFormat)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.fd.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    // set file format
    ctx_scrshot->filename = "test.txt";
    // call API and verify data
    screenshot_done(ctx_scrshot, lValidAddress, 1);
    EXPECT_EQ(save_as_png_fake.call_count, 0);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 1);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.fd.load(), -1);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_done_pngFileFormatSaveAsPngPassed
 * @brief               screenshot_done() process data when file format is png and can save as png file successfully
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# Setup file format is .png format
 *                      -# Mocking save_as_png to return success object
 *                      -# call screenshot_done() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotDoneCallbackFunc() should be called
 *                         +# The ctx_scrshot->result should be passed
 */
TEST_F(IlmControlTest, screenshot_done_pngFileFormatSaveAsPngPassed)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.fd.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    // set file format
    ctx_scrshot->filename = "test.png";
    // mock save_as_png
    int save_as_png_passed[1] = {0};
    SET_RETURN_SEQ(save_as_png, save_as_png_passed, 1);
    // call API and verify data
    screenshot_done(ctx_scrshot, lValidAddress, 1);
    EXPECT_EQ(save_as_png_fake.call_count, 1);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 0);
    EXPECT_EQ(ctx_scrshot->result, ILM_SUCCESS);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.fd.load(), -1);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_done_pngFileFormatSaveAsPngFailed
 * @brief               screenshot_done() process data when file format is png and cannot save as png file
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# Setup file format is .png format
 *                      -# Mocking save_as_png to return failure object
 *                      -# call screenshot_done() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotDoneCallbackFunc() should be called
 *                         +# The ctx_scrshot->result should be failed
 */
TEST_F(IlmControlTest, screenshot_done_pngFileFormatSaveAsPngFailed)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.fd.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    // set file format
    ctx_scrshot->filename = "test.png";
    // mock save_as_png
    int save_as_png_failed[1] = {1};
    SET_RETURN_SEQ(save_as_png, save_as_png_failed, 1);
    // call API and verify data
    screenshot_done(ctx_scrshot, lValidAddress, 1);
    EXPECT_EQ(save_as_png_fake.call_count, 1);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 0);
    EXPECT_EQ(ctx_scrshot->result, ILM_FAILED);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.fd.load(), -1);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_done_bitmapFileFormatSaveAsBitmapPassed
 * @brief               screenshot_done() process data when file format is bmp and can save as bmp file successfully
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# Setup file format is .bmp format
 *                      -# Mocking save_as_bitmap to return success object
 *                      -# call screenshot_done() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotDoneCallbackFunc() should be called
 *                         +# The ctx_scrshot->result should be passed
 */
TEST_F(IlmControlTest, screenshot_done_bitmapFileFormatSaveAsBitmapPassed)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.fd.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    // set file format
    ctx_scrshot->filename = "test.bmp";
    // mock save_as_bitmap
    int save_as_bitmap_passed[1] = {0};
    SET_RETURN_SEQ(save_as_bitmap, save_as_bitmap_passed, 1);
    // call API and verify data
    screenshot_done(ctx_scrshot, lValidAddress, 1);
    EXPECT_EQ(save_as_png_fake.call_count, 0);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 1);
    EXPECT_EQ(ctx_scrshot->result, ILM_SUCCESS);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.fd.load(), -1);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_done_bitmapFileFormatSaveAsBitmapFailed
 * @brief               screenshot_done() process data when file format is bmp and can not save as bmp file
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# Setup file format is .bmp format
 *                      -# Mocking save_as_bitmap to return failure object
 *                      -# call screenshot_done() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotDoneCallbackFunc() should be called
 *                         +# The ctx_scrshot->result should be failed
 */
TEST_F(IlmControlTest, screenshot_done_bitmapFileFormatSaveAsBitmapFailed)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.fd.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *lScreenshotRetList[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int lVersionRetList[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, lScreenshotRetList, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, lVersionRetList, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    // set file format
    ctx_scrshot->filename = "test.bmp";
    // mock save_as_bitmap
    int save_as_bitmap_failed[1] = {1};
    SET_RETURN_SEQ(save_as_bitmap, save_as_bitmap_failed, 1);
    // call API and verify data
    screenshot_done(ctx_scrshot, lValidAddress, 1);
    EXPECT_EQ(save_as_png_fake.call_count, 0);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 1);
    EXPECT_EQ(ctx_scrshot->result, ILM_FAILED);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.fd.load(), -1);
    free(ctx_scrshot);
}

/** ================================================================================================
 * @test_id             screenshot_error_CheckCall
 * @brief               screenshot_error() should raise our setted callback
 * @test_procedure Steps:
 *                      -# Setup data to make ilm_takeAsyncSurfaceScreenshot() process successfully
 *                      -# call screenshot_error() to process
 *                      -# Verification point:
 *                         +# The callback ScreenshotErrorCallbackFunc() should be called
 */
TEST_F(IlmControlTest, screenshot_error_CheckCall)
{
    t_ilm_uint lSurfaceID{1};
    constexpr uint32_t lValidAddress{0xFFFFFFFF};
    // prepare data
    RESET_FAKE(save_as_png);
    RESET_FAKE(save_as_bitmap);
    screenshot_data_t screenshotData;
    screenshotData.error.store(-1);
    ilm_context.wl.wl_shm = lValidAddress;
    ilm_context.wl.has_argb8888 = true;
    void *screenshot[4] = {(void *)lValidAddress, (void *)lValidAddress, (void *)lValidAddress, NULL};
    int version[1] = {1};
    SET_RETURN_SEQ(wl_proxy_marshal_flags, screenshot, 3);
    SET_RETURN_SEQ(wl_display_dispatch_queue, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_proxy_get_version, version, 1);
    // setup callback for ilm_context
    EXPECT_EQ(ILM_SUCCESS, ilm_takeAsyncSurfaceScreenshot(lSurfaceID, ScreenshotDoneCallbackFunc, ScreenshotErrorCallbackFunc, &screenshotData));
    EXPECT_EQ(wl_proxy_marshal_flags_fake.call_count, 5);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[0], IVI_WM_SURFACE_GET);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[1], WL_SHM_CREATE_POOL);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[2], WL_SHM_POOL_CREATE_BUFFER);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[3], WL_SHM_POOL_DESTROY);
    EXPECT_EQ(wl_proxy_marshal_flags_fake.arg1_history[4], IVI_WM_SURFACE_SCREENSHOT);
    EXPECT_EQ(wl_display_roundtrip_queue_fake.call_count, 1);
    EXPECT_EQ(wl_display_flush_fake.call_count, 1);
    EXPECT_EQ(wl_proxy_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_display_dispatch_queue_fake.call_count, 0);
    struct screenshot_context *ctx_scrshot = wl_proxy_add_listener_fake.arg2_val;
    EXPECT_EQ(ctx_scrshot->callback_done, ScreenshotDoneCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_error, ScreenshotErrorCallbackFunc);
    EXPECT_EQ(ctx_scrshot->callback_priv, &screenshotData);
    ctx_scrshot->filename = "test.txt";
    screenshot_error(ctx_scrshot, lValidAddress, 1, "Error message");
    EXPECT_EQ(save_as_png_fake.call_count, 0);
    EXPECT_EQ(save_as_bitmap_fake.call_count, 0);
    destroy_shm_buffer(ctx_scrshot->ivi_buffer);
    // make sure call back is called
    EXPECT_NE(screenshotData.error.load(), -1);
    free(ctx_scrshot);
}