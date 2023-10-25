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
#include "ivi-controller.h"
#include "ivi-input-server-protocol.h"
#include "ivi_layout_structure.hpp"
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"
#include "ilm_types.h"

static constexpr uint8_t MAX_NUMBER = 2;
static constexpr uint8_t DEFAULT_SEAT = 0;
static constexpr uint8_t CUSTOM_SEAT = 1;

extern "C"
{
FAKE_VALUE_FUNC(struct ivi_layout_surface *, get_surface, struct weston_surface *);

struct wl_resource * custom_wl_resource_from_link(struct wl_list *link)
{
	struct wl_resource *resource;

	return wl_container_of(link, resource, link);
}

struct wl_list * custom_wl_resource_get_link(struct wl_resource *resource)
{
	return &resource->link;
}

}

extern "C"
{
#include "ivi-input-controller.c"
}

class InputControllerTest: public ::testing::Test
{
public:
    void SetUp()
    {
        IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
        SERVER_API_FAKE_LIST(RESET_FAKE);
        init_input_controller_content();
    }

    void TearDown()
    {
        deinit_input_controller_content();
    }

    void init_input_controller_content()
    {
        // Prepare sample for ivi shell
        m_iviShell.interface = &g_iviLayoutInterfaceFake;
        m_iviShell.compositor = &m_westonCompositor;
        custom_wl_list_init(&m_iviShell.list_surface);
        custom_wl_list_init(&m_westonCompositor.seat_list);
        // Prepare the input context
        mp_ctxInput = (struct input_context*)calloc(1, sizeof(struct input_context));
        mp_ctxInput->ivishell = &m_iviShell;
        custom_wl_list_init(&mp_ctxInput->seat_list);
        custom_wl_list_init(&mp_ctxInput->resource_list);

        for(uint8_t i = 0; i < MAX_NUMBER; i++)
        {
            //Prepare for weston seats
            mp_westonSeat[i].seat_name = (char*)mp_seatName[i];
            custom_wl_list_init(&mp_westonSeat[i].destroy_signal.listener_list);
            custom_wl_list_init(&mp_westonSeat[i].updated_caps_signal.listener_list);
            //Prepare for seat context
            mpp_ctxSeat[i] = (struct seat_ctx*)calloc(1, sizeof(struct seat_ctx));
            mpp_ctxSeat[i]->input_ctx = mp_ctxInput;
            mpp_ctxSeat[i]->west_seat = &mp_westonSeat[i];
            mpp_ctxSeat[i]->pointer_grab.interface = &g_grabInterfaceFake;
            custom_wl_list_insert(&mp_ctxInput->seat_list, &mpp_ctxSeat[i]->seat_node);
            //Prepare for resource
            mp_wlResource[i].object.id = i + 1;
            custom_wl_list_insert(&mp_ctxInput->resource_list, &mp_wlResource[i].link);
            //Prepare for surface
            mp_layoutSurface[i].id_surface = i + 10;
            mp_iviSurface[i].shell = &m_iviShell;
            mp_iviSurface[i].layout_surface = &mp_layoutSurface[i];
            custom_wl_list_init(&mp_iviSurface[i].accepted_seat_list);
            custom_wl_list_insert(&m_iviShell.list_surface, &mp_iviSurface[i].link);
            // Prepare for accepted
            mpp_seatFocus[i] = (struct seat_focus*)calloc(1, sizeof(struct seat_focus));
            mpp_seatFocus[i]->seat_ctx =  mpp_ctxSeat[i];
            custom_wl_list_insert(&mp_iviSurface[i].accepted_seat_list, &mpp_seatFocus[i]->link);
        }
    }

    void enable_utility_funcs_of_array_list()
    {
        wl_list_init_fake.custom_fake = custom_wl_list_init;
        wl_list_insert_fake.custom_fake = custom_wl_list_insert;
        wl_list_remove_fake.custom_fake = custom_wl_list_remove;
        wl_list_empty_fake.custom_fake = custom_wl_list_empty;
        wl_array_init_fake.custom_fake = custom_wl_array_init;
        wl_array_release_fake.custom_fake = custom_wl_array_release;
        wl_array_add_fake.custom_fake = custom_wl_array_add;
        wl_signal_init(&m_iviShell.ivisurface_created_signal);
        wl_signal_init(&m_iviShell.ivisurface_removed_signal);
        wl_signal_init(&m_iviShell.compositor->destroy_signal);
        wl_signal_init(&m_iviShell.compositor->seat_created_signal);
    }

    void deinit_input_controller_content()
    {
        if(mp_ctxInput != nullptr)
        {
            free(mp_ctxInput);
        }

        for(uint8_t i = 0; i< MAX_NUMBER; i++)
        {
            if(mpp_ctxSeat[i] != nullptr)
            {
                free(mpp_ctxSeat[i]);
            }
            if(mpp_seatFocus[i] != nullptr)
            {
                free(mpp_seatFocus[i]);
            }
        }
    }

    struct input_context *mp_ctxInput = nullptr;
    struct wl_resource mp_wlResource[MAX_NUMBER];

    struct seat_ctx *mpp_ctxSeat[MAX_NUMBER] = {nullptr};
    struct weston_seat mp_westonSeat[MAX_NUMBER];
    const char* mp_seatName[MAX_NUMBER] = {"default", "weston_seat_1"};

    struct seat_focus *mpp_seatFocus[MAX_NUMBER] = {nullptr};
    struct ivisurface mp_iviSurface[MAX_NUMBER];
    struct ivi_layout_surface mp_layoutSurface[MAX_NUMBER];

    struct weston_compositor m_westonCompositor = {};
    struct ivishell m_iviShell = {};
};

/** ================================================================================================
 * @test_id             handle_seat_create_customSeat
 * @brief               Test case of handle_seat_create() where valid input params with seat name is a custom seat
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_create()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 3 times
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, handle_seat_create_customSeat)
{
    enable_utility_funcs_of_array_list();
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    mp_ctxInput->seat_default_name = (char*)mp_seatName[CUSTOM_SEAT];
    handle_seat_create(&mp_ctxInput->seat_create_listener, &mp_westonSeat[CUSTOM_SEAT]);
    ASSERT_EQ(wl_list_insert_fake.call_count, 5);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 6);

    struct seat_ctx *lp_ctxSeat = (struct seat_ctx *)((uintptr_t)wl_list_insert_fake.arg1_history[0] - offsetof(struct seat_ctx, seat_node));
    for(uint8_t i = 0; i < MAX_NUMBER; i++)
    {
        struct seat_focus *lp_seatFocus = (struct seat_focus *)((uintptr_t)wl_list_insert_fake.arg1_history[i+3] - offsetof(struct seat_focus, link));
        free(lp_seatFocus);
    }
    free(lp_ctxSeat);
}

/** ================================================================================================
 * @test_id             handle_seat_create_defaultSeat
 * @brief               Test case of handle_seat_create() where valid input params with seat name is default seat
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_create()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 5 times
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER + MAX_NUMBER*MAX_NUMBER} times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, handle_seat_create_defaultSeat)
{
    enable_utility_funcs_of_array_list();
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    mp_ctxInput->seat_default_name = (char*)mp_seatName[DEFAULT_SEAT];
    handle_seat_create(&mp_ctxInput->seat_create_listener, &mp_westonSeat[DEFAULT_SEAT]);

    ASSERT_EQ(wl_list_insert_fake.call_count, 5);
    ASSERT_EQ(MAX_NUMBER + MAX_NUMBER*MAX_NUMBER, wl_resource_post_event_fake.call_count);

    struct seat_ctx *lp_ctxSeat = (struct seat_ctx *)((uintptr_t)wl_list_insert_fake.arg1_history[0] - offsetof(struct seat_ctx, seat_node));
    for(uint8_t i = 0; i < MAX_NUMBER; i++)
    {
        struct seat_focus *lp_seatFocus = (struct seat_focus *)((uintptr_t)wl_list_insert_fake.arg1_history[i+3] - offsetof(struct seat_focus, link));
        free(lp_seatFocus);
    }
    free(lp_ctxSeat);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_allAvailableWithSameAdrr
 * @brief               Test case of handle_seat_updated_caps() where object address of mocked functions
 *                      is same as input address
 * @test_procedure Steps:
 *                      -# Mocking the weston_seat_get_keyboard() does return an object with address 0xFFFFFFFF
 *                      -# Mocking the weston_seat_get_pointer() does return an object with address 0xFFFFFFFF
 *                      -# Mocking the weston_seat_get_touch() does return an object with address 0xFFFFFFFF
 *                      -# Set input param keyboard_grab.keyboard to an object with address 0xFFFFFFFF
 *                      -# Set input param pointer_grab.pointer to an object with address 0xFFFFFFFF
 *                      -# Set input param touch_grab.touch to an object with address 0xFFFFFFFF
 *                      -# Calling the handle_seat_updated_caps()
 *                      -# Verification point:
 *                         +# weston_keyboard_start_grab() not be called
 *                         +# weston_pointer_start_grab() not be called
 *                         +# weston_touch_start_grab() not be called
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 */
TEST_F(InputControllerTest, handle_seat_updated_caps_allAvailableWithSameAdrr)
{
    struct weston_keyboard *lpp_keyboard [] = {(struct weston_keyboard *)0xFFFFFFFF};
    struct weston_pointer *lpp_pointer [] = {(struct weston_pointer *)0xFFFFFFFF};
    struct weston_touch *lpp_touch [] = {(struct weston_touch *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_keyboard, lpp_keyboard, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, lpp_pointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, lpp_touch, 1);

    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = {(struct weston_keyboard *)0xFFFFFFFF};
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = {(struct weston_pointer *)0xFFFFFFFF};
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = {(struct weston_touch *)0xFFFFFFFF};

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(0, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_allAvailableWithNotSameAdrr
 * @brief               Test case of handle_seat_updated_caps() where object address of mocked functions
 *                      is different from input address
 * @test_procedure Steps:
 *                      -# Mocking the weston_seat_get_keyboard() does return an object with address 0xFFFFFFFF
 *                      -# Mocking the weston_seat_get_pointer() does return an object with address 0xFFFFFFFF
 *                      -# Mocking the weston_seat_get_touch() does return an object with address 0xFFFFFFFF
 *                      -# Set input param keyboard_grab.keyboard to null pointer
 *                      -# Set input param pointer_grab.pointer to null pointer
 *                      -# Set input param touch_grab.touch to null pointer
 *                      -# Calling the handle_seat_updated_caps()
 *                      -# Verification point:
 *                         +# weston_keyboard_start_grab() must be called once time
 *                         +# weston_pointer_start_grab() must be called once time
 *                         +# weston_touch_start_grab() must be called once time
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 */
TEST_F(InputControllerTest, handle_seat_updated_caps_allAvailableWithNotSameAdrr)
{
    struct weston_keyboard *lpp_keyboard [] = {(struct weston_keyboard *)0xFFFFFFFF};
    struct weston_pointer *lpp_pointer [] = {(struct weston_pointer *)0xFFFFFFFF};
    struct weston_touch *lpp_touch [] = {(struct weston_touch *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_keyboard, lpp_keyboard, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, lpp_pointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, lpp_touch, 1);

    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = nullptr;

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(1, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(1, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(1, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_notAvailable
 * @brief               Test case of handle_seat_updated_caps() where weston_seat_get_keyboard() and weston_seat_get_pointer()
 *                      and weston_seat_get_touch() are not mocked
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_updated_caps() time 1
 *                      -# Set input param keyboard_grab.keyboard to an object with address 0xFFFFFFFF
 *                      -# Set input param pointer_grab.pointer to an object with address 0xFFFFFFFF
 *                      -# Set input param touch_grab.touch to an object with address 0xFFFFFFFF
 *                      -# Calling the handle_seat_updated_caps() time 2
 *                      -# Verification point:
 *                         +# weston_keyboard_start_grab() not be called
 *                         +# weston_pointer_start_grab() not be called
 *                         +# weston_touch_start_grab() not be called
 *                         +# keyboard_grab.keyboard set to null pointer
 *                         +# pointer_grab.pointer set to null pointer
 *                         +# touch_grab.touch set to null pointer
 *                         +# wl_resource_post_event() must be called {2*MAX_NUMBER} times
 */
TEST_F(InputControllerTest, handle_seat_updated_caps_notAvailable)
{
    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mp_westonSeat[CUSTOM_SEAT]);

    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = (struct weston_keyboard *)0xFFFFFFFF;
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = (struct weston_pointer *)0xFFFFFFFF;
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = (struct weston_touch *)0xFFFFFFFF;

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(0, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch);
    ASSERT_EQ(2*MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_destroy_notSurfaceAccepted
 * @brief               Test case of handle_seat_destroy() where seat_list is null list
 * @test_procedure Steps:
 *                      -# Prepare the data input, the seat_ctx object
 *                      -# Calling the handle_seat_destroy()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 *                         +# wl_list_remove() must be called 3 times
 */
TEST_F(InputControllerTest, handle_seat_destroy_notSurfaceAccepted)
{
    struct seat_ctx* lp_ctxSeat = (struct seat_ctx*)calloc(1, sizeof(struct seat_ctx));
    lp_ctxSeat->input_ctx = mp_ctxInput;
    lp_ctxSeat->west_seat = &mp_westonSeat[CUSTOM_SEAT];

    handle_seat_destroy(&lp_ctxSeat->destroy_listener, &mp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
    ASSERT_EQ(3, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_destroy_withSurfaceAccepted
 * @brief               Test case of handle_seat_destroy() where exist elements in seat_list
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_destroy()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 *                         +# wl_list_remove() must be called 4 times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, handle_seat_destroy_withSurfaceAccepted)
{
    handle_seat_destroy(&mpp_ctxSeat[CUSTOM_SEAT]->destroy_listener, &mp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
    ASSERT_EQ(4, wl_list_remove_fake.call_count);

    mpp_ctxSeat[CUSTOM_SEAT] = nullptr;
    mpp_seatFocus[CUSTOM_SEAT] = nullptr;
}

/** ================================================================================================
 * @test_id             input_controller_module_init_WrongInput
 * @brief               Test case of input_controller_module_init() where interface in ivishell object is a null pointer
 * @test_procedure Steps:
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must return 0
 */
TEST_F(InputControllerTest, input_controller_module_init_WrongInput)
{
    m_iviShell.interface = nullptr;
    ASSERT_NE(input_controller_module_init(&m_iviShell), 0);
}

/** ================================================================================================
 * @test_id             input_controller_module_init_cannotCreateIviInput
 * @brief               Test case of input_controller_module_init() where wl_global_create() is not mocked, returns null pointer
 * @test_procedure Steps:
 *                      -# Calling the enable_utility_funcs_of_array_list() to set real function for wl_list, wl_array
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must return 0
 *                         +# wl_global_create() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, input_controller_module_init_cannotCreateIviInput)
{
    enable_utility_funcs_of_array_list();
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    struct wl_global *lp_wlGlobal = (struct wl_global*) 0xFFFFFFFF;
    SET_RETURN_SEQ(wl_global_create, &lp_wlGlobal, 1);
    ASSERT_EQ(input_controller_module_init(&m_iviShell), 0);

    EXPECT_EQ(wl_global_create_fake.call_count, 1);

    struct input_context * tmp = wl_global_create_fake.arg3_val;
    free(tmp->seat_default_name);
    free(tmp);
}

/** ================================================================================================
 * @test_id             input_controller_module_init_canInitSuccess
 * @brief               Test case of input_controller_module_init() where wl_global_create() is mocked, returns an object
 * @test_procedure Steps:
 *                      -# Calling the enable_utility_funcs_of_array_list() to set real function for wl_list, wl_array
 *                      -# Mocking the wl_global_create() does return an object
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must return 0
 *                         +# wl_global_create() must be called once time
 *                         +# Input controller module member must same the prepared data
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, input_controller_module_init_canInitSuccess)
{
    enable_utility_funcs_of_array_list();
    struct wl_global *lp_wlGlobal = (struct wl_global*) 0xFFFFFFFF;
    SET_RETURN_SEQ(wl_global_create, &lp_wlGlobal, 1);

    ASSERT_EQ(input_controller_module_init(&m_iviShell), 0);

    ASSERT_EQ(wl_global_create_fake.call_count, 1);

    struct input_context *lp_ctxInput = (struct input_context *)((uintptr_t)wl_list_init_fake.arg0_history[4] - offsetof(struct input_context, resource_list));
    EXPECT_EQ(lp_ctxInput->ivishell, &m_iviShell);
    EXPECT_NE(lp_ctxInput->surface_created.notify, nullptr);
    EXPECT_NE(lp_ctxInput->surface_destroyed.notify, nullptr);
    EXPECT_NE(lp_ctxInput->seat_create_listener.notify, nullptr);
    EXPECT_EQ(lp_ctxInput->successful_init_stage, 1);

    free(lp_ctxInput->seat_default_name);
    free(lp_ctxInput);
}

/** ================================================================================================
 * @test_id             keyboard_grab_key_success
 * @brief               Test case of keyboard_grab_key() where surface_get_weston_surface() success, return an object
 *                      and valid input params
 * @test_procedure Steps:
 *                      -# Set seat focus to enable {1}
 *                      -# Set input timespec to TIME_UTC
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the keyboard_grab_key()
 *                      -# Verification point:
 *                         +# wl_display_next_serial() must be called once time
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, keyboard_grab_key_success)
{
    mpp_seatFocus[0]->focus = 1;

    struct timespec l_time;
    timespec_get(&l_time, TIME_UTC);

    mpp_ctxSeat[0]->keyboard_grab.keyboard = (struct weston_keyboard *)malloc(sizeof(struct weston_keyboard));
    mpp_ctxSeat[0]->keyboard_grab.keyboard->seat = (struct weston_seat *)malloc(sizeof(struct weston_seat));
    mpp_ctxSeat[0]->keyboard_grab.keyboard->seat->compositor = (struct weston_compositor *)malloc(sizeof(struct weston_compositor));
    mpp_ctxSeat[0]->keyboard_grab.keyboard->seat->compositor->wl_display = (struct wl_display *)0xFFFFFFFF;
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list, &mp_wlResource[0].link);
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list, &mp_wlResource[1].link);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    l_surf[0]->resource = (struct wl_resource *)malloc(sizeof(struct wl_resource));
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    keyboard_grab_key(&mpp_ctxSeat[0]->keyboard_grab, &l_time, 1, 1);

    ASSERT_EQ(wl_display_next_serial_fake.call_count, 1);
    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);

    free(l_surf[0]->resource);
    free(l_surf[0]);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard->seat->compositor);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard->seat);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard);
}

/** ================================================================================================
 * @test_id             keyboard_grab_modifiers_success
 * @brief               Test case of keyboard_grab_modifiers() where surface_get_weston_surface() success, return an object
 *                      and valid input params
 * @test_procedure Steps:
 *                      -# Set seat focus to enable {1}
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the keyboard_grab_modifiers()
 *                      -# Verification point:
 *                         +# wl_resource_get_client() must be called 3 times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, keyboard_grab_modifiers_success)
{
    mpp_seatFocus[0]->focus = 1;

    mpp_ctxSeat[0]->keyboard_grab.keyboard = (struct weston_keyboard *)malloc(sizeof(struct weston_keyboard));
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list, &mp_wlResource[0].link);
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list, &mp_wlResource[1].link);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    l_surf[0]->resource = (struct wl_resource *)malloc(sizeof(struct wl_resource));
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    keyboard_grab_modifiers(&mpp_ctxSeat[0]->keyboard_grab, 1, 1, 1, 1, 1);

    ASSERT_EQ(wl_resource_get_client_fake.call_count, 3);

    free(l_surf[0]->resource);
    free(l_surf[0]);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard);
}

/** ================================================================================================
 * @test_id             keyboard_grab_cancel_success
 * @brief               Test case of keyboard_grab_cancel() where surface_get_weston_surface() success, return an object
 *                      and valid input params
 * @test_procedure Steps:
 *                      -# Set seat focus to enable {1}
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Set input ctxSeat
 *                      -# Calling the keyboard_grab_cancel()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called 2 times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, keyboard_grab_cancel_success)
{
    mpp_seatFocus[0]->focus = 1;

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    l_surf[0]->resource = (struct wl_resource *)malloc(sizeof(struct wl_resource));
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    struct wl_resource l_wlResource;
    mpp_ctxSeat[0]->keyboard_grab.keyboard = (struct weston_keyboard *)malloc(sizeof(struct weston_keyboard));
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list, &l_wlResource.link);
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list);

    keyboard_grab_cancel(&mpp_ctxSeat[0]->keyboard_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 2);

    free(l_surf[0]->resource);
    free(l_surf[0]);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_wrongButtonCount
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count is enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_wrongButtonCount)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 1;

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 0);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_nullForcedPtr
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count is disable {0}
 *                      and forced_ptr_focus_surf is null pointer
 * @test_procedure Steps:
 *                      -# Set input ctxSeat (Do not set forced_ptr_focus_surf)
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_nullForcedPtr)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->seat = (struct weston_seat *)malloc(sizeof(struct weston_seat));
    mpp_ctxSeat[0]->pointer_grab.pointer->seat->compositor = (struct weston_compositor *)0xFFFFFFFF;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 0);

    free(mpp_ctxSeat[0]->pointer_grab.pointer->seat);
    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_surfNotEnabledAndNullFocus
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count, forced_surf_enabled is disable {0}
 *                      and forced_ptr_focus_surf is an object and pointer focus is null pointer
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_clear_focus() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_surfNotEnabledAndNullFocus)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_surf_enabled = ILM_FALSE;
    struct ivisurface l_ivi_surf;
    l_ivi_surf.layout_surface = (struct ivi_layout_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_ptr_focus_surf = &l_ivi_surf;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = {nullptr};

    struct weston_surface *l_surf[1] = {(struct weston_surface *)0xFFFFFFFF};
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);
    ASSERT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_surfNotEnabled
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count, forced_surf_enabled is disable {0}
 *                      and forced_ptr_focus_surf, pointer focus is an object
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_clear_focus() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_surfNotEnabled)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_surf_enabled = ILM_FALSE;
    struct ivisurface l_ivi_surf;
    l_ivi_surf.layout_surface = (struct ivi_layout_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_ptr_focus_surf = &l_ivi_surf;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    struct weston_surface *l_surf[1] = {(struct weston_surface *)0xFFFFFFFF};
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);
    ASSERT_EQ(weston_pointer_clear_focus_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_nullSurfCtx
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count is disable {0}
 *                      and forced_surf_enabled is enable {1} and forced_ptr_focus_surf, pointer focus is an object
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_set_focus() must be called once time
 *                         +# weston_pointer_clear_focus() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_nullSurfCtx)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_surf_enabled = ILM_TRUE;
    struct ivisurface l_ivi_surf;
    l_ivi_surf.layout_surface = (struct ivi_layout_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_ptr_focus_surf = &l_ivi_surf;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    custom_wl_list_init(&l_surf[0]->views);
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);
    ASSERT_EQ(weston_pointer_set_focus_fake.call_count, 1);
    ASSERT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(l_surf[0]);
    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_success
 * @brief               Test case of pointer_grab_focus() where input ctxSeat button_count is disable {0}
 *                      and forced_surf_enabled is enable {1} and forced_ptr_focus_surf, pointer focus is an object
 *                      and get_surface(), weston_surface_get_main_surface() are mocked to return an object
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_set_focus() must be called once time
 *                         +# weston_pointer_clear_focus() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_focus_success)
{
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_surf_enabled = ILM_TRUE;
    struct ivisurface l_ivi_surf;
    l_ivi_surf.layout_surface = (struct ivi_layout_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->forced_ptr_focus_surf = &l_ivi_surf;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    struct ivi_layout_surface *l_layout_surf[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface, l_layout_surf, 1);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    custom_wl_list_init(&l_surf[0]->views);
    SET_RETURN_SEQ(surface_get_weston_surface, l_surf, 1);
    SET_RETURN_SEQ(weston_surface_get_main_surface, l_surf, 1);

    pointer_grab_focus(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);
    // maybe logic is changed, need to check
    // ASSERT_EQ(weston_pointer_set_focus_fake.call_count, 1);
    ASSERT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(l_surf[0]);
    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_motion_success
 * @brief               Test case of pointer_grab_motion() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_motion() with valid input params
 *                      -# Verification point:
 *                         +# weston_pointer_send_motion() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_motion_success)
{
    pointer_grab_motion(&mpp_ctxSeat[0]->pointer_grab, nullptr, nullptr);
    ASSERT_EQ(weston_pointer_send_motion_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             pointer_grab_button_error
 * @brief               Test case of pointer_grab_button() where input ctxSeat button_count and state are enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is enable {1}
 *                      -# Calling the pointer_grab_button() time 1 with state is 0
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_button() time 2 with state is 1
 *                      -# Verification point:
 *                         +# focus() not be called
 *                         +# weston_pointer_send_button() must be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_button_error)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 1;

    pointer_grab_button(&mpp_ctxSeat[0]->pointer_grab, nullptr, 0, 0);

    ASSERT_EQ(weston_pointer_send_button_fake.call_count, 1);
    ASSERT_EQ(focus_fake.call_count, 0);

    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;

    pointer_grab_button(&mpp_ctxSeat[0]->pointer_grab, nullptr, 0, 1);

    ASSERT_EQ(weston_pointer_send_button_fake.call_count, 2);
    ASSERT_EQ(focus_fake.call_count, 0);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_button_success
 * @brief               Test case of pointer_grab_button() where input ctxSeat button_count and state are disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_button() with state is 0
 *                      -# Verification point:
 *                         +# weston_pointer_send_button() must be called once time
 *                         +# focus() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_button_success)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;

    pointer_grab_button(&mpp_ctxSeat[0]->pointer_grab, nullptr, 0, 0);

    ASSERT_EQ(weston_pointer_send_button_fake.call_count, 1);
    ASSERT_EQ(focus_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_axis_success
 * @brief               Test case of pointer_grab_axis() where input ctxSeat button_count is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_axis()
 *                      -# Verification point:
 *                         +# weston_pointer_send_axis() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_axis_success)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;

    pointer_grab_axis(&mpp_ctxSeat[0]->pointer_grab, nullptr, nullptr);

    ASSERT_EQ(weston_pointer_send_axis_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_axis_source_success
 * @brief               Test case of pointer_grab_axis_source() where input ctxSeat button_count is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_axis_source()
 *                      -# Verification point:
 *                         +# weston_pointer_send_axis_source() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_axis_source_success)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;

    pointer_grab_axis_source(&mpp_ctxSeat[0]->pointer_grab, 0);

    ASSERT_EQ(weston_pointer_send_axis_source_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_frame_success
 * @brief               Test case of pointer_grab_frame() where input ctxSeat button_count is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_frame()
 *                      -# Verification point:
 *                         +# weston_pointer_send_frame() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_frame_success)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;

    pointer_grab_frame(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(weston_pointer_send_frame_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_cancel_success
 * @brief               Test case of pointer_grab_cancel() where input ctxSeat button_count is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is disable {0}
 *                      -# Calling the pointer_grab_cancel()
 *                      -# Verification point:
 *                         +# weston_pointer_clear_focus() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, pointer_grab_cancel_success)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->button_count = 0;
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    pointer_grab_cancel(&mpp_ctxSeat[0]->pointer_grab);

    ASSERT_EQ(weston_pointer_clear_focus_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             touch_grab_down_nullFocus
 * @brief               Test case of touch_grab_down() where input ctxSeat focus is null pointer
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with focus is null pointer
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_down_nullFocus)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = nullptr;

    touch_grab_down(&mpp_ctxSeat[0]->touch_grab, nullptr, 0, 0, 0);

    ASSERT_EQ(weston_surface_get_main_surface_fake.call_count, 0);

    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_down_nullSurfCtx
 * @brief               Test case of touch_grab_down() where weston_surface_get_main_surface() fails, return null object
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() must be called once time
 *                         +# weston_touch_send_down() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_down_nullSurfCtx)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;

    touch_grab_down(&mpp_ctxSeat[0]->touch_grab, nullptr, 0, 0, 0);

    ASSERT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    ASSERT_EQ(weston_touch_send_down_fake.call_count, 1);

    free(mpp_ctxSeat[0]->touch_grab.touch->focus);
    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_down_wrongNumTp
 * @brief               Test case of touch_grab_down() where weston_surface_get_main_surface() success, return an object
 *                      and input ctxSeat num_tp is disable {0}
 * @test_procedure Steps:
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() must be called once time
 *                         +# weston_touch_send_down() must be called once time
 *                         +# get_id_of_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_down_wrongNumTp)
{
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    struct ivi_layout_surface *l_layout_surf[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface, l_layout_surf, 1);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    SET_RETURN_SEQ(weston_surface_get_main_surface, l_surf, 1);

    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->touch_grab.touch->num_tp = 0;

    touch_grab_down(&mpp_ctxSeat[0]->touch_grab, nullptr, 0, 0, 0);

    ASSERT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    ASSERT_EQ(weston_touch_send_down_fake.call_count, 1);
    ASSERT_EQ(get_id_of_surface_fake.call_count, 0);

    free(l_surf[0]);
    free(mpp_ctxSeat[0]->touch_grab.touch->focus);
    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_down_success
 * @brief               Test case of touch_grab_down() where weston_surface_get_main_surface() success, return an object
 *                      and input ctxSeat num_tp is enable {1}
 * @test_procedure Steps:
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() must be called once time
 *                         +# weston_touch_send_down() must be called once time
 *                         +# get_id_of_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_down_success)
{
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    struct ivi_layout_surface *l_layout_surf[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface, l_layout_surf, 1);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    SET_RETURN_SEQ(weston_surface_get_main_surface, l_surf, 1);

    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->touch_grab.touch->num_tp = 1;

    touch_grab_down(&mpp_ctxSeat[0]->touch_grab, nullptr, 0, 0, 0);

    ASSERT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    ASSERT_EQ(weston_touch_send_down_fake.call_count, 1);
    ASSERT_EQ(get_id_of_surface_fake.call_count, 1);

    free(l_surf[0]);
    free(mpp_ctxSeat[0]->touch_grab.touch->focus);
    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_nullFocus
 * @brief               Test case of touch_grab_up() where input ctxSeat focus is null pointer
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with focus is null pointer
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_up_nullFocus)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = nullptr;

    touch_grab_up(&mpp_ctxSeat[0]->touch_grab, nullptr, 0);

    ASSERT_EQ(weston_touch_send_up_fake.call_count, 0);

    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_wrongNumTp
 * @brief               Test case of touch_grab_up() where input ctxSeat num_tp is enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() must be called once time
 *                         +# get_id_of_surface() not be called
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_up_wrongNumTp)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)0xFFFFFFFF;
    mpp_ctxSeat[0]->touch_grab.touch->num_tp = 1;

    touch_grab_up(&mpp_ctxSeat[0]->touch_grab, nullptr, 0);

    ASSERT_EQ(weston_touch_send_up_fake.call_count, 1);
    ASSERT_EQ(get_id_of_surface_fake.call_count, 0);

    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_success
 * @brief               Test case of touch_grab_up() where weston_surface_get_main_surface() success, return an object
 *                      and input ctxSeat num_tp is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() must be called once time
 *                         +# get_id_of_surface() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_up_success)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;
    mpp_ctxSeat[0]->touch_grab.touch->num_tp = 0;

    struct ivi_layout_surface *l_layout_surf[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface, l_layout_surf, 1);

    struct weston_surface *l_surf[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    SET_RETURN_SEQ(weston_surface_get_main_surface, l_surf, 1);

    touch_grab_up(&mpp_ctxSeat[0]->touch_grab, nullptr, 0);

    ASSERT_EQ(weston_touch_send_up_fake.call_count, 1);
    ASSERT_EQ(get_id_of_surface_fake.call_count, 1);

    free(l_surf[0]);
    free(mpp_ctxSeat[0]->touch_grab.touch->focus);
    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_motion_success
 * @brief               Test case of touch_grab_motion() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the touch_grab_motion()
 *                      -# Verification point:
 *                         +# weston_touch_send_motion() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_motion_success)
{
    touch_grab_motion(&mpp_ctxSeat[0]->touch_grab, nullptr, 0, 0, 0);
    ASSERT_EQ(weston_touch_send_motion_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             touch_grab_frame_success
 * @brief               Test case of touch_grab_frame() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the touch_grab_frame()
 *                      -# Verification point:
 *                         +# weston_touch_send_frame() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_frame_success)
{
    touch_grab_frame(&mpp_ctxSeat[0]->touch_grab);
    ASSERT_EQ(weston_touch_send_frame_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             touch_grab_cancel_success
 * @brief               Test case of touch_grab_cancel() where valid input params
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_cancel()
 *                      -# Verification point:
 *                         +# weston_touch_set_focus() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, touch_grab_cancel_success)
{
    mpp_ctxSeat[0]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[0]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[0]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;
    custom_wl_list_init(&mpp_ctxSeat[0]->touch_grab.touch->focus_resource_list);

    touch_grab_cancel(&mpp_ctxSeat[0]->touch_grab);

    ASSERT_EQ(weston_touch_set_focus_fake.call_count, 1);

    free(mpp_ctxSeat[0]->touch_grab.touch->focus);
    free(mpp_ctxSeat[0]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_nullSurface
 * @brief               Test case of setup_input_focus() where get_surface_from_id() fails, return null object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the setup_input_focus()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_nullSurface)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    setup_input_focus(mp_ctxInput, 10, 1, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_wrongDevice
 * @brief               Test case of setup_input_focus() where get_surface_from_id() success, return an object
 *                      and input device not focus anything
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_focus() with input device is 0
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_wrongDevice)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    setup_input_focus(mp_ctxInput, 10, 0, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toKeyBoardWithNullKeyBoard
 * @brief               Test case of setup_input_focus() where weston_seat_get_keyboard() fails, return null pointer
 *                      and input device focus to keyboard
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_focus() with input device is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_toKeyBoardWithNullKeyBoard)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    setup_input_focus(mp_ctxInput, 10, 1, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_keyboard_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toKeyBoardWithOneKeyBoardAndNotEnabled
 * @brief               Test case of setup_input_focus() where weston_seat_get_keyboard() success, return an pointer
 *                      and input device focus to keyboard and input enabled is disable {0}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 0
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_toKeyBoardWithOneKeyBoardAndNotEnabled)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    struct weston_keyboard *l_keyboard[1] = {(struct weston_keyboard *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_keyboard, l_keyboard, 1);

    setup_input_focus(mp_ctxInput, 10, 1, 0);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_keyboard_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toKeyBoardWithOneKeyBoardAndEnabled
 * @brief               Test case of setup_input_focus() where weston_seat_get_keyboard() success, return an pointer
 *                      and input device focus to keyboard and input enabled is enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Mocking the wl_resource_get_version() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, input_set_input_focus_toKeyBoardWithOneKeyBoardAndEnabled)
{
    struct wl_resource l_wlResource;
    mpp_ctxSeat[0]->keyboard_grab.keyboard = (struct weston_keyboard *)malloc(sizeof(struct weston_keyboard));
    mpp_ctxSeat[0]->keyboard_grab.keyboard->seat = (struct weston_seat *)malloc(sizeof(struct weston_seat));
    mp_westonSeat[0].compositor = (struct weston_compositor *)malloc(sizeof(struct weston_compositor));
    mp_westonSeat[0].compositor->kb_repeat_rate = 0;
    mp_westonSeat[0].compositor->kb_repeat_delay = 0;
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list);
    custom_wl_list_insert(&mpp_ctxSeat[0]->keyboard_grab.keyboard->focus_resource_list, &l_wlResource.link);
    custom_wl_list_init(&mpp_ctxSeat[0]->keyboard_grab.keyboard->resource_list);

    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    struct weston_keyboard *l_keyboard[1] = {(struct weston_keyboard *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_keyboard, l_keyboard, 1);

    struct weston_surface *l_weston_surfaces[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    SET_RETURN_SEQ(surface_get_weston_surface, l_weston_surfaces, 1);

    int l_num[1] = {WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION};
    SET_RETURN_SEQ(wl_resource_get_version, l_num, 1);

    setup_input_focus(mp_ctxInput, 10, 1, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_keyboard_fake.call_count, 1);

    free(l_weston_surfaces[0]);
    free(mp_westonSeat[0].compositor);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard->seat);
    free(mpp_ctxSeat[0]->keyboard_grab.keyboard);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toPointerWithNullPointer
 * @brief               Test case of setup_input_focus() where weston_seat_get_pointer() fails, return null pointer
 *                      and input device focus to pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_toPointerWithNullPointer)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    setup_input_focus(mp_ctxInput, 10, 2, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toPointerWithOnePointerAndNotEnabled
 * @brief               Test case of setup_input_focus() where weston_seat_get_pointer() success, return an pointer
 *                      and input device focus to pointer and input enabled is disable {0}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 0
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_toPointerWithOnePointerAndNotEnabled)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    struct weston_pointer *l_ptr[1] = {(struct weston_pointer *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_pointer, l_ptr, 1);

    setup_input_focus(mp_ctxInput, 10, 2, 0);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toPointerWithOnePointerAndEnabled
 * @brief               Test case of setup_input_focus() where weston_seat_get_pointer() success, return an pointer
 *                      and input device focus to pointer and input enabled is enable {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_focus_toPointerWithOnePointerAndEnabled)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    struct weston_pointer *l_ptr[1] = {(struct weston_pointer *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_pointer, l_ptr, 1);

    setup_input_focus(mp_ctxInput, 10, 2, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_toTouch
 * @brief               Test case of setup_input_focus() where input device focus to touch
 *                      and input enabled is enable {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_focus() with input enabled is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called 2 times
 */
TEST_F(InputControllerTest, input_set_input_focus_toTouch)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    setup_input_focus(mp_ctxInput, 10, 4, 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_wrongSeatName
 * @brief               Test case of setup_input_acceptance() where input seat name is null
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the setup_input_acceptance()
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_acceptance_wrongSeatName)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    setup_input_acceptance(mp_ctxInput, 10, "", 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_nullSurface
 * @brief               Test case of setup_input_acceptance() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the setup_input_acceptance() with valid seat name
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# get_surface_from_id() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_acceptance_nullSurface)
{
    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    setup_input_acceptance(mp_ctxInput, 10, "default", 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 0);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_withNullPointerAndApccepted
 * @brief               Test case of setup_input_acceptance() where ctxSeat pointer is null pointer
 *                      and input accepted is enable {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_acceptance() with valid seat name
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_acceptance_withNullPointerAndApccepted)
{
    // error runtime: not allow pass nullptr
    // struct input_context *l_input[1] = {mp_ctxInput};
    // SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    // struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    // SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    // setup_input_acceptance(nullptr, 10, "default", 1);

    // ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    // ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    // ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_withValidPointerAndUnapccepted
 * @brief               Test case of setup_input_acceptance() where valid ctxSeat pointer
 *                      and input accepted is disable {0}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Mocking the weston_seat_get_touch() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_acceptance() with input accepted is 0
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# Free resources are allocated when running the test
 *                         +# Allocate memory for resources are freed when running the test
 */
TEST_F(InputControllerTest, input_set_input_acceptance_withValidPointerAndUnapccepted)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));

    struct weston_keyboard *lpp_keyboard [] = {(struct weston_keyboard *)0xFFFFFFFF};
    struct weston_pointer *lpp_pointer [] = {(struct weston_pointer *)0xFFFFFFFF};
    struct weston_touch *lpp_touch [] = {(struct weston_touch *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_keyboard, lpp_keyboard, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, lpp_pointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, lpp_touch, 1);

    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    setup_input_acceptance(mp_ctxInput, 10, "default", 0);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
    ASSERT_EQ(surface_get_weston_surface_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);

    mpp_seatFocus[0] = (struct seat_focus*)calloc(1, sizeof(struct seat_focus));
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_withValidPointerAndApccepted
 * @brief               Test case of setup_input_acceptance() where valid ctxSeat pointer
 *                      and input accepted is enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Calling the setup_input_acceptance() with input accepted is 1
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 *                         +# Free resources are allocated when running the test
 */
TEST_F(InputControllerTest, input_set_input_acceptance_withValidPointerAndApccepted)
{
    mpp_ctxSeat[0]->pointer_grab.pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    mpp_ctxSeat[0]->pointer_grab.pointer->focus = (struct weston_view *)malloc(sizeof(struct weston_view));

    struct input_context *l_input[1] = {mp_ctxInput};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)l_input, 1);

    struct ivi_layout_surface *l_surface[1] = {mp_layoutSurface};
    SET_RETURN_SEQ(get_surface_from_id, l_surface, 1);

    struct weston_pointer *l_ptr[1] = {(struct weston_pointer *)0xFFFFFFFF};
    SET_RETURN_SEQ(weston_seat_get_pointer, l_ptr, 1);

    setup_input_acceptance(mp_ctxInput, 10, "default", 1);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);

    free(mpp_ctxSeat[0]->pointer_grab.pointer->focus);
    free(mpp_ctxSeat[0]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             handle_surface_destroy_nullSurface
 * @brief               Test case of handle_surface_destroy() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the handle_surface_destroy()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(InputControllerTest, handle_surface_destroy_nullSurface)
{
    handle_surface_destroy(&mp_ctxInput->surface_destroyed, nullptr);
    ASSERT_EQ(wl_list_remove_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             handle_surface_destroy_success
 * @brief               Test case of handle_surface_destroy() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the handle_surface_destroy()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 *                         +# Allocate memory for resources are freed when running the test
 */
TEST_F(InputControllerTest, handle_surface_destroy_success)
{
    handle_surface_destroy(&mp_ctxInput->surface_destroyed, &mp_iviSurface[0]);

    ASSERT_EQ(wl_list_remove_fake.call_count, 1);

    mpp_seatFocus[0] = (struct seat_focus*)calloc(1, sizeof(struct seat_focus));
}

/** ================================================================================================
 * @test_id             handle_surface_create_nullSeatCtx
 * @brief               Test case of handle_surface_create() where invalid input params
 * @test_procedure Steps:
 *                      -# Create input context and set seat_list for input context
 *                      -# Calling the handle_surface_create() with input surface is null pointer
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(InputControllerTest, handle_surface_create_nullSeatCtx)
{
    struct input_context l_input_ctx;
    custom_wl_list_init(&l_input_ctx.seat_list);
    l_input_ctx.ivishell = &m_iviShell;
    l_input_ctx.seat_default_name = (char*)mp_seatName[0];

    handle_surface_create(&l_input_ctx.surface_created, &mp_iviSurface[0]);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             handle_surface_create_success
 * @brief               Test case of handle_surface_create() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the handle_surface_create() with valid input context and input surface
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called 2 times
 */
TEST_F(InputControllerTest, handle_surface_create_success)
{
    mp_ctxInput->seat_default_name = (char*)mp_seatName[0];
    handle_surface_create(&mp_ctxInput->surface_created, &mp_iviSurface[0]);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             unbind_resource_controller_success
 * @brief               Test case of unbind_resource_controller() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the unbind_resource_controller()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 */
TEST_F(InputControllerTest, unbind_resource_controller_success)
{
    enable_utility_funcs_of_array_list();
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    unbind_resource_controller(&(mp_wlResource[0]));
    ASSERT_EQ(wl_list_remove_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_controller_destroy_nullListener
 * @brief               Test case of input_controller_destroy() where input context is null pointer
 * @test_procedure Steps:
 *                      -# Calling the input_controller_destroy()
 */
TEST_F(InputControllerTest, input_controller_destroy_nullListener)
{
    input_controller_destroy(nullptr, nullptr);
}

/** ================================================================================================
 * @test_id             input_controller_destroy_success
 * @brief               Test case of input_controller_destroy() where valid input params
 * @test_procedure Steps:
 *                      -# Set ctx input successful_init_stage is -1
 *                      -# Calling the input_controller_destroy()
 */
TEST_F(InputControllerTest, input_controller_destroy_success)
{
    mp_ctxInput->successful_init_stage = -1;
    input_controller_destroy(&mp_ctxInput->shell_destroy_listener, nullptr);
}
