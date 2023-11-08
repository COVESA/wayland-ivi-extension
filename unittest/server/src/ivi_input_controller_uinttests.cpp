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
        wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
        wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;

        // Prepare sample for ivi shell
        m_iviShell.interface = &g_iviLayoutInterfaceFake;
        m_iviShell.compositor = &m_westonCompositor;
        custom_wl_list_init(&m_iviShell.list_surface);
        custom_wl_list_init(&m_westonCompositor.seat_list);
        custom_wl_list_init(&m_iviShell.ivisurface_created_signal.listener_list);
        custom_wl_list_init(&m_iviShell.ivisurface_removed_signal.listener_list);
        custom_wl_list_init(&m_iviShell.compositor->destroy_signal.listener_list);
        custom_wl_list_init(&m_iviShell.compositor->seat_created_signal.listener_list);

        /* initialize the input_context */
        mp_ctxInput = (struct input_context*)calloc(1, sizeof(struct input_context));
        mp_ctxInput->ivishell = &m_iviShell;
        custom_wl_list_init(&mp_ctxInput->seat_list);
        custom_wl_list_init(&mp_ctxInput->resource_list);
        mp_ctxInput->successful_init_stage = 0;
        mp_ctxInput->seat_default_name = (char*)mp_seatName[DEFAULT_SEAT];
        custom_wl_list_init(&mp_ctxInput->surface_created.link);
        custom_wl_list_init(&mp_ctxInput->surface_destroyed.link);
        custom_wl_list_init(&mp_ctxInput->shell_destroy_listener.link);
        custom_wl_list_init(&mp_ctxInput->seat_create_listener.link);

        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            /* setup the list of weston seats:
            * mpp_westonSeat[DEFAULT_SEAT]: {seat_name: "default", ...}
            * mpp_westonSeat[CUSTOM_SEAT]: {seat_name: "weston_seat", ...} */
            mpp_westonSeat[i].seat_name = (char*)mp_seatName[i];
            mpp_westonSeat[i].compositor = &m_westonCompositor;
            custom_wl_list_init(&mpp_westonSeat[i].destroy_signal.listener_list);
            custom_wl_list_init(&mpp_westonSeat[i].updated_caps_signal.listener_list);
            /* setup the list of seat_ctx:
            * mpp_ctxSeat[DEFAULT_SEAT]: {input_ctx: mp_ctxInput, west_seat: mpp_westonSeat[DEFAULT_SEAT]...}
            * mpp_ctxSeat[CUSTOM_SEAT]: {input_ctx: mp_ctxInput, west_seat: mpp_westonSeat[CUSTOM_SEAT] ...} */
            mpp_ctxSeat[i] = (struct seat_ctx*)calloc(1, sizeof(struct seat_ctx));
            mpp_ctxSeat[i]->input_ctx = mp_ctxInput;
            mpp_ctxSeat[i]->west_seat = &mpp_westonSeat[i];
            mpp_ctxSeat[i]->pointer_grab.interface = nullptr;
            mpp_ctxSeat[i]->pointer_grab.pointer = nullptr;
            mpp_ctxSeat[i]->keyboard_grab.interface = nullptr;
            mpp_ctxSeat[i]->keyboard_grab.keyboard = nullptr;
            mpp_ctxSeat[i]->touch_grab.interface = nullptr;
            mpp_ctxSeat[i]->touch_grab.touch = nullptr;
            mpp_ctxSeat[i]->forced_surf_enabled = ILM_FALSE;
            mpp_ctxSeat[i]->forced_ptr_focus_surf = nullptr;
            custom_wl_list_init(&mpp_ctxSeat[i]->seat_node);
            custom_wl_list_init(&mpp_ctxSeat[i]->updated_caps_listener.link);
            custom_wl_list_init(&mpp_ctxSeat[i]->destroy_listener.link);
            custom_wl_list_insert(&mp_ctxInput->seat_list, &mpp_ctxSeat[i]->seat_node);
            /* setup the list of input resources:
            * mp_wlResource[DEFAULT_SEAT]: {object.id: 0, ...}
            * mp_wlResource[CUSTOM_SEAT]: {object.id: 1, ...} */
            mp_wlResource[i].object.id = 0;
            custom_wl_list_insert(&mp_ctxInput->resource_list, &mp_wlResource[i].link);
            /* setup the list of ivisurfaces and ivi_layout_surfaces:
            * mp_layoutSurface[DEFAULT_SEAT]: {id_surface: 11, ...}
            * mp_layoutSurface[CUSTOM_SEAT]: {id_surface: 12, ...}
            * mpp_iviSurface[DEFAULT_SEAT]: {shell: m_iviShell, layout_surface: mp_layoutSurface[DEFAULT_SEAT], accepted_seat_list: mpp_ctxSeat[DEFAULT_SEAT]...}
            * mpp_iviSurface[CUSTOM_SEAT]: {shell: m_iviShell, layout_surface: mp_layoutSurface[CUSTOM_SEAT], accepted_seat_list: mpp_ctxSeat[CUSTOM_SEAT]...}*/
            mp_layoutSurface[i].id_surface = i + 10;
            mpp_iviSurface[i].shell = &m_iviShell;
            mpp_iviSurface[i].layout_surface = &mp_layoutSurface[i];
            custom_wl_list_init(&mpp_iviSurface[i].accepted_seat_list);
            custom_wl_list_insert(&m_iviShell.list_surface, &mpp_iviSurface[i].link);
            mpp_seatFocus[i] = (struct seat_focus*)calloc(1, sizeof(struct seat_focus));
            mpp_seatFocus[i]->seat_ctx =  mpp_ctxSeat[i];
            mpp_seatFocus[i]->focus = ILM_INPUT_DEVICE_ALL;
            custom_wl_list_insert(&mpp_iviSurface[i].accepted_seat_list, &mpp_seatFocus[i]->link);
        }
    }

    void deinit_input_controller_content()
    {
        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            if(mpp_ctxSeat[i] != nullptr)
                free(mpp_ctxSeat[i]);
            if(mpp_seatFocus[i] != nullptr)
                free(mpp_seatFocus[i]);
        }

        if(mp_ctxInput != nullptr)
            free(mp_ctxInput);
    }

    struct input_context *mp_ctxInput = nullptr;
    struct wl_resource mp_wlResource[MAX_NUMBER] = {};

    struct seat_ctx *mpp_ctxSeat[MAX_NUMBER] = {nullptr};
    struct weston_seat mpp_westonSeat[MAX_NUMBER];
    const char* mp_seatName[MAX_NUMBER] = {"default", "weston_seat"};

    struct seat_focus *mpp_seatFocus[MAX_NUMBER] = {nullptr};
    struct ivisurface mpp_iviSurface[MAX_NUMBER];
    struct ivi_layout_surface mp_layoutSurface[MAX_NUMBER];

    struct weston_compositor m_westonCompositor = {};
    struct ivishell m_iviShell = {};
    
    void *mp_fakePointer = (void*)0xFFFFFFFF;
    void *mp_nullPointer = nullptr;
    struct ivi_layout_surface *mpp_layoutSurface[CUSTOM_SEAT] = {mp_layoutSurface};
};

/** ================================================================================================
 * @test_id             handle_seat_create_customSeat
 * @brief               Test case of handle_seat_create() where has new custom seat on system
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_create() with weston custom seat
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 3 times
 *                         +# wl_resource_post_event() must be called 2 times
 *                         +# member of seat_ctx struct must be initialized
 */
TEST_F(InputControllerTest, handle_seat_create_customSeat)
{
    handle_seat_create(&mp_ctxInput->seat_create_listener, &mpp_westonSeat[CUSTOM_SEAT]);
    struct seat_ctx *lp_ctxSeat = (struct seat_ctx *)
            ((uintptr_t)wl_list_insert_fake.arg1_history[DEFAULT_SEAT] - offsetof(struct seat_ctx, seat_node));

    EXPECT_EQ(wl_list_insert_fake.call_count, 3);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);
    EXPECT_EQ(lp_ctxSeat->input_ctx, mp_ctxInput);
    EXPECT_EQ(lp_ctxSeat->west_seat, &mpp_westonSeat[CUSTOM_SEAT]);
    EXPECT_EQ(lp_ctxSeat->keyboard_grab.interface, &keyboard_grab_interface);
    EXPECT_EQ(lp_ctxSeat->pointer_grab.interface, &pointer_grab_interface);
    EXPECT_EQ(lp_ctxSeat->touch_grab.interface, &touch_grab_interface);

    free(lp_ctxSeat);
}

/** ================================================================================================
 * @test_id             handle_seat_create_defaultSeat
 * @brief               Test case of handle_seat_create() where detecting the default seat
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_create()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 5 times
 *                         +# wl_resource_post_event() must be called 6 times
 *                         +# member of seat_ctx struct must be initialized
 *                         +# member of seat_focus struct must be initialized
 */
TEST_F(InputControllerTest, handle_seat_create_defaultSeat)
{
    handle_seat_create(&mp_ctxInput->seat_create_listener, &mpp_westonSeat[DEFAULT_SEAT]);
    struct seat_ctx *lp_ctxSeat = (struct seat_ctx *)
            ((uintptr_t)wl_list_insert_fake.arg1_history[DEFAULT_SEAT] - offsetof(struct seat_ctx, seat_node));
    struct seat_focus *lp_seatFocus1 = (struct seat_focus *)
            ((uintptr_t)wl_list_insert_fake.arg1_history[3] - offsetof(struct seat_focus, link));
    struct seat_focus *lp_seatFocus2 = (struct seat_focus *)
            ((uintptr_t)wl_list_insert_fake.arg1_history[4] - offsetof(struct seat_focus, link));

    ASSERT_EQ(wl_list_insert_fake.call_count, 5);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 6);
    EXPECT_EQ(lp_ctxSeat->input_ctx, mp_ctxInput);
    EXPECT_EQ(lp_ctxSeat->west_seat, &mpp_westonSeat[DEFAULT_SEAT]);
    EXPECT_EQ(lp_ctxSeat->keyboard_grab.interface, &keyboard_grab_interface);
    EXPECT_EQ(lp_ctxSeat->pointer_grab.interface, &pointer_grab_interface);
    EXPECT_EQ(lp_ctxSeat->touch_grab.interface, &touch_grab_interface);
    EXPECT_EQ(lp_seatFocus1->seat_ctx, lp_ctxSeat);
    EXPECT_EQ(lp_seatFocus2->seat_ctx, lp_ctxSeat);
    EXPECT_EQ(wl_list_insert_fake.arg0_history[3], &mpp_iviSurface[CUSTOM_SEAT].accepted_seat_list);
    EXPECT_EQ(wl_list_insert_fake.arg0_history[4], &mpp_iviSurface[DEFAULT_SEAT].accepted_seat_list);

    free(lp_seatFocus1);
    free(lp_seatFocus2);
    free(lp_ctxSeat);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_noUpdate
 * @brief               Test case of handle_seat_updated_caps() where no change in capabilities
 * @test_procedure Steps:
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Mocking the weston_seat_get_touch() does return an object
 *                      -# Set input param keyboard_grab.keyboard to an object
 *                      -# Set input param pointer_grab.pointer to an object
 *                      -# Set input param touch_grab.touch to an object
 *                      -# Calling the handle_seat_updated_caps()
 *                      -# Verification point:
 *                         +# weston_keyboard_start_grab() not be called
 *                         +# weston_pointer_start_grab() not be called
 *                         +# weston_touch_start_grab() not be called
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 */
TEST_F(InputControllerTest, handle_seat_updated_caps_noUpdate)
{
    SET_RETURN_SEQ(weston_seat_get_keyboard, (struct weston_keyboard **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, (struct weston_touch **)&mp_fakePointer, 1);
    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = {(struct weston_keyboard *)mp_fakePointer};
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = {(struct weston_pointer *)mp_fakePointer};
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = {(struct weston_touch *)mp_fakePointer};

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(0, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_newUpdate
 * @brief               Test case of handle_seat_updated_caps() where has new capabilities
 * @test_procedure Steps:
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Mocking the weston_seat_get_touch() does return an object
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
TEST_F(InputControllerTest, handle_seat_updated_caps_newUpdate)
{
    SET_RETURN_SEQ(weston_seat_get_keyboard, (struct weston_keyboard **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, (struct weston_touch **)&mp_fakePointer, 1);
    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = nullptr;

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(1, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(1, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(1, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_updated_caps_noCapabitilies
 * @brief               Test case of handle_seat_updated_caps() where weston_seat_get_keyboard() and weston_seat_get_pointer()
 *                      and weston_seat_get_touch() are not mocked
 * @test_procedure Steps:
 *                      -# Mocking the weston_seat_get_keyboard() does return nullptr
 *                      -# Mocking the weston_seat_get_pointer() does return nullptr
 *                      -# Mocking the weston_seat_get_touch() does return nullptr
 *                      -# Set input param keyboard_grab.keyboard to nullptr
 *                      -# Set input param pointer_grab.pointer to nullptr
 *                      -# Set input param touch_grab.touch to nullptr
 *                      -# Calling the handle_seat_updated_caps() time 1
 *                      -# Set input param keyboard_grab.keyboard to an object
 *                      -# Set input param pointer_grab.pointer to an object
 *                      -# Set input param touch_grab.touch to an object
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
TEST_F(InputControllerTest, handle_seat_updated_caps_noCapabitilies)
{
    SET_RETURN_SEQ(weston_seat_get_keyboard, (struct weston_keyboard **)&mp_nullPointer, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_nullPointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, (struct weston_touch **)&mp_nullPointer, 1);
    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = nullptr;
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = nullptr;

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard = (struct weston_keyboard *)mp_fakePointer;
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = (struct weston_pointer *)mp_fakePointer;
    mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch = (struct weston_touch *)mp_fakePointer;

    handle_seat_updated_caps(&mpp_ctxSeat[CUSTOM_SEAT]->updated_caps_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(0, weston_keyboard_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_pointer_start_grab_fake.call_count);
    ASSERT_EQ(0, weston_touch_start_grab_fake.call_count);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->keyboard_grab.keyboard);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer);
    ASSERT_EQ(nullptr, mpp_ctxSeat[CUSTOM_SEAT]->touch_grab.touch);
    ASSERT_EQ(2*MAX_NUMBER, wl_resource_post_event_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_destroy_noSurfaceAcceptance
 * @brief               Test case of handle_seat_destroy() where no surface acceptance
 * @test_procedure Steps:
 *                      -# Prepare the data input, the seat_ctx object
 *                      -# Calling the handle_seat_destroy()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 *                         +# wl_list_remove() must be called 3 times
 */
TEST_F(InputControllerTest, handle_seat_destroy_noSurfaceAcceptance)
{
    struct seat_ctx* lp_ctxSeat = (struct seat_ctx*)calloc(1, sizeof(struct seat_ctx));
    lp_ctxSeat->input_ctx = mp_ctxInput;
    lp_ctxSeat->west_seat = &mpp_westonSeat[CUSTOM_SEAT];

    handle_seat_destroy(&lp_ctxSeat->destroy_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
    ASSERT_EQ(3, wl_list_remove_fake.call_count);
}

/** ================================================================================================
 * @test_id             handle_seat_destroy_hasSurfaceAcceptance
 * @brief               Test case of handle_seat_destroy() where has a surface acceptance
 * @test_procedure Steps:
 *                      -# Calling the handle_seat_destroy()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called {MAX_NUMBER} times
 *                         +# wl_list_remove() must be called 4 times
 */
TEST_F(InputControllerTest, handle_seat_destroy_hasSurfaceAcceptance)
{
    handle_seat_destroy(&mpp_ctxSeat[CUSTOM_SEAT]->destroy_listener, &mpp_westonSeat[CUSTOM_SEAT]);

    ASSERT_EQ(MAX_NUMBER, wl_resource_post_event_fake.call_count);
    ASSERT_EQ(4, wl_list_remove_fake.call_count);

    mpp_ctxSeat[CUSTOM_SEAT] = nullptr;
    mpp_seatFocus[CUSTOM_SEAT] = nullptr;
}

/** ================================================================================================
 * @test_id             input_controller_module_init_badInput
 * @brief               Test case of input_controller_module_init() where the interface member of
 *                      ivishell points to nullptr
 * @test_procedure Steps:
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must not return 0
 *                         +#  wl_list_init_fake must not invoked
 */
TEST_F(InputControllerTest, input_controller_module_init_badInput)
{
    m_iviShell.interface = nullptr;
    ASSERT_NE(input_controller_module_init(&m_iviShell), 0);
    ASSERT_EQ(0, wl_list_init_fake.call_count);
}

/** ================================================================================================
 * @test_id             input_controller_module_init_cannotCreateGlobal
 * @brief               Test case of input_controller_module_init() where wl_global_create() return a nullptr
 * @test_procedure Steps:
 *                      -# Mocking the wl_global_create() returns a nullptr
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must not return 0
 *                         +# wl_global_create() must be called once time
 *                         +# wl_list_remove() must be called 6 times
 */
TEST_F(InputControllerTest, input_controller_module_init_cannotCreateGlobal)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    SET_RETURN_SEQ(wl_global_create, (struct wl_global**)&mp_nullPointer, 1);

    EXPECT_NE(input_controller_module_init(&m_iviShell), 0);

    EXPECT_EQ(wl_global_create_fake.call_count, 1);
    EXPECT_EQ(wl_list_remove_fake.call_count, 6);

    /*mpp_seatFocus will release, because they are a part of m_ivishell*/
    mpp_seatFocus[DEFAULT_SEAT] = nullptr;
    mpp_seatFocus[CUSTOM_SEAT] = nullptr;
}

/** ================================================================================================
 * @test_id             input_controller_module_init_canInitSuccess
 * @brief               Test case of input_controller_module_init() where wl_global_create() returns an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_global_create() returns an object
 *                      -# Calling the input_controller_module_init()
 *                      -# Verification point:
 *                         +# input_controller_module_init() must return 0
 *                         +# wl_global_create() must be called once time
 *                         +# Input controller module member must same the prepared data
 */
TEST_F(InputControllerTest, input_controller_module_init_canInitSuccess)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    SET_RETURN_SEQ(wl_global_create, (struct wl_global**)&mp_fakePointer, 1);

    ASSERT_EQ(input_controller_module_init(&m_iviShell), 0);

    ASSERT_EQ(wl_global_create_fake.call_count, 1);
    ASSERT_EQ(wl_list_init_fake.call_count, 2);
    struct input_context *lp_ctxInput = (struct input_context *)
            ((uintptr_t)wl_list_init_fake.arg0_history[DEFAULT_SEAT] - offsetof(struct input_context, resource_list));
    EXPECT_EQ(lp_ctxInput->ivishell, &m_iviShell);
    EXPECT_NE(lp_ctxInput->surface_created.notify, nullptr);
    EXPECT_NE(lp_ctxInput->surface_destroyed.notify, nullptr);
    EXPECT_NE(lp_ctxInput->seat_create_listener.notify, nullptr);
    EXPECT_EQ(lp_ctxInput->successful_init_stage, 1);

    free(lp_ctxInput->seat_default_name);
    free(lp_ctxInput);
}

/** ================================================================================================
 * @test_id             keyboard_grab_key_aSurfaceAvailable
 * @brief               Test case of keyboard_grab_key() where surface listen keyboard events
 * @test_procedure Steps:
 *                      -# Mocking the surface_get_weston_surface() returns an object
 *                      -# Calling the keyboard_grab_key()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called two times
 */
TEST_F(InputControllerTest, keyboard_grab_key_aSurfaceAvailable)
{
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct timespec l_time;
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_seatFocus[DEFAULT_SEAT]->focus = ILM_INPUT_DEVICE_KEYBOARD;
    lp_keyboard->seat = &mpp_westonSeat[DEFAULT_SEAT];
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);
    custom_wl_list_insert(&lp_keyboard->focus_resource_list, &mp_wlResource[DEFAULT_SEAT].link);
    custom_wl_list_insert(&lp_keyboard->resource_list, &mp_wlResource[CUSTOM_SEAT].link);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);

    keyboard_grab_key(&mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab, &l_time, 1, 1);

    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);

    free(lp_westSurf);
    free(lp_keyboard);
}

/** ================================================================================================
 * @test_id             keyboard_grab_modifiers_aSurfaceAvaiable
 * @brief               Test case of keyboard_grab_modifiers() where has a surface listening
 * @test_procedure Steps:
 *                      -# Mocking the surface_get_weston_surface() returns an object
 *                      -# Calling the keyboard_grab_modifiers()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called two times
 */
TEST_F(InputControllerTest, keyboard_grab_modifiers_aSurfaceAvaiable)
{
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct timespec l_time;
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_seatFocus[DEFAULT_SEAT]->focus = ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_POINTER;
    lp_keyboard->seat = &mpp_westonSeat[DEFAULT_SEAT];
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);
    custom_wl_list_insert(&lp_keyboard->focus_resource_list, &mp_wlResource[DEFAULT_SEAT].link);
    custom_wl_list_insert(&lp_keyboard->resource_list, &mp_wlResource[CUSTOM_SEAT].link);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);

    keyboard_grab_modifiers(&mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab, 1, 1, 1, 1, 1);

    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);

    free(lp_westSurf);
    free(lp_keyboard);
}

/** ================================================================================================
 * @test_id             keyboard_grab_cancel
 * @brief               Test case of keyboard_grab_cancel() where has a available surface
 * @test_procedure Steps:
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the keyboard_grab_cancel()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called 2 times
 *                         +# wl_resource_post_event() must be called 4 times
 */
TEST_F(InputControllerTest, keyboard_grab_cancel)
{
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct timespec l_time;
    struct wl_resource l_wlResource1, l_wlResource2;
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_seatFocus[DEFAULT_SEAT]->focus = ILM_INPUT_DEVICE_KEYBOARD;
    lp_keyboard->seat = &mpp_westonSeat[DEFAULT_SEAT];
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);
    custom_wl_list_insert(&lp_keyboard->focus_resource_list, &l_wlResource1.link);
    custom_wl_list_insert(&lp_keyboard->resource_list, &l_wlResource2.link);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);

    keyboard_grab_cancel(&mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab);

    EXPECT_EQ(wl_resource_post_event_fake.call_count, 4);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 2);

    free(lp_westSurf);
    free(lp_keyboard);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_hasButtonCount
 * @brief               Test case of pointer_grab_focus() where button_count is bigger than 0.
 *                      This function should not do anything.
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() not be called
 */
TEST_F(InputControllerTest, pointer_grab_focus_hasButtonCount)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    lp_pointer->button_count = 1;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 0);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_noFocusSurface
 * @brief               Test case of pointer_grab_focus() where no focus surface.
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() should not call
 *                         +# weston_compositor_pick_view() should call once time
 */
TEST_F(InputControllerTest, pointer_grab_focus_noFocusSurface)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    lp_pointer->button_count = 0;
    lp_pointer->focus = nullptr;
    lp_pointer->seat = &mpp_westonSeat[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    SET_RETURN_SEQ(weston_compositor_pick_view, (struct weston_view **)&mp_nullPointer, 1);

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 0);
    EXPECT_EQ(weston_compositor_pick_view_fake.call_count, 1);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_hasFocusSurfaceNoEnableForce
 * @brief               Test case of pointer_grab_focus() where has surface focus, but
 *                      seat don't enable to force to surface.
 * @test_procedure Steps:
 *                      -# Mocking the surface_get_weston_surface() return nullptr
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_clear_focus() not be called
 */
TEST_F(InputControllerTest, pointer_grab_focus_hasFocusSurfaceNoEnableForce)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    lp_pointer->button_count = 0;
    lp_pointer->focus = nullptr;
    lp_pointer->seat = &mpp_westonSeat[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_FALSE;
    SET_RETURN_SEQ(surface_get_weston_surface, (struct weston_surface **)&mp_nullPointer, 1);

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_hasFocusView
 * @brief               Test case of pointer_grab_focus() where has focus view
 * @test_procedure Steps:
 *                      -# Mocking the surface_get_weston_surface() returns an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_clear_focus() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_focus_hasFocusView)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    struct weston_view l_focusView;
    l_focusView.surface = (struct weston_surface*)mp_fakePointer;
    lp_pointer->button_count = 0;
    lp_pointer->focus = &l_focusView;
    lp_pointer->seat = &mpp_westonSeat[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_FALSE;
    SET_RETURN_SEQ(surface_get_weston_surface, (struct weston_surface **)&mp_fakePointer, 1);

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(weston_pointer_clear_focus_fake.call_count, 1);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_noWestonSurface
 * @brief               Test case of pointer_grab_focus() where has focus surface and
 *                      force surface is enable, but no weston_surface return from 
 *                      surface_get_weston_surface
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the surface_get_weston_surface() returns nullptr
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_set_focus() must not be called
 *                         +# weston_pointer_clear_focus() must not be called
 */
TEST_F(InputControllerTest, pointer_grab_focus_noWestonSurface)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    struct weston_view l_focusView;
    l_focusView.surface = (struct weston_surface*)mp_fakePointer;
    lp_pointer->button_count = 0;
    lp_pointer->focus = &l_focusView;
    lp_pointer->seat = &mpp_westonSeat[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_TRUE;
    SET_RETURN_SEQ(surface_get_weston_surface, (struct weston_surface **)&mp_nullPointer, 1);

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(weston_pointer_set_focus_fake.call_count, 0);
    EXPECT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_focus_hasWestonSurface
 * @brief               Test case of pointer_grab_focus() where has focus surface and
 *                      force surface is enable, but has weston_surface return from 
 *                      surface_get_weston_surface
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Calling the pointer_grab_focus()
 *                      -# Verification point:
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# weston_pointer_clear_focus() not be called
 */
TEST_F(InputControllerTest, pointer_grab_focus_hasWestonSurface)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    struct weston_surface *lp_surf = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    struct weston_view l_focusView;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    l_focusView.surface = (struct weston_surface*)mp_fakePointer;
    lp_pointer->button_count = 0;
    lp_pointer->focus = &l_focusView;
    lp_pointer->seat = &mpp_westonSeat[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_TRUE;
    custom_wl_list_init(&lp_surf->views);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_surf, 1);
    SET_RETURN_SEQ(get_surface, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_surface_get_main_surface, &lp_surf, 1);

    pointer_grab_focus(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(weston_pointer_clear_focus_fake.call_count, 0);

    free(lp_surf);
    free(mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_motion
 * @brief               Test case of pointer_grab_motion() where seat has a focus surface
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_motion()
 *                      -# Verification point:
 *                         +# weston_pointer_send_motion() must be called once time
 *                         +# forced_ptr_focus_surf of seat should become nullptr
 */
TEST_F(InputControllerTest, pointer_grab_motion)
{
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];

    pointer_grab_motion(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, nullptr, nullptr);

    EXPECT_EQ(weston_pointer_send_motion_fake.call_count, 1);
    EXPECT_EQ(mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf, nullptr);
}

/** ================================================================================================
 * @test_id             pointer_grab_button_dontInvokeFocusCallback
 * @brief               Test case of pointer_grab_button() should not call to pointer focus
 *                      callback when button_count is bigger than 0 or state is PRESSED.
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is 1
 *                      -# Calling the pointer_grab_button() time 1 with state is RELEASED
 *                      -# Set input ctxSeat with button_count is 0
 *                      -# Calling the pointer_grab_button() time 2 with state is PRESSED
 *                      -# Verification point:
 *                         +# focus() not be called
 *                         +# weston_pointer_send_button() must be called
 */
TEST_F(InputControllerTest, pointer_grab_button_dontInvokeFocusCallback)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    lp_pointer->button_count = 1;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.interface = &g_grabInterfaceFake;

    pointer_grab_button(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, nullptr, 0, WL_POINTER_BUTTON_STATE_RELEASED);

    lp_pointer->button_count = 0;

    pointer_grab_button(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, nullptr, 0, WL_POINTER_BUTTON_STATE_PRESSED);

    EXPECT_EQ(weston_pointer_send_button_fake.call_count, 2);
    EXPECT_EQ(focus_fake.call_count, 0);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_button_invokeFocusCallback
 * @brief               Test case of pointer_grab_button() where focus callback is invoked
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with button_count is 0
 *                      -# Calling the pointer_grab_button() with state is RELEASED
 *                      -# Verification point:
 *                         +# weston_pointer_send_button() must be called once time
 *                         +# focus() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_button_invokeFocusCallback)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    lp_pointer->button_count = 0;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.interface = &g_grabInterfaceFake;

    pointer_grab_button(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, nullptr, 0, WL_POINTER_BUTTON_STATE_RELEASED);

    EXPECT_EQ(weston_pointer_send_button_fake.call_count, 1);
    EXPECT_EQ(focus_fake.call_count, 1);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             pointer_grab_axis
 * @brief               Test case of pointer_grab_axis() to verify its steps
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_axis()
 *                      -# Verification point:
 *                         +# weston_pointer_send_axis() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_axis)
{
    pointer_grab_axis(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, nullptr, nullptr);

    ASSERT_EQ(weston_pointer_send_axis_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             pointer_grab_axis_source
 * @brief               Test case of pointer_grab_axis_source() to verify its steps
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_axis_source()
 *                      -# Verification point:
 *                         +# weston_pointer_send_axis_source() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_axis_source)
{
    pointer_grab_axis_source(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab, 0);

    ASSERT_EQ(weston_pointer_send_axis_source_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             pointer_grab_frame
 * @brief               Test case of pointer_grab_frame() to verify its steps
 * @test_procedure Steps:
 *                      -# Calling the pointer_grab_frame()
 *                      -# Verification point:
 *                         +# weston_pointer_send_frame() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_frame)
{
    pointer_grab_frame(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    ASSERT_EQ(weston_pointer_send_frame_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             pointer_grab_cancel
 * @brief               Test case of pointer_grab_cancel() to verify its steps
 * @test_procedure Steps:
 *                      -# Prepare the pointer object
 *                      -# Calling the pointer_grab_cancel()
 *                      -# Verification point:
 *                         +# weston_pointer_clear_focus() must be called once time
 */
TEST_F(InputControllerTest, pointer_grab_cancel)
{
    struct weston_pointer *lp_pointer = (struct weston_pointer *)malloc(sizeof(struct weston_pointer));
    struct weston_view l_view;
    l_view.surface = (struct weston_surface *)mp_fakePointer;
    lp_pointer->focus = &l_view;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = lp_pointer;

    pointer_grab_cancel(&mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab);

    EXPECT_EQ(weston_pointer_clear_focus_fake.call_count, 1);

    free(lp_pointer);
}

/** ================================================================================================
 * @test_id             touch_grab_down_noFocusView
 * @brief               Test case of touch_grab_down() where no focus view, it should not
 *                      do anything.
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with focus is null pointer
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() not be called
 */
TEST_F(InputControllerTest, touch_grab_down_noFocusView)
{
    struct weston_touch *lp_touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    lp_touch->focus = nullptr;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = lp_touch;

    touch_grab_down(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0, 0, 0);

    EXPECT_EQ(weston_surface_get_main_surface_fake.call_count, 0);

    free(lp_touch);
}

/** ================================================================================================
 * @test_id             touch_grab_down_noSurfCtx
 * @brief               Test case of touch_grab_down() where is no main surface
 * @test_procedure Steps:
 *                      -# Prepare mock for weston_surface_get_main_surface to return nullptr
 *                      -# Calling the touch_grab_down()
 *                      -# Verification point:
 *                         +# weston_surface_get_main_surface() must be called once time
 *                         +# weston_touch_send_down() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_down_noSurfCtx)
{
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus->surface = (struct weston_surface *)mp_fakePointer;
    SET_RETURN_SEQ(weston_surface_get_main_surface, (struct weston_surface**)&mp_nullPointer, 1);

    touch_grab_down(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0, 0, 0);

    EXPECT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    EXPECT_EQ(weston_touch_send_down_fake.call_count, 1);

    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_down_noNumTp
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
 */
TEST_F(InputControllerTest, touch_grab_down_noNumTp)
{
    struct weston_surface *l_surf = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus->surface = (struct weston_surface *)mp_fakePointer;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->num_tp = 0;
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    SET_RETURN_SEQ(get_surface, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_surface_get_main_surface, &l_surf, 1);

    touch_grab_down(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0, 0, 0);

    EXPECT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    EXPECT_EQ(weston_touch_send_down_fake.call_count, 1);
    EXPECT_EQ(get_id_of_surface_fake.call_count, 0);

    free(l_surf);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
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
 */
TEST_F(InputControllerTest, touch_grab_down_success)
{
    struct weston_surface *l_surf = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus->surface = (struct weston_surface *)0xFFFFFFFF;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->num_tp = 1;
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    SET_RETURN_SEQ(get_surface, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_surface_get_main_surface, &l_surf, 1);

    touch_grab_down(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0, 0, 0);

    EXPECT_EQ(weston_surface_get_main_surface_fake.call_count, 1);
    EXPECT_EQ(weston_touch_send_down_fake.call_count, 1);
    EXPECT_EQ(get_id_of_surface_fake.call_count, 1);

    free(l_surf);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_noFocusView
 * @brief               Test case of touch_grab_up() where no focus view, should not do anything
 * @test_procedure Steps:
 *                      -# Set input ctxSeat with focus is null pointer
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() not be called
 */
TEST_F(InputControllerTest, touch_grab_up_noFocusView)
{
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = nullptr;

    touch_grab_up(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0);

    EXPECT_EQ(weston_touch_send_up_fake.call_count, 0);

    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_hasNumTp
 * @brief               Test case of touch_grab_up() where input ctxSeat num_tp is enable {1}
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() must be called once time
 *                         +# get_id_of_surface() not be called
 */
TEST_F(InputControllerTest, touch_grab_up_hasNumTp)
{
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)mp_fakePointer;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->num_tp = 1;

    touch_grab_up(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0);

    EXPECT_EQ(weston_touch_send_up_fake.call_count, 1);
    EXPECT_EQ(get_id_of_surface_fake.call_count, 0);

    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_up_noNumTp
 * @brief               Test case of touch_grab_up() where no num tp
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Mocking the get_surface() does return an object
 *                      -# Mocking the weston_surface_get_main_surface() does return an object
 *                      -# Calling the touch_grab_up()
 *                      -# Verification point:
 *                         +# weston_touch_send_up() must be called once time
 *                         +# get_id_of_surface() must be called once time
 *                         +# wl_resource_post_event() must be called two times
 */
TEST_F(InputControllerTest, touch_grab_up_noNumTp)
{
    struct weston_surface *l_surf = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus->surface = (struct weston_surface *)mp_fakePointer;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->num_tp = 0;
    g_iviLayoutInterfaceFake.get_surface = get_surface;
    SET_RETURN_SEQ(get_surface, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_surface_get_main_surface, &l_surf, 1);

    touch_grab_up(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0);

    EXPECT_EQ(weston_touch_send_up_fake.call_count, 1);
    EXPECT_EQ(get_id_of_surface_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);

    free(l_surf);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             touch_grab_motion
 * @brief               Test case of touch_grab_motion() to verify its steps
 * @test_procedure Steps:
 *                      -# Calling the touch_grab_motion()
 *                      -# Verification point:
 *                         +# weston_touch_send_motion() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_motion)
{
    touch_grab_motion(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab, nullptr, 0, 0, 0);
    ASSERT_EQ(weston_touch_send_motion_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             touch_grab_frame
 * @brief               Test case of touch_grab_frame() to verify its steps
 * @test_procedure Steps:
 *                      -# Calling the touch_grab_frame()
 *                      -# Verification point:
 *                         +# weston_touch_send_frame() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_frame)
{
    touch_grab_frame(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab);
    ASSERT_EQ(weston_touch_send_frame_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             touch_grab_cancel_noSurface
 * @brief               Test case of touch_grab_cancel() where valid input params
 * @test_procedure Steps:
 *                      -# Set input ctxSeat
 *                      -# Calling the touch_grab_cancel()
 *                      -# Verification point:
 *                         +# weston_touch_set_focus() must be called once time
 */
TEST_F(InputControllerTest, touch_grab_cancel_noSurface)
{
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = (struct weston_touch *)malloc(sizeof(struct weston_touch));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus = (struct weston_view *)malloc(sizeof(struct weston_view));
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus->surface = (struct weston_surface *)mp_fakePointer;
    custom_wl_list_init(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus_resource_list);
    SET_RETURN_SEQ(weston_surface_get_main_surface, (struct weston_surface**)&mp_nullPointer, 1);

    touch_grab_cancel(&mpp_ctxSeat[DEFAULT_SEAT]->touch_grab);

    EXPECT_EQ(weston_touch_set_focus_fake.call_count, 1);

    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch->focus);
    free(mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_noSurface
 * @brief               Test case of setup_input_focus() where get_surface_from_id() returns wrong surface
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns wrong surface
 *                      -# Calling the setup_input_focus()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must not be called
 */
TEST_F(InputControllerTest, input_set_input_focus_noSurface)
{
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface**)&mp_fakePointer, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_ALL, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_wrongDevice
 * @brief               Test case of setup_input_focus() where get_surface_from_id() returns an object,
 *                      but no devices to focus.
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns an object
 *                      -# Calling the setup_input_focus() with input device is 0
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must not be called
 */
TEST_F(InputControllerTest, input_set_input_focus_wrongDevice)
{
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);

    setup_input_focus(mp_ctxInput, 10, 0, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_noKeyboard
 * @brief               Test case of setup_input_focus() where seat don't have the keyboard
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns an object
 *                      -# Mocking the weston_seat_get_keyboard() return nullptr
 *                      -# Calling the setup_input_focus() with input device is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 *                         +# wl_resource_post_event() must not be called
 */
TEST_F(InputControllerTest, input_set_input_focus_noKeyboard)
{
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_seat_get_keyboard, (struct weston_keyboard**)&mp_nullPointer, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_keyboard_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_disableKeyboard
 * @brief               Test case of setup_input_focus() where seat has the keyboard device, 
 *                      and surface try to un-focus it.
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the setup_input_focus() with input is disable
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 *                         +# wl_resource_post_event() must called 3 times
 *                         +# No ILM_INPUT_DEVICE_KEYBOARD device in mpp_seatFocus[DEFAULT_SEAT]->focus
 */
TEST_F(InputControllerTest, input_set_input_focus_disableKeyboard)
{
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    struct wl_resource l_wlResource;
    lp_westSurf->resource = &l_wlResource;
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_seatFocus[DEFAULT_SEAT]->focus = ILM_INPUT_DEVICE_KEYBOARD;
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);
    custom_wl_list_insert(&lp_keyboard->focus_resource_list, &l_wlResource.link);
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);
    SET_RETURN_SEQ(weston_seat_get_keyboard, &lp_keyboard, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_KEYBOARD, ILM_FALSE);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(weston_seat_get_keyboard_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 3);
    EXPECT_EQ(mpp_seatFocus[DEFAULT_SEAT]->focus & ILM_INPUT_DEVICE_KEYBOARD, 0);

    free(lp_westSurf);
    free(lp_keyboard);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_enableKeyboard
 * @brief               Test case of setup_input_focus() where seat has the keyboard device, 
 *                      and surface try to focus it
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the weston_seat_get_keyboard() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the setup_input_focus() with input is enable
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_keyboard() must be called once time
 *                         +# wl_resource_post_event() must called 4 times
 *                         +# ILM_INPUT_DEVICE_KEYBOARD device in mpp_seatFocus[DEFAULT_SEAT]->focus
 */
TEST_F(InputControllerTest, input_set_input_focus_enableKeyboard)
{
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    struct wl_resource l_wlResource;
    lp_westSurf->resource = &l_wlResource;
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_seatFocus[DEFAULT_SEAT]->focus = 0;
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);
    custom_wl_list_insert(&lp_keyboard->focus_resource_list, &l_wlResource.link);
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);
    SET_RETURN_SEQ(weston_seat_get_keyboard, &lp_keyboard, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(weston_seat_get_keyboard_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 4);
    EXPECT_EQ(mpp_seatFocus[DEFAULT_SEAT]->focus & ILM_INPUT_DEVICE_KEYBOARD, 1);

    free(lp_westSurf);
    free(lp_keyboard);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_noPointer
 * @brief               Test case of setup_input_focus() where no pointer device in seat
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns an object
 *                      -# Mocking the weston_seat_get_pointer() returns nullptr
 *                      -# Calling the setup_input_focus() with input enabled is 1
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 *                         +# wl_resource_post_event() must not be called
 */
TEST_F(InputControllerTest, input_set_input_focus_noPointer)
{
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer**)&mp_nullPointer, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_POINTER, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_disablePointer
 * @brief               Test case of setup_input_focus() where surface removes the focusion of pointer
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns an object
 *                      -# Mocking the weston_seat_get_pointer() returns an object
 *                      -# Calling the setup_input_focus() with input disable
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 *                         +# forced_surf_enabled of mpp_ctxSeat is false
 */
TEST_F(InputControllerTest, input_set_input_focus_disablePointer)
{
    auto pointer_focus = [](struct weston_pointer_grab *grab) {};
    struct weston_pointer_grab_interface l_pointerInterface = {.focus = pointer_focus};
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.interface = &l_pointerInterface;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = &mpp_iviSurface[DEFAULT_SEAT];
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_TRUE;

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_POINTER, ILM_FALSE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
    ASSERT_EQ(mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled, ILM_FALSE);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_enablePointer
 * @brief               Test case of setup_input_focus() where surface get the focusion of pointer
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() returns an object
 *                      -# Mocking the weston_seat_get_pointer() returns an object
 *                      -# Calling the setup_input_focus() with input enable
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# weston_seat_get_pointer() must be called once time
 *                         +# forced_surf_enabled of mpp_ctxSeat is true
*                         +# forced_ptr_focus_surf of mpp_ctxSeat sets to mpp_iviSurface[DEFAULT_SEAT]
 */
TEST_F(InputControllerTest, input_set_input_focus_enablePointer)
{
    auto pointer_focus = [](struct weston_pointer_grab *grab) {};
    struct weston_pointer_grab_interface l_pointerInterface = {.focus = pointer_focus};
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.interface = &l_pointerInterface;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf = nullptr;
    mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled = ILM_FALSE;

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_POINTER, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(weston_seat_get_pointer_fake.call_count, 1);
    ASSERT_EQ(mpp_ctxSeat[DEFAULT_SEAT]->forced_ptr_focus_surf, &mpp_iviSurface[DEFAULT_SEAT]);
    ASSERT_EQ(mpp_ctxSeat[DEFAULT_SEAT]->forced_surf_enabled, ILM_TRUE);
}

/** ================================================================================================
 * @test_id             input_set_input_focus_enableTouch
 * @brief               Test case of setup_input_focus() where to enable the touch device.
 *                      Cannot assign touch to special surface, may need to remove.
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the setup_input_focus() with input is enable
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called 2 times
 */
TEST_F(InputControllerTest, input_set_input_focus_enableTouch)
{
    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);

    setup_input_focus(mp_ctxInput, 10, ILM_INPUT_DEVICE_TOUCH, ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_wrongSeatName
 * @brief               Test case of setup_input_acceptance() where input seat name is "error".
 *                      that doesn't exist in list.
 * @test_procedure Steps:
 *                      -# Calling the setup_input_acceptance() with non-exist seat name
 *                      -# Verification point:
 *                         +# get_surface_from_id() not be called
 */
TEST_F(InputControllerTest, input_set_input_acceptance_wrongSeatName)
{
    setup_input_acceptance(mp_ctxInput, 10, "error", ILM_TRUE);
    ASSERT_EQ(get_surface_from_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_nullSurface
 * @brief               Test case of setup_input_acceptance() where get_surface_from_id()
 *                      returns a wrong surface
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() return wrong surface
 *                      -# Calling the setup_input_acceptance() with valid seat name
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(InputControllerTest, input_set_input_acceptance_nullSurface)
{
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface**)&mp_fakePointer, 1);

    setup_input_acceptance(mp_ctxInput, 10, mp_seatName[DEFAULT_SEAT], ILM_TRUE);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_removeAcceptance
 * @brief               Test case of setup_input_acceptance() for acceptance removing.
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() for resturn a valid surface
 *                      -# Mocking the weston_seat_get_keyboard() returns an object
 *                      -# Mocking the weston_seat_get_pointer() returns an object
 *                      -# Mocking the weston_seat_get_touch() returns an object
 *                      -# Mocking the surface_get_weston_surface() returns a weston_surface
  *                      -# Calling the setup_input_acceptance() with false input
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called 4 times
 *                         +# surface_get_weston_surface() must be called once time
 */
TEST_F(InputControllerTest, input_set_input_acceptance_removeAcceptance)
{
    struct weston_pointer l_pointer = {.focus = nullptr};
    struct weston_touch   l_touch = {.focus = nullptr};
    struct weston_surface *lp_westSurf = (struct weston_surface*)calloc(1, sizeof(struct weston_surface));
    struct weston_keyboard *lp_keyboard = (struct weston_keyboard*)calloc(1, sizeof(struct weston_keyboard));
    mpp_ctxSeat[DEFAULT_SEAT]->keyboard_grab.keyboard = lp_keyboard;
    mpp_ctxSeat[DEFAULT_SEAT]->pointer_grab.pointer = &l_pointer;
    mpp_ctxSeat[DEFAULT_SEAT]->touch_grab.touch = &l_touch;
    custom_wl_list_init(&lp_keyboard->focus_resource_list);
    custom_wl_list_init(&lp_keyboard->resource_list);

    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westSurf, 1);
    SET_RETURN_SEQ(weston_seat_get_keyboard, (struct weston_keyboard **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_seat_get_touch, (struct weston_touch **)&mp_fakePointer, 1);

    setup_input_acceptance(mp_ctxInput, 10, mp_seatName[DEFAULT_SEAT], ILM_FALSE);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 4);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);

    free(lp_westSurf);
    free(lp_keyboard);
    mpp_seatFocus[DEFAULT_SEAT] = nullptr;
}

/** ================================================================================================
 * @test_id             input_set_input_acceptance_doAcceptance
 * @brief               Test case of setup_input_acceptance() for doing acceptance
 * @test_procedure Steps:
 *                      -# Mocking the get_surface_from_id() does return first surface
 *                      -# Mocking the weston_seat_get_pointer() does return an object
 *                      -# Calling the setup_input_acceptance() with input is accepted
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called 2 times
 */
TEST_F(InputControllerTest, input_set_input_acceptance_doAcceptance)
{
    struct weston_pointer l_pointer = {.focus = mp_fakePointer};
    mpp_ctxSeat[CUSTOM_SEAT]->pointer_grab.pointer = &l_pointer;

    SET_RETURN_SEQ(get_surface_from_id, mpp_layoutSurface, 1);
    SET_RETURN_SEQ(weston_seat_get_pointer, (struct weston_pointer **)&mp_fakePointer, 1);

    setup_input_acceptance(mp_ctxInput, 10, mp_seatName[CUSTOM_SEAT], ILM_TRUE);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);
    EXPECT_EQ(wl_list_insert_fake.call_count, 1);

    struct seat_focus *lp_seatFocus = (struct seat_focus*)
            ((uintptr_t)wl_list_insert_fake.arg1_history[DEFAULT_SEAT] - offsetof(struct seat_focus, link));
    free(lp_seatFocus);
}

/** ================================================================================================
 * @test_id             handle_surface_destroy_wrongSurface
 * @brief               Test case of handle_surface_destroy() where invalid input params
 * @test_procedure Steps:
 *                      -# Calling the handle_surface_destroy()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(InputControllerTest, handle_surface_destroy_wrongSurface)
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
 */
TEST_F(InputControllerTest, handle_surface_destroy_success)
{
    handle_surface_destroy(&mp_ctxInput->surface_destroyed, &mpp_iviSurface[DEFAULT_SEAT]);

    EXPECT_EQ(wl_list_remove_fake.call_count, 1);

    mpp_seatFocus[DEFAULT_SEAT] = nullptr;
}

/** ================================================================================================
 * @test_id             handle_surface_create_noDefaultSeat
 * @brief               Test case of handle_surface_create() where no default seat
 * @test_procedure Steps:
 *                      -# Setup default seat is no_seat
 *                      -# Calling the handle_surface_create()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 *                         +# wl_list_init() must be called once time
 */
TEST_F(InputControllerTest, handle_surface_create_noDefaultSeat)
{
    struct ivisurface l_iviSurface;
    mp_ctxInput->seat_default_name = (char*)"no_seat";

    handle_surface_create(&mp_ctxInput->surface_created, &l_iviSurface);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(wl_list_init_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             handle_surface_create_hasDefaultSeat
 * @brief               Test case of handle_surface_create() where has default seat
 * @test_procedure Steps:
 *                      -# Setup default seat
 *                      -# Calling the handle_surface_create()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called 2 times
 *                         +# wl_list_insert() must be called 1 time
 */
TEST_F(InputControllerTest, handle_surface_create_hasDefaultSeat)
{
    struct ivisurface l_iviSurface;
    custom_wl_list_init(&l_iviSurface.accepted_seat_list);
    mp_ctxInput->seat_default_name = (char*)mp_seatName[DEFAULT_SEAT];
    l_iviSurface.shell = &m_iviShell;

    handle_surface_create(&mp_ctxInput->surface_created, &l_iviSurface);

    EXPECT_EQ(wl_resource_post_event_fake.call_count, 2);
    EXPECT_EQ(wl_list_insert_fake.call_count, 1);

    struct seat_focus *lp_seatFocus = (struct seat_focus*)
            ((uintptr_t)wl_list_insert_fake.arg1_history[DEFAULT_SEAT] - offsetof(struct seat_focus, link));
    free(lp_seatFocus);

}

/** ================================================================================================
 * @test_id             unbind_resource_controller
 * @brief               Test case of unbind_resource_controller() with a real resource
 * @test_procedure Steps:
 *                      -# Calling the unbind_resource_controller()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 */
TEST_F(InputControllerTest, unbind_resource_controller)
{
    unbind_resource_controller(&(mp_wlResource[DEFAULT_SEAT]));

    ASSERT_EQ(wl_list_remove_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             input_controller_destroy_wrongInput
 * @brief               Test case of input_controller_destroy() where wrong input
 * @test_procedure Steps:
 *                      -# Calling the input_controller_destroy()
 *                      -# wl_list_remove should not be called
 */
TEST_F(InputControllerTest, input_controller_destroy_wrongInput)
{
    input_controller_destroy(nullptr, nullptr);
    ASSERT_EQ(wl_list_remove_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             input_controller_destroy
 * @brief               Test case of input_controller_destroy() for destroying real object
 * @test_procedure Steps:
 *                      -# Calling the input_controller_destroy()
 *                      -# wl_list_remove should be called 6 times
 */
TEST_F(InputControllerTest, input_controller_destroy)
{
    wl_list_remove_fake.custom_fake = custom_wl_list_remove;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    mp_ctxInput->seat_default_name = nullptr;

    input_controller_destroy(&mp_ctxInput->shell_destroy_listener, nullptr);

    EXPECT_EQ(wl_list_remove_fake.call_count, 12);

    for(uint8_t i = 0; i < MAX_NUMBER; i++) {
        mpp_ctxSeat[i] = nullptr;
        mpp_seatFocus[i] = nullptr;
    }
    mp_ctxInput = nullptr;
}
