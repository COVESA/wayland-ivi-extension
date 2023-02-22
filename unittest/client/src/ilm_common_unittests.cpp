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
#include "ilm_common.h"
#include "ilm_common_platform.h"
#include "client_api_fake.h"

extern "C"
{
    FAKE_VALUE_FUNC(ilmErrorTypes, ilmControl_init, t_ilm_nativedisplay);
    FAKE_VOID_FUNC(ilmControl_destroy);
    FAKE_VALUE_FUNC(ilmErrorTypes, ilmControl_registerShutdownNotification, shutdownNotificationFunc, void*);
}

class IlmCommonTest : public ::testing::Test 
{
public:
    void SetUp()
    {
        CLIENT_API_FAKE_LIST(RESET_FAKE);
        RESET_FAKE(ilmControl_init);
        RESET_FAKE(ilmControl_destroy);
        RESET_FAKE(ilmControl_registerShutdownNotification);
    }

    void TearDown()
    {
        if(ilm_isInitialized())
        {
            ilm_destroy();
        }
    }

    void mock_ilmInitSuccess()
    {
        mpp_wlDisplays[0] = (struct wl_display*)&m_wlDisplayFakePointer;
        SET_RETURN_SEQ(wl_display_connect, mpp_wlDisplays, MAX_NUMBER);

        mp_ilmErrorType[0] = ILM_SUCCESS;
        SET_RETURN_SEQ(ilmControl_init, mp_ilmErrorType, MAX_NUMBER);
    }

    static constexpr uint8_t MAX_NUMBER = 1;
    uint8_t m_wlDisplayFakePointer = 0;
    struct wl_display* mpp_wlDisplays [MAX_NUMBER] = {nullptr};
    ilmErrorTypes mp_ilmErrorType[MAX_NUMBER] = {ILM_FAILED};
};

/** ================================================================================================
 * @test_id             ilm_init_cannotGetDisplay
 * @brief               Test case of ilm_init() where wl_display_connect() fails, returns null object
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_connect() to return a null object
 *                      -# Calling the ilm_init()
 *                      -# Verification point:
 *                         +# ilm_init() must return ILM_FAILED
 *                         +# wl_display_connect() must be called once time and return nullptr
 */
TEST_F(IlmCommonTest, ilm_init_cannotGetDisplay)
{
    mpp_wlDisplays[0] = nullptr;
    SET_RETURN_SEQ(wl_display_connect, mpp_wlDisplays, MAX_NUMBER);

    ASSERT_EQ(ILM_FAILED, ilm_init());

    ASSERT_EQ(wl_display_connect_fake.call_count, 1);
    ASSERT_EQ(wl_display_connect_fake.return_val_history[0], nullptr);
}

/** ================================================================================================
 * @test_id             ilm_init_getFailedOnilmControl_init
 * @brief               Test case of ilm_init() where the internal function ilmControl_init() fails
 * @test_procedure Steps:
 *                      -# Mocking the wl_display_connect() doesn't return null object
 *                      -# Mocking the ilmControl_init() returns ILM_FAILED
 *                      -# Calling the ilm_init()
 *                      -# Verification point:
 *                         +# ilm_init() must return ILM_FAILED
 *                         +# ilmControl_init() must be called once time and return ILM_FAILED
 */
TEST_F(IlmCommonTest, ilm_init_getFailedOnilmControl_init)
{
    mpp_wlDisplays[0] = (struct wl_display*)&m_wlDisplayFakePointer;
    SET_RETURN_SEQ(wl_display_connect, mpp_wlDisplays, MAX_NUMBER);
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(ilmControl_init, mp_ilmErrorType, MAX_NUMBER);

    ASSERT_EQ(ILM_FAILED, ilm_init());

    ASSERT_EQ(ilmControl_init_fake.call_count, 1);
    ASSERT_EQ(mp_ilmErrorType[0], ilmControl_init_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_init_getSuccess
 * @brief               Test case of ilm_init() where wl_display_connect() success, return an object
 *                      and the internal function ilmControl_init() success
 * @test_procedure Steps:
 *                      -# Call mock_ilmInitSuccess() to mocking the wl_display_connect()
 *                         doesn't return null object and mocking the ilmControl_init() returns ILM_SUCCESS
 *                      -# Calling the ilm_init()
 *                      -# Verification point:
 *                         +# ilm_init() must return ILM_SUCCESS
 *                         +# wl_display_connect() must be called once time
 *                         +# ilmControl_init() must be called once time and retrun ILM_SUCCESS
 */
TEST_F(IlmCommonTest, ilm_init_getSuccess)
{
    mock_ilmInitSuccess();

    ASSERT_EQ(ILM_SUCCESS, ilm_init());

    ASSERT_EQ(wl_display_connect_fake.call_count, 1);
    ASSERT_EQ(ilmControl_init_fake.call_count, 1);
    ASSERT_EQ(ILM_SUCCESS, ilmControl_init_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_init_manyTimes
 * @brief               Test case to ensure the number of calls to ilm_init() and ilm_destroy() must correspond
 *                      Can't destroy without init
 * @test_procedure Steps:
 *                      -# Call mock_ilmInitSuccess() to mocking the wl_display_connect()
 *                         doesn't return null object and mocking the ilmControl_init() returns ILM_SUCCESS
 *                      -# Calling multiple the ilm_init() and ilm_destroy()
 *                      -# Verification point:
 *                         +# 2 calls to ilm_init() must return ILM_SUCCESS
 *                         +# 2 calls to ilm_destroy() respectively must return ILM_SUCCESS
 *                         +# The last call ilm_destroy() does not correspond to lm_init(), so it must return ILM_FAILED
 */
TEST_F(IlmCommonTest, ilm_init_manyTimes)
{
    mock_ilmInitSuccess();

    ASSERT_EQ(ILM_SUCCESS, ilm_init());
    ASSERT_EQ(ILM_SUCCESS, ilm_init());
    ASSERT_EQ(ILM_SUCCESS, ilm_destroy());
    ASSERT_EQ(ILM_SUCCESS, ilm_destroy());
    ASSERT_EQ(ILM_FAILED, ilm_destroy());
}

/** ================================================================================================
 * @test_id             ilm_initWithNativedisplay_existDisplay
 * @brief               Test case of ilm_initWithNativedisplay() where wl_display_connect() success, return an object
 *                      and the internal function ilmControl_init() success, but input exists a display
 * @test_procedure Steps:
 *                      -# Call mock_ilmInitSuccess() to mocking the wl_display_connect()
 *                         doesn't return null object and mocking the ilmControl_init() returns ILM_SUCCESS
 *                      -# Calling the ilm_initWithNativedisplay() with input nativedisplay is 1 (exist)
 *                      -# Verification point:
 *                         +# ilm_initWithNativedisplay() must return ILM_SUCCESS
 *                         +# wl_display_connect() not be called because exist a display
 *                         +# ilmControl_init() must be called once time and retrun ILM_SUCCESS
 */
TEST_F(IlmCommonTest, ilm_initWithNativedisplay_existDisplay)
{
    mock_ilmInitSuccess();

    ASSERT_EQ(ILM_SUCCESS, ilm_initWithNativedisplay(1));

    ASSERT_EQ(wl_display_connect_fake.call_count, 0);
    ASSERT_EQ(ilmControl_init_fake.call_count, 1);
    ASSERT_EQ(ILM_SUCCESS, ilmControl_init_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_isInitialized_getFalse
 * @brief               Test case of ilm_isInitialized() where ilm_init() is not called, ilmControl is not inittialized
 * @test_procedure Steps:
 *                      -# Calling the ilm_isInitialized()
 *                      -# Verification point:
 *                         +# ilm_isInitialized() must return ILM_FALSE
 */
TEST_F(IlmCommonTest, ilm_isInitialized_getFalse)
{
    ASSERT_EQ(ILM_FALSE, ilm_isInitialized());
}

/** ================================================================================================
 * @test_id             ilm_isInitialized_getTrue
 * @brief               Test case of ilm_isInitialized() where ilm_init() is called, ilmControl is inittialized
 * @test_procedure Steps:
 *                      -# Call mock_ilmInitSuccess() to mocking the wl_display_connect()
 *                         doesn't return null object and mocking the ilmControl_init() returns ILM_SUCCESS
 *                      -# Calling the ilm_init()
 *                      -# Calling the ilm_isInitialized()
 *                      -# Verification point:
 *                         +# ilm_init() must return ILM_SUCCESS
 *                         +# ilm_isInitialized() must return ILM_TRUE
 */
TEST_F(IlmCommonTest, ilm_isInitialized_getTrue)
{
    mock_ilmInitSuccess();

    ASSERT_EQ(ILM_SUCCESS, ilm_init());
    ASSERT_EQ(ILM_TRUE, ilm_isInitialized());
}

/** ================================================================================================
 * @test_id             ilm_registerShutdownNotification_getFailure
 * @brief               Test case of ilm_registerShutdownNotification()
 *                      where the internal function ilmControl_registerShutdownNotification fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the ilmControl_registerShutdownNotification() does return ILM_FAILED
 *                      -# Calling the ilm_registerShutdownNotification()
 *                      -# Verification point:
 *                         +# ilm_registerShutdownNotification() must return ILM_FAILED
 *                         +# ilmControl_registerShutdownNotification() must be called once time and retrun ILM_FAILED
 */
TEST_F(IlmCommonTest, ilm_registerShutdownNotification_getFailure)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(ilmControl_registerShutdownNotification, mp_ilmErrorType, MAX_NUMBER);

    ASSERT_EQ(ILM_FAILED, ilm_registerShutdownNotification(nullptr, nullptr));

    ASSERT_EQ(1, ilmControl_registerShutdownNotification_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], ilmControl_registerShutdownNotification_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_registerShutdownNotification_getSuccess
 * @brief               Test case of ilm_registerShutdownNotification()
 *                      where the internal function ilmControl_registerShutdownNotification() success, return ILM_SUCCESS
 * @test_procedure Steps:
 *                      -# Mocking the ilmControl_registerShutdownNotification() does return ILM_SUCCESS
 *                      -# Calling the ilm_registerShutdownNotification()
 *                      -# Verification point:
 *                         +# ilm_registerShutdownNotification() must return ILM_SUCCESS
 *                         +# ilmControl_registerShutdownNotification() must be called once time and retrun ILM_SUCCESS
 */
TEST_F(IlmCommonTest, ilm_registerShutdownNotification_getSuccess)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(ilmControl_registerShutdownNotification, mp_ilmErrorType, MAX_NUMBER);

    ASSERT_EQ(ILM_SUCCESS, ilm_registerShutdownNotification(nullptr, nullptr));

    ASSERT_EQ(1, ilmControl_registerShutdownNotification_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], ilmControl_registerShutdownNotification_fake.return_val_history[0]);
}
