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
#include <string>
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"

extern "C"
{
#include "ivi-id-agent.c"
}

#define INVALID_ID 0xFFFFFFFF
static constexpr uint8_t MAX_NUMBER = 2;

class IdAgentTest: public ::testing::Test
{
public:
    void SetUp()
    {
        IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
        SERVER_API_FAKE_LIST(RESET_FAKE);
        init_controller_content();
    }

    void TearDown()
    {
        deinit_controller_content();
    }

    void init_controller_content()
    {
        m_iviShell.interface = &g_iviLayoutInterfaceFake;
        m_iviShell.compositor = &m_westonCompositor;
        custom_wl_list_init(&m_westonCompositor.seat_list);
        custom_wl_list_init(&m_iviShell.list_surface);
        custom_wl_list_init(&m_westonCompositor.seat_list);
        wl_signal_init(&m_iviShell.id_allocation_request_signal);
        wl_signal_init(&m_iviShell.ivisurface_created_signal);
        wl_signal_init(&m_iviShell.ivisurface_removed_signal);

        /* Initialize the ivi_id_agent */
        mp_iviIdAgent = (struct ivi_id_agent*)malloc(sizeof(struct ivi_id_agent));
        mp_iviIdAgent->compositor = &m_westonCompositor;
        mp_iviIdAgent->interface = &g_iviLayoutInterfaceFake;
        mp_iviIdAgent->default_surface_id = 100;
        mp_iviIdAgent->default_surface_id_max = 200;
        mp_iviIdAgent->default_behavior_set = 0;
        custom_wl_list_init(&mp_iviIdAgent->app_list);
        custom_wl_list_init(&mp_iviIdAgent->id_allocation_listener.link);
        custom_wl_list_init(&mp_iviIdAgent->surface_removed.link);
        custom_wl_list_init(&mp_iviIdAgent->destroy_listener.link);

        /* setup the list of desktop application:
         * app_1: {surface_id: 11, cfg_app_id: "1", cfg_title: "xdg_app_1", layout_surface: nullptr}
         * app_2: {surface_id: 12, cfg_app_id: "2", cfg_title: "xdg_app_2", layout_surface: nullptr}
         */
        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            mpp_dbElem[i] = (struct db_elem*)malloc(sizeof(struct db_elem));
            mpp_dbElem[i]->surface_id = 10 + i;
            mpp_dbElem[i]->cfg_app_id = (char*)malloc(5);
            mpp_dbElem[i]->cfg_title = (char*)malloc(10);
            mpp_dbElem[i]->layout_surface = nullptr;
            snprintf(mpp_dbElem[i]->cfg_app_id, 5, "%d", i);
            snprintf(mpp_dbElem[i]->cfg_title, 10, "xdg_app_%d", i);
            custom_wl_list_insert(&mp_iviIdAgent->app_list, &mpp_dbElem[i]->link);
        }
    }

    void deinit_controller_content()
    {
        for(uint8_t i = 0; i < MAX_NUMBER; i++) {
            if(mpp_dbElem[i] != nullptr) {
                free(mpp_dbElem[i]->cfg_app_id);
                free(mpp_dbElem[i]->cfg_title);
                free(mpp_dbElem[i]);
            }
        }

        if(mp_iviIdAgent != nullptr)
            free(mp_iviIdAgent);
    }

    struct ivishell m_iviShell = {};
    struct weston_compositor m_westonCompositor = {};
    struct ivi_id_agent *mp_iviIdAgent = nullptr;
    struct db_elem *mpp_dbElem[MAX_NUMBER] = {nullptr};

    void *mp_fakePointer = (void*)0xFFFFFFFF;
    void *mp_nullPointer = nullptr;

    static uint32_t ms_surfaceId;
    static uint32_t ms_defaultSurfaceId;
    static uint32_t ms_defaultSurfaceIdMax;
    static char *ms_appId;
    static char *ms_appTitle;
};

char *IdAgentTest::ms_appId = (char*)"0";
char *IdAgentTest::ms_appTitle = (char*)"xdg_app_0";
uint32_t IdAgentTest::ms_surfaceId = 10;
uint32_t IdAgentTest::ms_defaultSurfaceId = 100;
uint32_t IdAgentTest::ms_defaultSurfaceIdMax = 200;

static int custom_weston_config_next_section_1(struct weston_config *config, struct weston_config_section **section, const char **name)
{
    *name = "desktop-app";
    return 1;
}

static int custom_weston_config_next_section_2(struct weston_config *config, struct weston_config_section **section, const char **name)
{
    return 0;
}

static int custom_weston_config_section_get_uint(struct weston_config_section *section, const char *key, uint32_t *value, uint32_t default_value)
{
    if(strcmp(key, "surface-id") == 0)
        *value = IdAgentTest::ms_surfaceId;
    else if(strcmp(key, "default-surface-id") == 0)
        *value = IdAgentTest::ms_defaultSurfaceId;
    else if(strcmp(key, "default-surface-id-max") == 0)
        *value = IdAgentTest::ms_defaultSurfaceIdMax;
    return 0;
}

static int custom_weston_config_section_get_string(struct weston_config_section *section, const char *key, char **value, const char *default_value)
{
    if(strcmp(key, "app-id") == 0)
        *value = (IdAgentTest::ms_appId != nullptr) ? strdup(IdAgentTest::ms_appId) : nullptr;
    else if(strcmp(key, "app-title") == 0)
        *value = (IdAgentTest::ms_appTitle != nullptr) ? strdup(IdAgentTest::ms_appId) : nullptr;
    return 0;
}

/** ================================================================================================
 * @test_id             surface_event_remove
 * @brief               Test case of surface_event_remove() where layout_surface is
 *                      existing in app list.
 * @test_procedure Steps:
 *                      -# Set the layout surface for db elem, to check function
 *                      -# Calling the surface_event_remove()
 *                      -# Verification point:
 *                         +# If the db elem have the layout surface is mp_fakePointer, it should reset to null pointer
 *                         +# If not, keep the right pointer
 */
TEST_F(IdAgentTest, surface_event_remove)
{
    mpp_dbElem[0]->layout_surface = (struct ivi_layout_surface *)mp_fakePointer;
    mpp_dbElem[1]->layout_surface = (struct ivi_layout_surface *)0xFFFFFF00;

    surface_event_remove(&mp_iviIdAgent->surface_removed, mp_fakePointer);

    ASSERT_EQ(mpp_dbElem[0]->layout_surface, nullptr);
    ASSERT_EQ(mpp_dbElem[1]->layout_surface, (struct ivi_layout_surface *)0xFFFFFF00);
}

/** ================================================================================================
 * @test_id             id_agent_module_deinit
 * @brief               Test case of id_agent_module_deinit(), it should release all the resources
 * @test_procedure Steps:
 *                      -# Calling the id_agent_module_deinit()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 5 times
 */
TEST_F(IdAgentTest, id_agent_module_deinit)
{
    id_agent_module_deinit(&mp_iviIdAgent->destroy_listener, nullptr);
    ASSERT_EQ(wl_list_remove_fake.call_count, 5);

    /* Prevent double free in TearDown*/
    for(uint8_t i = 0; i < MAX_NUMBER; i++)
    {
        mpp_dbElem[i] = nullptr;
    }
    mp_iviIdAgent = nullptr;
}

/** ================================================================================================
 * @test_id             id_agent_module_init_cannotGetwestonConfig
 * @brief               Test case of id_agent_module_init() where wet_get_config() returns nullptr object
 * @test_procedure Steps:
 *                      -# Prepare mock for wet_get_config(), to return a nullptr
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# wl_list_remove() must be called three times
 *                         +# weston_config_get_section() not be called
 */
TEST_F(IdAgentTest, id_agent_module_init_cannotGetwestonConfig)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_nullPointer, 1);

    ASSERT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(wl_list_remove_fake.call_count, 3);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehavior
 * @brief               Test case of id_agent_module_init() where no section of "desktop-app-default"
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return a nullptr
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_get_section() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() not be called
 *                         +# weston_config_section_get_string() not be called
 *                         +# wl_list_remove() must be called three times
 *                         +# wl_list_empty() must return 0
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehavior)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_nullPointer, 1);

    ASSERT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 1);
    ASSERT_EQ(strncmp(weston_config_get_section_fake.arg1_val, "desktop-app-default", 19), 0);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 0);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 0);
    ASSERT_NE(wl_list_empty_fake.return_val_history[0], 0);
    ASSERT_EQ(wl_list_remove_fake.call_count, 3);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDesktopApp
 * @brief               Test case of id_agent_module_init() where has "desktop-app-default" section with
 *                      "default-surface-id" and "default-surface-id-max" are INVALID_ID.
 *                      these is no "desktop-app" section.
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return a nullptr
 *                      -# Set the default surface id and default surface id max to INVALID_ID
 *                      -# Prepare mock for weston_config_section_get_uint(), to return expectation values
 *                      -# Calling the id_agent_module_init() time 1
 *                      -# Set the default surface id to valid value and default surface id max to INVALID_ID
 *                      -# Calling the id_agent_module_init() time 2
 *                      -# Verification point:
 *                         +# id_agent_module_init() time 1 and time 2 must return IVI_FAILED
 *                         +# wet_get_config() must be called 2 times
 *                         +# weston_config_get_section() must be called 2 times
 *                         +# weston_config_section_get_uint() must be called 4 times
 *                         +# weston_config_section_get_string() not be called
 *                         +# wl_list_remove_fake() must be called 6 times
 */
TEST_F(IdAgentTest, id_agent_module_init_noDesktopApp)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    IdAgentTest::ms_defaultSurfaceId = INVALID_ID;
    IdAgentTest::ms_defaultSurfaceIdMax = INVALID_ID;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;

    /*"default-surface-id" and "default-surface-id-max" should not be INVALID_ID*/
    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);
    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = INVALID_ID;
    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 2);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 2);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 4);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 0);
    ASSERT_EQ(wl_list_remove_fake.call_count, 6);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_hasDesktopAppInvalidSurfaceId
 * @brief               Test case of id_agent_module_init() where has "desktop-app-default" and
 *                      "desktop-app" sections. But the "surface-id" has setup with invalid.
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the surface id to INVALID_ID
 *                      -# Prepare mock for weston_config_section_get_uint(), to return expectation value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called three times
 *                         +# weston_config_section_get_string() not be called
 *                         +# wl_list_remove_fake() must be called 3 times
 */
TEST_F(IdAgentTest, id_agent_module_init_hasDesktopAppInvalidSurfaceId)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1, // mock for "desktop-app" section
        custom_weston_config_next_section_2  // mock to notify end section
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);
    IdAgentTest::ms_surfaceId = INVALID_ID;
    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;

    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    EXPECT_EQ(wet_get_config_fake.call_count, 1);
    EXPECT_EQ(weston_config_next_section_fake.call_count, 1);
    EXPECT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    EXPECT_EQ(strncmp("surface-id", weston_config_section_get_uint_fake.arg1_history[2], 11), 0);
    EXPECT_EQ(weston_config_section_get_string_fake.call_count, 0);
    EXPECT_EQ(wl_list_remove_fake.call_count, 3);

    struct db_elem *db_elem = (struct db_elem *)
            ((uintptr_t)weston_config_section_get_uint_fake.arg2_history[2] - offsetof(struct db_elem, surface_id));
    free(db_elem);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_hasDesktopAppInvalidOfAppIdAndAppTitle
 * @brief               Test case of id_agent_module_init() where has "desktop-app-default" and
 *                      "desktop-app" sections. The app has valid "surface-id", and invalid
 *                      for "app-id" and "app-title".
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the surface id to a valid value
 *                      -# Set the appTitle and appId to null pointer
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Prepare mock for weston_config_section_get_string(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called three times
 *                         +# weston_config_section_get_string() must be called 2 times
 *                         +# wl_list_remove_fake() must be called 3 times
 */
TEST_F(IdAgentTest, id_agent_module_init_hasDesktopAppInvalidOfAppIdAndAppTitle)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1, // mock for "desktop-app" section
        custom_weston_config_next_section_2  // mock to notify end section
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);
    IdAgentTest::ms_surfaceId = 10;
    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    IdAgentTest::ms_appId = nullptr;
    IdAgentTest::ms_appTitle = nullptr;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    EXPECT_EQ(wet_get_config_fake.call_count, 1);
    EXPECT_EQ(weston_config_next_section_fake.call_count, 1);
    EXPECT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    EXPECT_EQ(weston_config_section_get_string_fake.call_count, 2);
    EXPECT_EQ(wl_list_remove_fake.call_count, 3);

    struct db_elem *db_elem = (struct db_elem *)
            ((uintptr_t)weston_config_section_get_uint_fake.arg2_history[2] - offsetof(struct db_elem, surface_id));
    free(db_elem);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehaviorHasDesktopApp
 * @brief               Test case of id_agent_module_init() where has no "desktop-app-default" section.
 *                      The "desktop-app" section has valid "surface-id", "app-id" and "app-title".
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return nullptr
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set "surface-id", "app-id" and "app-title" to valid values
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Prepare mock for weston_config_section_get_string(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() should return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called once time
 *                         +# weston_config_section_get_string() must be called 2 times
 *                         +# wl_list_remove_fake() must be called 3 times
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehaviorHasDesktopApp)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1, // mock for "desktop-app" section
        custom_weston_config_next_section_2  // mock to notify end section
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_nullPointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);
    IdAgentTest::ms_surfaceId = 10;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"xdg_app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    EXPECT_EQ(wet_get_config_fake.call_count, 1);
    EXPECT_EQ(weston_config_next_section_fake.call_count, 2);
    EXPECT_EQ(weston_config_section_get_uint_fake.call_count, 1);
    EXPECT_EQ(weston_config_section_get_string_fake.call_count, 2);
    EXPECT_EQ(wl_list_remove_fake.call_count, 3);

    struct db_elem *db_elem = (struct db_elem *)
            ((uintptr_t)weston_config_section_get_uint_fake.arg2_history[0] - offsetof(struct db_elem, surface_id));
    free(db_elem->cfg_app_id);
    free(db_elem->cfg_title);
    free(db_elem);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_hasDefaultBehaviorHasDesktopAppUnexpectedSurfaceId
 * @brief               Test case of id_agent_module_init() where has "desktop-app-default" section.
 *                      The "desktop-app" section has valid "surface-id", "app-id" and "app-title".
 *                      but the "surface-id" is in range of "default-surface-id" and "default-surface-id-max".
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the default surface id and default surface id max
 *                      -# Set the surface id to a valid value but it exceeds the default value
 *                      -# Set the appTitle and appId to a valid value
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Prepare mock for weston_config_section_get_string(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_get_section() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called 3 times
 *                         +# weston_config_section_get_string() must be called 2 times
 *                         +# wl_list_remove_fake() must be called 3 times
 */
TEST_F(IdAgentTest, id_agent_module_init_hasDefaultBehaviorHasDesktopAppUnexpectedSurfaceId)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1, // mock for "desktop-app" section
        custom_weston_config_next_section_2  // mock to notify end section
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);
    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    IdAgentTest::ms_surfaceId = IdAgentTest::ms_defaultSurfaceId + 1;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"xdg_app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_FAILED);

    EXPECT_EQ(wet_get_config_fake.call_count, 1);
    EXPECT_EQ(weston_config_get_section_fake.call_count, 1);
    EXPECT_EQ(weston_config_next_section_fake.call_count, 1);
    EXPECT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    EXPECT_EQ(weston_config_section_get_string_fake.call_count, 2);
    EXPECT_EQ(wl_list_remove_fake.call_count, 3);

    struct db_elem *db_elem = (struct db_elem *)
            ((uintptr_t)weston_config_section_get_uint_fake.arg2_history[2] - offsetof(struct db_elem, surface_id));
    free(db_elem->cfg_app_id);
    free(db_elem->cfg_title);
    free(db_elem);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_success
 * @brief               Test case of id_agent_module_init() where has "desktop-app-default" section.
 *                      The "desktop-app" section has valid "surface-id", "app-id" and "app-title".
 * @test_procedure Steps:
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the default surface id and default surface id max
 *                      -# Set the surface id to a valid value that is in the range of the default value
 *                      -# Set the appTitle and appId to a valid value
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Prepare mock for weston_config_section_get_string(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_SUCCEEDED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_get_section() must be called once time
 *                         +# weston_config_next_section() must be called 2 times
 *                         +# weston_config_section_get_uint() must be called 3 times
 *                         +# weston_config_section_get_string() must be called 2 times
 *                         +# wl_list_remove_fake() must be called 0 time
 */
TEST_F(IdAgentTest, id_agent_module_init_success)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1, // mock for "desktop-app" section
        custom_weston_config_next_section_2  // mock to notify end section
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);
    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    IdAgentTest::ms_surfaceId = 10;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"xdg_app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    EXPECT_EQ(id_agent_module_init(&m_iviShell), IVI_SUCCEEDED);

    EXPECT_EQ(wet_get_config_fake.call_count, 1);
    EXPECT_EQ(weston_config_get_section_fake.call_count, 1);
    EXPECT_EQ(weston_config_next_section_fake.call_count, 2);
    EXPECT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    EXPECT_EQ(weston_config_section_get_string_fake.call_count, 2);
    EXPECT_EQ(wl_list_remove_fake.call_count, 0);

    struct db_elem *db_elem = (struct db_elem *)
            ((uintptr_t)weston_config_section_get_uint_fake.arg2_history[2] - offsetof(struct db_elem, surface_id));
    free(db_elem->cfg_app_id);
    free(db_elem->cfg_title);
    free(db_elem);

    struct ivi_id_agent *ida = (struct ivi_id_agent *)
            ((uintptr_t)add_listener_remove_surface_fake.arg0_history[0] - offsetof(struct ivi_id_agent, surface_removed));
    free(ida);
}
