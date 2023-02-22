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

#include "ivi_id_agent_base_class.hpp"
#include <gtest/gtest.h>

#define INVALID_ID 0xFFFFFFFF
static constexpr uint8_t MAX_NUMBER = 2;

class IdAgentTest: public ::testing::Test, public IdAgentBase
{
public:
    void SetUp()
    {
        ASSERT_EQ(initBaseModule(), true);
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

        mp_iviIdAgent = (struct ivi_id_agent*)malloc(sizeof(struct ivi_id_agent));
        mp_iviIdAgent->compositor = &m_westonCompositor;
        mp_iviIdAgent->interface = &g_iviLayoutInterfaceFake;
        mp_iviIdAgent->default_surface_id = 100;
        mp_iviIdAgent->default_surface_id_max = 200;
        mp_iviIdAgent->default_behavior_set = 0;

        custom_wl_list_init(&mp_iviIdAgent->app_list);
        custom_wl_list_init(&m_westonCompositor.destroy_signal.listener_list);

        for(uint8_t i = 0; i < MAX_NUMBER; i++)
        {
            // prepare for desktop apps
            mp_dbElem[i] = (struct db_elem*)malloc(sizeof(struct db_elem));
            mp_dbElem[i]->surface_id = 10 + i;
            mp_dbElem[i]->cfg_app_id = (char*)malloc(5);
            mp_dbElem[i]->cfg_title = (char*)malloc(10);
            snprintf(mp_dbElem[i]->cfg_app_id, 5, "%d", i);
            snprintf(mp_dbElem[i]->cfg_title, 10, "idtest%d", i);
            custom_wl_list_insert(&mp_iviIdAgent->app_list, &mp_dbElem[i]->link);
        }
    }

    void deinit_controller_content()
    {
        for(uint8_t i = 0; i < MAX_NUMBER; i++)
        {
            if(mp_dbElem[i] != nullptr)
            {
                free(mp_dbElem[i]->cfg_app_id);
                free(mp_dbElem[i]->cfg_title);
                free(mp_dbElem[i]);
            }
        }
        if(mp_iviIdAgent != nullptr)
        {
            free(mp_iviIdAgent);
        }
    }

    struct ivi_id_agent *mp_iviIdAgent = nullptr;
    struct weston_compositor m_westonCompositor = {};

    struct db_elem *mp_dbElem[MAX_NUMBER] = {nullptr};
    void *mp_fakePointer = (void*)0xFFFFFFFF;
    void *mp_nullPointer = nullptr;

    static uint32_t ms_surfaceId;
    static uint32_t ms_defaultSurfaceId;
    static uint32_t ms_defaultSurfaceIdMax;
    static char *ms_appId;
    static char *ms_appTitle;

};

char *IdAgentTest::ms_appId = (char*)"0";
char *IdAgentTest::ms_appTitle = (char*)"app_1";
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
    {
        *value = IdAgentTest::ms_surfaceId;
    }
    else if(strcmp(key, "default-surface-id") == 0)
    {
        *value = IdAgentTest::ms_defaultSurfaceId;
    }
    else if(strcmp(key, "default-surface-id-max") == 0)
    {
        *value = IdAgentTest::ms_defaultSurfaceIdMax;
    }
    return 0;
}

static int custom_weston_config_section_get_string(struct weston_config_section *section, const char *key, char **value, const char *default_value)
{
    if(strcmp(key, "app-id") == 0)
    {
        *value = (IdAgentTest::ms_appId != nullptr) ? strdup(IdAgentTest::ms_appId) : nullptr;
    }
    else if(strcmp(key, "app-title") == 0)
    {
        *value = (IdAgentTest::ms_appTitle != nullptr) ? strdup(IdAgentTest::ms_appId) : nullptr;
    }
    return 0;
}

/** ================================================================================================
 * @test_id             desktop_surface_event_configure_hasDataInList
 * @brief               Test case of desktop_surface_event_configure() where
 *                      cfg_app_id and cfg_title are in the same object, get_id_from_config return IVI_SUCCEEDED
 * @test_procedure Steps:
 *                      -# Mocking the weston_desktop_surface_get_app_id() to return mp_dbElem[0]->cfg_app_id
 *                      -# Mocking the weston_desktop_surface_get_title() to return mp_dbElem[0]->cfg_title
 *                      -# Calling the desktop_surface_event_configure()
 *                      -# Verification point:
 *                         +# The result output should same with prepare data
 *                         +# surface_set_id() must be called once time
 *                         +# Input surface_set_id() should be mp_dbElem[0]->surface_id
 */
TEST_F(IdAgentTest, desktop_surface_event_configure_hasDataInList)
{
    SET_RETURN_SEQ(weston_desktop_surface_get_app_id, (const char**)&mp_dbElem[0]->cfg_app_id, 1);
    SET_RETURN_SEQ(weston_desktop_surface_get_title, (const char**)&mp_dbElem[0]->cfg_title, 1);

    desktop_surface_event_configure(&mp_iviIdAgent->desktop_surface_configured, mp_fakePointer);

    ASSERT_EQ(mp_dbElem[0]->layout_surface, mp_fakePointer);
    ASSERT_EQ(surface_set_id_fake.call_count, 1);
    ASSERT_EQ(surface_set_id_fake.arg1_history[0], mp_dbElem[0]->surface_id);
}

/** ================================================================================================
 * @test_id             desktop_surface_event_configure_noDataInListNoDefaultBehavior
 * @brief               Test case of desktop_surface_event_configure() where
 *                      cfg_app_id and cfg_title are in the different object, get_id_from_config return IVI_FAILED
 * @test_procedure Steps:
 *                      -# Mocking the weston_desktop_surface_get_app_id() to return mp_dbElem[1]->cfg_app_id
 *                      -# Mocking the weston_desktop_surface_get_title() to return mp_dbElem[0]->cfg_title
 *                      -# Calling the desktop_surface_event_configure()
 *                      -# Verification point:
 *                         +# surface_set_id() not be called
 */
TEST_F(IdAgentTest, desktop_surface_event_configure_noDataInListNoDefaultBehavior)
{
    SET_RETURN_SEQ(weston_desktop_surface_get_app_id, (const char**)&mp_dbElem[1]->cfg_app_id, 1);
    SET_RETURN_SEQ(weston_desktop_surface_get_title, (const char**)&mp_dbElem[0]->cfg_title, 1);

    desktop_surface_event_configure(&mp_iviIdAgent->desktop_surface_configured, mp_fakePointer);

    ASSERT_EQ(surface_set_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             desktop_surface_event_configure_noDataInListHasDefaultBehavior
 * @brief               Test case of desktop_surface_event_configure() where
 *                      cfg_app_id and cfg_title are in the different object, get_id_from_config return IVI_FAILED
 *                      and enable the default behavior
 * @test_procedure Steps:
 *                      -# Set default_behavior_set to 1 (Enable the default behavior)
 *                      -# Mocking the weston_desktop_surface_get_app_id() to return mp_dbElem[1]->cfg_app_id
 *                      -# Mocking the weston_desktop_surface_get_title() to return mp_dbElem[0]->cfg_title
 *                      -# Calling the desktop_surface_event_configure()
 *                      -# Verification point:
 *                         +# surface_set_id() must be called once time
 *                         +# Input surface_set_id() should be {mp_iviIdAgent->default_surface_id - 1}
 */
TEST_F(IdAgentTest, desktop_surface_event_configure_noDataInListHasDefaultBehavior)
{
    mp_iviIdAgent->default_behavior_set = 1;
    SET_RETURN_SEQ(weston_desktop_surface_get_app_id, (const char**)&mp_dbElem[1]->cfg_app_id, 1);
    SET_RETURN_SEQ(weston_desktop_surface_get_title, (const char**)&mp_dbElem[0]->cfg_title, 1);

    desktop_surface_event_configure(&mp_iviIdAgent->desktop_surface_configured, mp_fakePointer);

    ASSERT_EQ(surface_set_id_fake.call_count, 1);
    ASSERT_EQ(surface_set_id_fake.arg1_history[0], mp_iviIdAgent->default_surface_id - 1);
}

/** ================================================================================================
 * @test_id             desktop_surface_event_configure_noDataInListHasDefaultBehaviorExistSurfaceId
 * @brief               Test case of desktop_surface_event_configure() where
 *                      cfg_app_id and cfg_title are in the different object, get_id_from_config return IVI_FAILED
 *                      and enable the default behavior and exist surface id
 * @test_procedure Steps:
 *                      -# Set default_behavior_set to 1 (Enable the default behavior)
 *                      -# Mocking the weston_desktop_surface_get_app_id() to return mp_dbElem[1]->cfg_app_id
 *                      -# Mocking the weston_desktop_surface_get_title() to return mp_dbElem[0]->cfg_title
 *                      -# Mocking the get_surface_from_id() to return an object (not null pointer)
 *                      -# Calling the desktop_surface_event_configure()
 *                      -# Verification point:
 *                         +# surface_set_id() not be called
 */
TEST_F(IdAgentTest, desktop_surface_event_configure_noDataInListHasDefaultBehaviorExistSurfaceId)
{
    mp_iviIdAgent->default_behavior_set = 1;
    SET_RETURN_SEQ(weston_desktop_surface_get_app_id, (const char**)&mp_dbElem[1]->cfg_app_id, 1);
    SET_RETURN_SEQ(weston_desktop_surface_get_title, (const char**)&mp_dbElem[0]->cfg_title, 1);

    struct ivi_layout_surface *lp_fakeLayoutLayer = (struct ivi_layout_surface *)0xFFFFFF00;
    SET_RETURN_SEQ(get_surface_from_id, &lp_fakeLayoutLayer, 1);

    desktop_surface_event_configure(&mp_iviIdAgent->desktop_surface_configured, mp_fakePointer);

    ASSERT_EQ(surface_set_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             desktop_surface_event_configure_noDataInListHasDefaultBehaviorOverMaxId
 * @brief               Test case of desktop_surface_event_configure() where
 *                      cfg_app_id and cfg_title are in the different object, get_id_from_config return IVI_FAILED
 *                      and enable the default behavior and the default id and default id max are same
 * @test_procedure Steps:
 *                      -# Set default_behavior_set to 1 (Enable the default behavior)
 *                      -# Set the default id and default max id are same
 *                      -# Mocking the weston_desktop_surface_get_app_id() to return mp_dbElem[1]->cfg_app_id
 *                      -# Mocking the weston_desktop_surface_get_title() to return mp_dbElem[0]->cfg_title
 *                      -# Calling the desktop_surface_event_configure()
 *                      -# Verification point:
 *                         +# surface_set_id() not be called
 */
TEST_F(IdAgentTest, desktop_surface_event_configure_noDataInListHasDefaultBehaviorOverMaxId)
{
    mp_iviIdAgent->default_behavior_set = 1;
    mp_iviIdAgent->default_surface_id = 200;
    mp_iviIdAgent->default_surface_id_max = 200;
    SET_RETURN_SEQ(weston_desktop_surface_get_app_id, (const char**)&mp_dbElem[1]->cfg_app_id, 1);
    SET_RETURN_SEQ(weston_desktop_surface_get_title, (const char**)&mp_dbElem[0]->cfg_title, 1);

    desktop_surface_event_configure(&mp_iviIdAgent->desktop_surface_configured, mp_fakePointer);

    ASSERT_EQ(surface_set_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             surface_event_remove
 * @brief               Test case of surface_event_remove() where valid input params
 * @test_procedure Steps:
 *                      -# Set the layout surface for db elem, to check function
 *                      -# Calling the surface_event_remove()
 *                      -# Verification point:
 *                         +# If the db elem have the layout surface is mp_fakePointer, it should reset to null pointer
 *                         +# If not, keep the right pointer
 */
TEST_F(IdAgentTest, surface_event_remove)
{
    mp_dbElem[0]->layout_surface = (struct ivi_layout_surface *)mp_fakePointer;
    mp_dbElem[1]->layout_surface = (struct ivi_layout_surface *)0xFFFFFF00;

    surface_event_remove(&mp_iviIdAgent->surface_removed, mp_fakePointer);

    ASSERT_EQ(mp_dbElem[0]->layout_surface, nullptr);
    ASSERT_EQ(mp_dbElem[1]->layout_surface, (struct ivi_layout_surface *)0xFFFFFF00);
}

/** ================================================================================================
 * @test_id             id_agent_module_deinit
 * @brief               Test case of id_agent_module_deinit() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the id_agent_module_deinit()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 5 times
 *                         +# Free resources are allocated when running the test
 */
TEST_F(IdAgentTest, id_agent_module_deinit)
{
    id_agent_module_deinit(&mp_iviIdAgent->destroy_listener, nullptr);

    ASSERT_EQ(wl_list_remove_fake.call_count, 5);

    for(uint8_t i = 0; i < MAX_NUMBER; i++)
    {
        mp_dbElem[i] = nullptr;
    }
    mp_iviIdAgent = nullptr;
}

/** ================================================================================================
 * @test_id             id_agent_module_init_cannotGetwestonConfig
 * @brief               Test case of id_agent_module_init() where wet_get_config() does not mock, return null pointer
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_get_section() not be called
 */
TEST_F(IdAgentTest, id_agent_module_init_cannotGetwestonConfig)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehaviorNoDesktopApp
 * @brief               Test case of id_agent_module_init() where weston_config_get_section() does not mock, return null pointer
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_get_section() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() not be called
 *                         +# weston_config_section_get_string() not be called
 *                         +# wl_list_empty() must return 0
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehaviorNoDesktopApp)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 0);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 0);
    ASSERT_NE(wl_list_empty_fake.return_val_history[0], 0);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_hasDefaultBehaviorNoDesktopApp
 * @brief               Test case of id_agent_module_init() where weston_config_get_section() does mock, return an object
 *                      and default surface id and default surface id max are INVALID_ID
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_get_section() to return an object
 *                      -# Set the default surface id and default surface id max to INVALID_ID
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Calling the id_agent_module_init() time 1
 *                      -# Set the default surface id to valid value and default surface id max to INVALID_ID
 *                      -# Calling the id_agent_module_init() time 2
 *                      -# Verification point:
 *                         +# id_agent_module_init() time 1 and time 2 must return IVI_FAILED
 *                         +# wet_get_config() must be called 2 times
 *                         +# weston_config_get_section() must be called 2 times
 *                         +# weston_config_section_get_uint() must be called 4 times
 *                         +# weston_config_section_get_string() not be called
 */
TEST_F(IdAgentTest, id_agent_module_init_hasDefaultBehaviorNoDesktopApp)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);

    IdAgentTest::ms_defaultSurfaceId = INVALID_ID;
    IdAgentTest::ms_defaultSurfaceIdMax = INVALID_ID;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;

    EXPECT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = INVALID_ID;
    EXPECT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 2);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 2);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 4);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehaviorHasDesktopAppWithInvalidSurfaceId
 * @brief               Test case of id_agent_module_init() where weston_config_next_section() does mock, return an object
 *                      and no set default behavior and invalid surface id
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the surface id to INVALID_ID
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called once time
 *                         +# weston_config_section_get_string() not be called
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehaviorHasDesktopAppWithInvalidSurfaceId)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1,
        custom_weston_config_next_section_2
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);

    IdAgentTest::ms_surfaceId = INVALID_ID;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehaviorHasDesktopAppWithNullOfAppIdAndAppTitle
 * @brief               Test case of id_agent_module_init() where weston_config_next_section() does mock, return an object
 *                      and no set default behavior and valid surface id and invalid appTitle, appId
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Mocking the wet_get_config() to return an object
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
 *                         +# weston_config_section_get_uint() must be called once time
 *                         +# weston_config_section_get_string() must be called 2 times
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehaviorHasDesktopAppWithNullOfAppIdAndAppTitle)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1,
        custom_weston_config_next_section_2
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);

    IdAgentTest::ms_surfaceId = 10;
    IdAgentTest::ms_appId = nullptr;
    IdAgentTest::ms_appTitle = nullptr;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_noDefaultBehaviorHasDesktopAppWithRightConfig
 * @brief               Test case of id_agent_module_init() where weston_config_next_section() does mock, return an object
 *                      and no set default behavior and valid surface id, appTitle, appId
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
 *                      -# Mocking the wet_get_config() to return an object
 *                      -# Mocking the weston_config_next_section() to return an object
 *                      -# Set the surface id to a valid value
 *                      -# Set the appTitle and appId to a valid value
 *                      -# Prepare mock for weston_config_section_get_uint(), to set the input value
 *                      -# Prepare mock for weston_config_section_get_string(), to set the input value
 *                      -# Calling the id_agent_module_init()
 *                      -# Verification point:
 *                         +# id_agent_module_init() must return IVI_FAILED
 *                         +# wet_get_config() must be called once time
 *                         +# weston_config_next_section() must be called once time
 *                         +# weston_config_section_get_uint() must be called once time
 *                         +# weston_config_section_get_string() must be called 2 times
 * @todo                Maybe have a logic issue here, do we must have the default-app-default section?
 *                      only that section configured the default id and default max id
 */
TEST_F(IdAgentTest, id_agent_module_init_noDefaultBehaviorHasDesktopAppWithRightConfig)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1,
        custom_weston_config_next_section_2
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);

    IdAgentTest::ms_surfaceId = 0;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_hasDefaultBehaviorHasDesktopAppWithRightConfig
 * @brief               Test case of id_agent_module_init() where weston_config_next_section() does mock, return an object
 *                      and set default behavior and valid surface id, appTitle, appId but surface id exceeds the default value
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
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
 * @todo                Maybe have a logic issue here, range of surface id in check_config functions
 *                      enable free resoure, if there is a change in the lib
 */
TEST_F(IdAgentTest, id_agent_module_init_hasDefaultBehaviorHasDesktopAppWithRightConfig)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1,
        custom_weston_config_next_section_2
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);

    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    IdAgentTest::ms_surfaceId = IdAgentTest::ms_defaultSurfaceId + 1;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_FAILED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             id_agent_module_init_success
 * @brief               Test case of id_agent_module_init() where weston_config_next_section() does mock, return an object
 *                      and set default behavior and valid surface id, appTitle, appId (surface id is in the range of the default value)
 * @test_procedure Steps:
 *                      -# Prepare mock for wl_list_init(), to init real list
 *                      -# Prepare mock for wl_list_insert(), to insert real object
 *                      -# Prepare mock for  wl_list_empty(), to empty real list
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
 */
TEST_F(IdAgentTest, id_agent_module_init_success)
{
    wl_list_init_fake.custom_fake = custom_wl_list_init;
    wl_list_insert_fake.custom_fake = custom_wl_list_insert;
    wl_list_empty_fake.custom_fake = custom_wl_list_empty;

    int (*weston_config_next_section_fakes[])(struct weston_config *, struct weston_config_section **, const char **) = {
        custom_weston_config_next_section_1,
        custom_weston_config_next_section_2
    };
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&mp_fakePointer, 1);
    SET_RETURN_SEQ(weston_config_get_section, (struct weston_config_section **)&mp_fakePointer, 1);
    SET_CUSTOM_FAKE_SEQ(weston_config_next_section, weston_config_next_section_fakes, 2);

    IdAgentTest::ms_defaultSurfaceId = 100;
    IdAgentTest::ms_defaultSurfaceIdMax = 200;
    IdAgentTest::ms_surfaceId = IdAgentTest::ms_defaultSurfaceId - 1;
    IdAgentTest::ms_appId = (char*)"0";
    IdAgentTest::ms_appTitle = (char*)"app_0";
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;

    ASSERT_EQ(id_agent_module_init(&m_westonCompositor, &g_iviLayoutInterfaceFake), IVI_SUCCEEDED);

    ASSERT_EQ(wet_get_config_fake.call_count, 1);
    ASSERT_EQ(weston_config_get_section_fake.call_count, 1);
    ASSERT_EQ(weston_config_next_section_fake.call_count, 2);
    ASSERT_EQ(weston_config_section_get_uint_fake.call_count, 3);
    ASSERT_EQ(weston_config_section_get_string_fake.call_count, 2);
}
