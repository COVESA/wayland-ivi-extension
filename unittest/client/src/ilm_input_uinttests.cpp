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
#include "ilm_input.h"
#include "ilm_control_platform.h"
#include "client_api_fake.h"

extern "C"{
    extern struct ilm_control_context ilm_context;
    FAKE_VALUE_FUNC(ilmErrorTypes, impl_sync_and_acquire_instance, struct ilm_control_context *);
    FAKE_VOID_FUNC(release_instance);
}

class IlmInputTest : public ::testing::Test
{
public:
    void SetUp()
    {
        CLIENT_API_FAKE_LIST(RESET_FAKE);
        RESET_FAKE(impl_sync_and_acquire_instance);
        RESET_FAKE(release_instance);

        init_ctx_list_content();
    }

    void TearDown()
    {
        deinit_ctx_list_content();
    }

    void init_ctx_list_content()
    {
        custom_wl_list_init(&ilm_context.wl.list_surface);
        custom_wl_list_init(&ilm_context.wl.list_seat);
        for(uint8_t i = 0; i < MAX_NUMBER; i++)
        {
            //prepare the surfaces
            mp_ctxSurface[i].id_surface = mp_ilmSurfaceIds[i];
            mp_ctxSurface[i].ctx = &ilm_context.wl;
            mp_ctxSurface[i].prop.focus = ILM_INPUT_DEVICE_KEYBOARD;
            custom_wl_list_insert(&ilm_context.wl.list_surface, &mp_ctxSurface[i].link);
            custom_wl_list_init(&mp_ctxSurface[i].list_accepted_seats);
            //prepare the accepted seat
            mp_acceptedSeat[i].seat_name = (char*)mp_seatName[i];
            custom_wl_list_insert(&mp_ctxSurface[i].list_accepted_seats, &mp_acceptedSeat[i].link);
            //prepare the seats
            mp_ctxSeat[i].capabilities = ILM_INPUT_DEVICE_ALL;
            mp_ctxSeat[i].seat_name = (char*)mp_seatName[i];
            custom_wl_list_insert(&ilm_context.wl.list_seat, &mp_ctxSeat[i].link);
        }
    }

    void deinit_ctx_list_content()
    {
        struct surface_context *l, *n;
        struct accepted_seat *seat, *seat_next;
        wl_list_for_each_safe(l, n, &ilm_context.wl.list_surface, link)
        {
            wl_list_for_each_safe(seat, seat_next, &l->list_accepted_seats, link)
            {
                custom_wl_list_remove(&seat->link);
            }
            custom_wl_list_remove(&l->link);
        }
    }

    static constexpr uint8_t MAX_NUMBER = 5;
    const char *SEAT_DEFAULT = "seat_not_found";

    t_ilm_surface mp_ilmSurfaceIds[MAX_NUMBER] = {1, 2, 3, 4, 5};
    const char *mp_seatName[MAX_NUMBER] = {"seat_1", "seat_2", "seat_3", "seat_4", "seat_5"};
    struct surface_context mp_ctxSurface[MAX_NUMBER] = {};
    struct seat_context mp_ctxSeat[MAX_NUMBER] = {};
    struct accepted_seat mp_acceptedSeat[MAX_NUMBER] = {};

    ilmErrorTypes mp_ilmErrorType[MAX_NUMBER] = {ILM_FAILED};

    t_ilm_string *mp_getSeats = nullptr;
    t_ilm_uint m_numberSeat = 0;
};

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_invalidAgrument
 * @brief               Test case of ilm_setInputAcceptanceOn() where invalid input seats
 * @test_procedure Steps:
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is a null pointer
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_invalidAgrument)
{
    ASSERT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(mp_ilmSurfaceIds[0], 1, nullptr));

    ASSERT_EQ(0, impl_sync_and_acquire_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_cannotSyncAcquireInstance
 * @brief               Test case of ilm_setInputAcceptanceOn() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 *                      and input seats is SEAT_DEFAULT
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is SEAT_DEFAULT
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_cannotSyncAcquireInstance)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(mp_ilmSurfaceIds[0], 1, (t_ilm_string*)&SEAT_DEFAULT));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_cannotFindSurfaceId
 * @brief               Test case of ilm_setInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and invalid input surface id and input seats is SEAT_DEFAULT
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is SEAT_DEFAULT
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_cannotFindSurfaceId)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(MAX_NUMBER + 1, 1, (t_ilm_string*)&SEAT_DEFAULT));

    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_cannotFindSeat
 * @brief               Test case of ilm_setInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid input surface id and input seats is SEAT_DEFAULT
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is SEAT_DEFAULT
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_cannotFindSeat)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(mp_ilmSurfaceIds[0], 1, (t_ilm_string*)&SEAT_DEFAULT));

    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_addNewOne
 * @brief               Test case of ilm_setInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid input surface id and input seats is mp_seatName
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is mp_seatName
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags() must be called once time
 *                         +# release_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_addNewOne)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(mp_ilmSurfaceIds[0], 2, (t_ilm_string*)&mp_seatName));

    ASSERT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputAcceptanceOn_removeOne
 * @brief               Test case of ilm_setInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid input surface id and input seats is mp_seatName[1]
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputAcceptanceOn() with input seats is mp_seatName[1]
 *                      -# Verification point:
 *                         +# ilm_setInputAcceptanceOn() must return ILM_SUCCESS
 *                         +# wl_proxy_marshal_flags() must be called 2 times
 *                         +# release_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputAcceptanceOn_removeOne)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(mp_ilmSurfaceIds[0], 1, (t_ilm_string*)&mp_seatName[1]));

    ASSERT_EQ(2, wl_proxy_marshal_flags_fake.call_count);
    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_getInputAcceptanceOn_invalidAgrument
 * @brief               Test case of ilm_getInputAcceptanceOn() where invalid input num_seats and seats
 * @test_procedure Steps:
 *                      -# Calling the ilm_getInputAcceptanceOn() time 1 with num_seats and seats are null pointer
 *                      -# Calling the ilm_getInputAcceptanceOn() time 2 with seats is null pointer
 *                      -# Verification point:
 *                         +# ilm_getInputAcceptanceOn() time 1 and time 2 must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_getInputAcceptanceOn_invalidAgrument)
{
    ASSERT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(mp_ilmSurfaceIds[0], nullptr, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(mp_ilmSurfaceIds[0], &m_numberSeat, nullptr));

    ASSERT_EQ(0, impl_sync_and_acquire_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_getInputAcceptanceOn_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getInputAcceptanceOn() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_getInputAcceptanceOn()
 *                      -# Verification point:
 *                         +# ilm_getInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time and return ILM_FAILED
 */
TEST_F(IlmInputTest, ilm_getInputAcceptanceOn_cannotSyncAcquireInstance)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(mp_ilmSurfaceIds[0], &m_numberSeat, &mp_getSeats));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], impl_sync_and_acquire_instance_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getInputAcceptanceOn_cannotFindSurfaceId
 * @brief               Test case of ilm_getInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and invalid surface id
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_getInputAcceptanceOn() with invalid surface id
 *                      -# Verification point:
 *                         +# ilm_getInputAcceptanceOn() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time
 *                         +# release_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_getInputAcceptanceOn_cannotFindSurfaceId)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(MAX_NUMBER + 1, &m_numberSeat, &mp_getSeats));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_getInputAcceptanceOn_success
 * @brief               Test case of ilm_getInputAcceptanceOn() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid surface id
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_getInputAcceptanceOn() with invalid surface id
 *                      -# Verification point:
 *                         +# ilm_getInputAcceptanceOn() must return ILM_SUCCESS
 *                         +# Output should same with prepare data
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IlmInputTest, ilm_getInputAcceptanceOn_success)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_getInputAcceptanceOn(mp_ilmSurfaceIds[0], &m_numberSeat, &mp_getSeats));

    EXPECT_EQ(1, m_numberSeat);
    EXPECT_EQ(0, strcmp(mp_getSeats[0], "seat_1"));
    EXPECT_EQ(1, release_instance_fake.call_count);

    free(mp_getSeats[0]);
    free(mp_getSeats);
}

/** ================================================================================================
 * @test_id             ilm_getInputDevices_invalidAgrument
 * @brief               Test case of ilm_getInputDevices() where invalid input num_seats and seats
 * @test_procedure Steps:
 *                      -# Calling the ilm_getInputDevices() time 1 with num_seats and seats are null pointer
 *                      -# Calling the ilm_getInputDevices() time 2 with seats is null pointer
 *                      -# Verification point:
 *                         +# ilm_getInputDevices() time 1 and time 2 must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_getInputDevices_invalidAgrument)
{
    ASSERT_EQ(ILM_FAILED, ilm_getInputDevices(ILM_INPUT_DEVICE_KEYBOARD, nullptr, nullptr));
    ASSERT_EQ(ILM_FAILED, ilm_getInputDevices(ILM_INPUT_DEVICE_KEYBOARD, &m_numberSeat, nullptr));

    ASSERT_EQ(impl_sync_and_acquire_instance_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_getInputDevices_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getInputDevices() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_getInputDevices()
 *                      -# Verification point:
 *                         +# ilm_getInputDevices() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time and return ILM_FAILED
 */
TEST_F(IlmInputTest, ilm_getInputDevices_cannotSyncAcquireInstance)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_getInputDevices(ILM_INPUT_DEVICE_KEYBOARD, &m_numberSeat, &mp_getSeats));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], impl_sync_and_acquire_instance_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getInputDevices_success
 * @brief               Test case of ilm_getInputDevices() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid bitmask
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_getInputDevices()
 *                      -# Verification point:
 *                         +# ilm_getInputDevices() must return ILM_SUCCESS
 *                         +# release_instance() must be called once time
 *                         +# Output should same with prepare data
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IlmInputTest, ilm_getInputDevices_success)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_getInputDevices(ILM_INPUT_DEVICE_KEYBOARD, &m_numberSeat, &mp_getSeats));

    EXPECT_EQ(1, release_instance_fake.call_count);
    EXPECT_EQ(5, m_numberSeat);
    for(uint8_t i = 0; i < m_numberSeat; i++)
    {
        EXPECT_EQ(0, strcmp(mp_getSeats[i], mp_seatName[m_numberSeat - i - 1]));
    }

    for(uint8_t i = 0; i < m_numberSeat; i++)
    {
        free(mp_getSeats[i]);
    }
    free(mp_getSeats);
}

/** ================================================================================================
 * @test_id             ilm_getInputDeviceCapabilities_invalidAgrument
 * @brief               Test case of ilm_getInputDeviceCapabilities() where invalid input seat_name and bitmask
 * @test_procedure Steps:
 *                      -# Calling the ilm_getInputDeviceCapabilities() time 1 with seat_name is null pointer
 *                      -# Calling the ilm_getInputDeviceCapabilities() time 2 with bitmask is null pointer
 *                      -# Verification point:
 *                         +# ilm_getInputDeviceCapabilities() time 1 and time 2 must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_getInputDeviceCapabilities_invalidAgrument)
{
    ilmInputDevice bitmask;
    t_ilm_string l_stringSeat = (t_ilm_string)"seat_1";
    ASSERT_EQ(ILM_FAILED, ilm_getInputDeviceCapabilities(nullptr, &bitmask));
    ASSERT_EQ(ILM_FAILED, ilm_getInputDeviceCapabilities(l_stringSeat, nullptr));

    ASSERT_EQ(impl_sync_and_acquire_instance_fake.call_count, 0);

}

/** ================================================================================================
 * @test_id             ilm_getInputDeviceCapabilities_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getInputDeviceCapabilities() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_getInputDeviceCapabilities()
 *                      -# Verification point:
 *                         +# ilm_getInputDeviceCapabilities() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time and return ILM_FAILED
 */
TEST_F(IlmInputTest, ilm_getInputDeviceCapabilities_cannotSyncAcquireInstance)
{
    ilmInputDevice bitmask;
    t_ilm_string l_stringSeat = (t_ilm_string)"seat_1";
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_FAILED, ilm_getInputDeviceCapabilities(l_stringSeat, &bitmask));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], impl_sync_and_acquire_instance_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getInputDeviceCapabilities_success
 * @brief               Test case of ilm_getInputDeviceCapabilities() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid bitmask
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_getInputDeviceCapabilities()
 *                      -# Verification point:
 *                         +# ilm_getInputDeviceCapabilities() must return ILM_SUCCESS
 *                         +# Output should same with prepare data
 *                         +# release_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_getInputDeviceCapabilities_success)
{
    ilmInputDevice bitmask;
    t_ilm_string l_stringSeat = (t_ilm_string)"seat_1";
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    ASSERT_EQ(ILM_SUCCESS, ilm_getInputDeviceCapabilities(l_stringSeat, &bitmask));

    ASSERT_EQ(ILM_INPUT_DEVICE_ALL, bitmask);
    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputFocus_invalidAgrument
 * @brief               Test case of ilm_setInputFocus() where invalid input seat_name and bitmask
 * @test_procedure Steps:
 *                      -# Calling the ilm_setInputFocus() time 1 with surface id is null pointer
 *                      -# Calling the ilm_setInputFocus() time 2 with surface id is not in the list of surface id
 *                      -# Verification point:
 *                         +# ilm_setInputFocus() time 1 and time 2 must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_setInputFocus_invalidAgrument)
{
    t_ilm_surface l_surfaceIDs[] = {MAX_NUMBER + 1, MAX_NUMBER + 2};
    ASSERT_EQ(ILM_FAILED, ilm_setInputFocus(nullptr, 2, ILM_INPUT_DEVICE_POINTER | ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));
    ASSERT_EQ(ILM_FAILED, ilm_setInputFocus(l_surfaceIDs, 2, ILM_INPUT_DEVICE_POINTER | ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));

    ASSERT_EQ(impl_sync_and_acquire_instance_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_setInputFocus_cannotSyncAcquireInstance
 * @brief               Test case of ilm_setInputFocus() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_setInputFocus()
 *                      -# Verification point:
 *                         +# ilm_setInputFocus() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time and return ILM_FAILED
 */
TEST_F(IlmInputTest, ilm_setInputFocus_cannotSyncAcquireInstance)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    t_ilm_surface l_surfaceIDs[] = {1, 2};
    ASSERT_EQ(ILM_FAILED, ilm_setInputFocus(l_surfaceIDs, 2, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], impl_sync_and_acquire_instance_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_setInputFocus_surfaceNotFound
 * @brief               Test case of ilm_setInputFocus() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and invalid input surface id
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputFocus()
 *                      -# Verification point:
 *                         +# ilm_setInputFocus() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time
 *                         +# release_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputFocus_surfaceNotFound)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    t_ilm_surface l_surfaceIDs[] = {MAX_NUMBER + 1, MAX_NUMBER + 2};
    ASSERT_EQ(ILM_FAILED, ilm_setInputFocus(l_surfaceIDs, 2, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(1, release_instance_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_setInputFocus_success
 * @brief               Test case of ilm_setInputFocus() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid input surface id
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_setInputFocus()
 *                      -# Verification point:
 *                         +# ilm_setInputFocus() must return ILM_SUCCESS
 *                         +# release_instance() must be called once time
 *                         +# impl_sync_and_acquire_instance() must be called once time
 */
TEST_F(IlmInputTest, ilm_setInputFocus_success)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    t_ilm_surface l_surfaceIDs[] = {1};
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputFocus(l_surfaceIDs, 1, ILM_INPUT_DEVICE_POINTER|ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));

    ASSERT_EQ(1, release_instance_fake.call_count);
    ASSERT_EQ(1, wl_proxy_marshal_flags_fake.call_count);
}

/** ================================================================================================
 * @test_id             ilm_getInputFocus_invalidAgrument
 * @brief               Test case of ilm_getInputFocus() where invalid inputs
 * @test_procedure Steps:
 *                      -# Calling the ilm_getInputFocus() time 1 with surfaceIDs is null pointer
 *                      -# Calling the ilm_getInputFocus() time 2 with bitmasks is null pointer
 *                      -# Calling the ilm_getInputFocus() time 3 with num_ids is null pointer
 *                      -# Verification point:
 *                         +# ilm_getInputFocus() time 1 and time 2 and time 3 must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() not be called
 */
TEST_F(IlmInputTest, ilm_getInputFocus_invalidAgrument)
{
    t_ilm_surface *lp_surfaceIds = nullptr;
    ilmInputDevice *lp_bitmasks = nullptr;
    t_ilm_uint l_numId = 0;
    ASSERT_EQ(ILM_FAILED, ilm_getInputFocus(nullptr, &lp_bitmasks, &l_numId));
    ASSERT_EQ(ILM_FAILED, ilm_getInputFocus(&lp_surfaceIds, nullptr, &l_numId));
    ASSERT_EQ(ILM_FAILED, ilm_getInputFocus(&lp_surfaceIds, &lp_bitmasks, nullptr));

    ASSERT_EQ(impl_sync_and_acquire_instance_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             ilm_getInputFocus_cannotSyncAcquireInstance
 * @brief               Test case of ilm_getInputFocus() where impl_sync_and_acquire_instance() fails, return ILM_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_FAILED
 *                      -# Calling the ilm_getInputFocus()
 *                      -# Verification point:
 *                         +# ilm_getInputFocus() must return ILM_FAILED
 *                         +# impl_sync_and_acquire_instance() must be called once time and return ILM_FAILED
 */
TEST_F(IlmInputTest, ilm_getInputFocus_cannotSyncAcquireInstance)
{
    mp_ilmErrorType[0] = ILM_FAILED;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    t_ilm_surface *lp_surfaceIds = nullptr;
    ilmInputDevice *lp_bitmasks = nullptr;
    t_ilm_uint l_numId = 0;
    ASSERT_EQ(ILM_FAILED, ilm_getInputFocus(&lp_surfaceIds, &lp_bitmasks, &l_numId));

    ASSERT_EQ(1, impl_sync_and_acquire_instance_fake.call_count);
    ASSERT_EQ(mp_ilmErrorType[0], impl_sync_and_acquire_instance_fake.return_val_history[0]);
}

/** ================================================================================================
 * @test_id             ilm_getInputFocus_success
 * @brief               Test case of ilm_getInputFocus() where impl_sync_and_acquire_instance() success, return ILM_SUCCESS
 *                      and valid inputs
 * @test_procedure Steps:
 *                      -# Mocking the impl_sync_and_acquire_instance() does return ILM_SUCCESS
 *                      -# Calling the ilm_getInputFocus()
 *                      -# Verification point:
 *                         +# ilm_getInputFocus() must return ILM_SUCCESS
 *                         +# release_instance() must be called once time
 *                         +# Output should same with prepare data
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IlmInputTest, ilm_getInputFocus_success)
{
    mp_ilmErrorType[0] = ILM_SUCCESS;
    SET_RETURN_SEQ(impl_sync_and_acquire_instance, mp_ilmErrorType, 1);

    t_ilm_surface *lp_surfaceIds = nullptr;
    ilmInputDevice *lp_bitmasks = nullptr;
    t_ilm_uint l_numId = 0;
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&lp_surfaceIds, &lp_bitmasks, &l_numId));

    EXPECT_EQ(1, release_instance_fake.call_count);
    EXPECT_EQ(5, l_numId);
    for(uint8_t i = 0; i < l_numId; i++)
    {
        EXPECT_EQ(lp_surfaceIds[i], mp_ilmSurfaceIds[l_numId - i -1]);
        EXPECT_EQ(lp_bitmasks[i], ILM_INPUT_DEVICE_KEYBOARD);
    }

    free(lp_surfaceIds);
    free(lp_bitmasks);
}
