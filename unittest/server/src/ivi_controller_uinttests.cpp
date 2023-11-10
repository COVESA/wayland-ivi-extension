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
#include <dlfcn.h>
#include <string>
#include "server_api_fake.h"
#include "ivi_layout_interface_fake.h"
#include "ivi_layout_structure.hpp"

enum {
    WESTON_BUFFER_SHM,
    WESTON_BUFFER_DMABUF,
    WESTON_BUFFER_RENDERER_OPAQUE,
    WESTON_BUFFER_SOLID,
} type;

extern "C"
{
#include "ivi-controller.c"
}

const struct wl_interface wl_buffer_interface, wl_output_interface;
static constexpr uint8_t OBJECT_NUMBER = 2;
static uint32_t g_SurfaceCreatedCount = 0;

struct wl_resource * custom_wl_resource_from_link(struct wl_list *link)
{
	struct wl_resource *resource;

	return wl_container_of(link, resource, link);
}

struct wl_list * custom_wl_resource_get_link(struct wl_resource *resource)
{
	return &resource->link;
}

static uint32_t custom_get_id_of_surface(struct ivi_layout_surface *ivisurf)
{
    return ivisurf->id_surface;
}

static uint32_t custom_get_id_of_layer(struct ivi_layout_layer *ivilayer)
{
    return ivilayer->id_layer;
}

static int32_t custom_get_layers_on_screen(struct weston_output *output, int32_t *pLength,
					                       struct ivi_layout_layer ***ppArray)
{
    struct ivi_layout_layer ivilayer;
    ivilayer.id_layer = 1;

    *pLength = 1;
    *ppArray = (ivi_layout_layer**)malloc(sizeof(struct ivi_layout_layer *));
    (*ppArray)[0] = &ivilayer;

    return 0;
}

static int32_t custom_get_surfaces_on_layer(struct ivi_layout_layer *ivilayer, int32_t *pLength,
				                            struct ivi_layout_surface ***ppArray)
{
    struct ivi_layout_surface ivisurface;
    ivisurface.id_surface = 1;

    *pLength = 1;
    *ppArray = (ivi_layout_surface**)malloc(sizeof(struct ivi_layout_surface *));
    (*ppArray)[0] = &ivisurface;

    return 0;
}

static int32_t custom_surface_get_size(struct ivi_layout_surface *ivisurf, int32_t *width, int32_t *height,
			                           int32_t *stride)
{
    *width = 1;
    *height = 1;
    *stride = 1;

    return 0;
}

static int custom_read_pixels(struct weston_output *output,
			                  pixman_format_code_t format, void *pixels,
			                  uint32_t x, uint32_t y,
			                  uint32_t width, uint32_t height)
{
    return x == 0 || y == 0;
}

static void surface_create_callback(struct wl_listener *listener, void *data)
{
    g_SurfaceCreatedCount++;
}

class ControllerTests : public ::testing::Test
{
public:
    void SetUp()
    {
        IVI_LAYOUT_FAKE_LIST(RESET_FAKE);
        SERVER_API_FAKE_LIST(RESET_FAKE);
        get_id_of_surface_fake.custom_fake = custom_get_id_of_surface;
        get_id_of_layer_fake.custom_fake = custom_get_id_of_layer;
        get_layers_on_screen_fake.custom_fake = custom_get_layers_on_screen;
        get_surfaces_on_layer_fake.custom_fake = custom_get_surfaces_on_layer;
        surface_get_size_fake.custom_fake = custom_surface_get_size;
        g_SurfaceCreatedCount = 0;
        init_controller_content();
    }

    void TearDown()
    {
        deinit_controller_content();
    }

    void init_controller_content()
    {
        uint8_t l_fakeResource = 0, l_fakeClient = 0;
        mp_iviShell = (struct ivishell *)malloc(sizeof(struct ivishell));
        mp_iviShell->interface = &g_iviLayoutInterfaceFake;
        mp_iviShell->compositor = &m_westonCompositor;
        mp_iviShell->bkgnd_surface_id = 1;
        mp_iviShell->bkgnd_surface = nullptr;
        mp_iviShell->bkgnd_view = nullptr;
        mp_iviShell->client = nullptr;
        mp_iviShell->screen_id_offset = 1000;
        custom_wl_array_init(&mp_iviShell->screen_ids);
        mp_screenInfo = (struct screen_id_info *)custom_wl_array_add(&mp_iviShell->screen_ids, sizeof(struct screen_id_info));
        mp_screenInfo->screen_name = (char *)"default_screen";
        mp_screenInfo->screen_id = 0;

        custom_wl_list_init(&m_westonCompositor.output_list);
        custom_wl_list_init(&mp_iviShell->list_surface);
        custom_wl_list_init(&mp_iviShell->list_layer);
        custom_wl_list_init(&mp_iviShell->list_screen);
        custom_wl_list_init(&mp_iviShell->list_controller);
        custom_wl_list_init(&mp_iviShell->ivisurface_created_signal.listener_list);
        custom_wl_list_init(&mp_iviShell->ivisurface_removed_signal.listener_list);
        custom_wl_list_init(&mp_iviShell->id_allocation_request_signal.listener_list);

        for(uint8_t i = 0; i < OBJECT_NUMBER; i++)
        {
            // Prepare layout surfaces prperties
            m_layoutSurface[i].id_surface = i + 10;
            m_layoutSurfaceProperties[i].opacity = 1000 + i*100;
            m_layoutSurfaceProperties[i].source_x = 0;
            m_layoutSurfaceProperties[i].source_y = 0;
            m_layoutSurfaceProperties[i].source_width = 100 + i*10;
            m_layoutSurfaceProperties[i].source_height = 100 + i*10;
            m_layoutSurfaceProperties[i].start_x = 0;
            m_layoutSurfaceProperties[i].start_y = 0;
            m_layoutSurfaceProperties[i].start_width = 50 + i*20;
            m_layoutSurfaceProperties[i].start_height = 50 + i*20;
            m_layoutSurfaceProperties[i].dest_x = 0;
            m_layoutSurfaceProperties[i].dest_y = 0;
            m_layoutSurfaceProperties[i].dest_width = 300 + i*5;
            m_layoutSurfaceProperties[i].dest_height = 300 + i*5;
            m_layoutSurfaceProperties[i].orientation = WL_OUTPUT_TRANSFORM_NORMAL;
            m_layoutSurfaceProperties[i].visibility = true;
            m_layoutSurfaceProperties[i].transition_type = 1;
            m_layoutSurfaceProperties[i].transition_duration = 20;
            m_layoutSurfaceProperties[i].event_mask = 0xFFFFFFFF;

            // Prepare the ivi surface
            mp_iviSurface[i] = (struct ivisurface*)malloc(sizeof(struct ivisurface));
            mp_iviSurface[i]->shell = mp_iviShell;
            mp_iviSurface[i]->layout_surface = &m_layoutSurface[i];
            mp_iviSurface[i]->prop = &m_layoutSurfaceProperties[i];
            mp_iviSurface[i]->type = IVI_WM_SURFACE_TYPE_DESKTOP;
            custom_wl_list_insert(&mp_iviShell->list_surface, &mp_iviSurface[i]->link);

            // the client callback, it listen the change from specific ivi surface id
            mp_surfaceNotification[i] = (struct notification*)malloc(sizeof(struct notification));
            mp_surfaceNotification[i]->resource = (struct wl_resource*)((uintptr_t)mp_wlResourceDefault + i);
            custom_wl_list_init(&mp_iviSurface[i]->notification_list);
            custom_wl_list_insert(&mp_iviSurface[i]->notification_list, &mp_surfaceNotification[i]->layout_link);

            // Prepare layout layer properties
            m_layoutLayer[i].id_layer = i + 100;
            m_layoutLayerProperties[i].opacity = 1000 + i*100;
            m_layoutLayerProperties[i].source_x = 0;
            m_layoutLayerProperties[i].source_y = 0;
            m_layoutLayerProperties[i].source_width = 100 + i*10;
            m_layoutLayerProperties[i].source_height = 100 + i*10;
            m_layoutLayerProperties[i].dest_x = 0;
            m_layoutLayerProperties[i].dest_y = 0;
            m_layoutLayerProperties[i].dest_width = 300 + i*5;
            m_layoutLayerProperties[i].dest_height = 300 + i*5;
            m_layoutLayerProperties[i].orientation = WL_OUTPUT_TRANSFORM_NORMAL;
            m_layoutLayerProperties[i].visibility = true;
            m_layoutLayerProperties[i].transition_type = 1;
            m_layoutLayerProperties[i].transition_duration = 20;
            m_layoutLayerProperties[i].start_alpha = 0.1;
            m_layoutLayerProperties[i].end_alpha = 10.0;
            m_layoutLayerProperties[i].is_fade_in = 1 ;
            m_layoutLayerProperties[i].event_mask = 0 ;

            // Prepare the ivi layer
            mp_iviLayer[i] = (struct ivilayer *)malloc(sizeof(struct ivilayer));
            mp_iviLayer[i]->shell = mp_iviShell;
            mp_iviLayer[i]->layout_layer = &m_layoutLayer[i];
            mp_iviLayer[i]->prop = &m_layoutLayerProperties[i];
            custom_wl_list_insert(&mp_iviShell->list_layer, &mp_iviLayer[i]->link);

            // The client callback, it lusten the change from specific ivi layer id
            mp_LayerNotification[i] = (struct notification *)malloc(sizeof(struct notification));
            mp_LayerNotification[i]->resource = (struct wl_resource*)((uintptr_t)mp_wlResourceDefault + i);
            custom_wl_list_init(&mp_iviLayer[i]->notification_list);
            custom_wl_list_insert(&mp_iviLayer[i]->notification_list, &mp_LayerNotification[i]->layout_link);

            // Prepare the ivi screens
            m_westonOutput[i].id = i + 1000;
            mp_iviScreen[i] = (struct iviscreen*)malloc(sizeof(struct iviscreen));
            mp_iviScreen[i]->shell = mp_iviShell;
            mp_iviScreen[i]->output = &m_westonOutput[i];
            mp_iviScreen[i]->id_screen = m_westonOutput[i].id;
            custom_wl_list_init(&mp_iviScreen[i]->resource_list);
            custom_wl_list_insert(&mp_iviShell->list_screen, &mp_iviScreen[i]->link);

            // Prepare sample controllers
            mp_iviController[i] = (struct ivicontroller*)malloc(sizeof(struct ivicontroller));
            mp_iviController[i]->resource = (struct wl_resource*)&l_fakeResource;
            mp_iviController[i]->client = (struct wl_client*)&l_fakeClient;
            mp_iviController[i]->shell = mp_iviShell;
            mp_iviController[i]->id = i;
            custom_wl_list_insert(&mp_iviShell->list_controller, &mp_iviController[i]->link);

            // Prepare handler for new surfaces
            m_listenSurface[i].notify = surface_create_callback;
            custom_wl_list_insert(mp_iviShell->ivisurface_created_signal.listener_list.prev, &m_listenSurface[i].link);
        }
    }

    void deinit_controller_content()
    {
        for(uint8_t i = 0; i < OBJECT_NUMBER; i++)
        {
            free(mp_surfaceNotification[i]);
            free(mp_iviSurface[i]);
            free(mp_LayerNotification[i]);
            free(mp_iviLayer[i]);
            free(mp_iviScreen[i]);
            free(mp_iviController[i]);
        }
        if(mp_iviShell)
        {
            custom_wl_array_release(&mp_iviShell->screen_ids);
            free(mp_iviShell);
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
        wl_signal_init(&m_westonCompositor.output_created_signal);
        wl_signal_init(&m_westonCompositor.output_destroyed_signal);
        wl_signal_init(&m_westonCompositor.output_resized_signal);
        wl_signal_init(&m_westonCompositor.destroy_signal);
    }

    struct wl_resource *mp_wlResourceDefault = (struct wl_resource *)0xFFFFFF00;
    struct weston_buffer *mp_westonBuffer = (struct weston_buffer *)0xFFFFFF00;

    struct wl_listener m_listenSurface[OBJECT_NUMBER] = {};
    struct ivisurface *mp_iviSurface[OBJECT_NUMBER] = {nullptr};
    struct ivi_layout_surface m_layoutSurface[OBJECT_NUMBER] = {};
    struct ivi_layout_surface_properties m_layoutSurfaceProperties[OBJECT_NUMBER] = {};
    struct notification *mp_surfaceNotification[OBJECT_NUMBER] = {nullptr};

    struct ivilayer *mp_iviLayer[OBJECT_NUMBER] = {nullptr};
    struct ivi_layout_layer m_layoutLayer[OBJECT_NUMBER] = {};
    struct ivi_layout_layer_properties m_layoutLayerProperties[OBJECT_NUMBER] = {};
    struct notification *mp_LayerNotification[OBJECT_NUMBER] = {nullptr};

    struct iviscreen *mp_iviScreen[OBJECT_NUMBER] = {nullptr};
    struct weston_output m_westonOutput[OBJECT_NUMBER] = {};

    struct ivicontroller *mp_iviController[OBJECT_NUMBER] = {nullptr};

    struct ivishell *mp_iviShell = nullptr;
    struct weston_compositor m_westonCompositor = {};

    struct screen_id_info *mp_screenInfo = nullptr;

    struct wl_client *mp_fakeClient = nullptr;

    static int ms_idAgentModuleValue;
    static int ms_inputControllerModuleValue;
    static uint32_t ms_screenIdOffset;
    static uint32_t ms_screenId;
    static char *ms_iviClientName;
    static char *ms_debugScopes;
    static char *ms_screenName;
    static int32_t ms_bkgndSurfaceId;
    static uint32_t ms_bkgndColor;
    static bool ms_enableCursor;
    static uint32_t ms_westonConfigNextSection;

    int mp_successResult[1] = {0};
    int mp_failureResult[1] = {-1};
};

int ControllerTests::ms_idAgentModuleValue = 0;
int ControllerTests::ms_inputControllerModuleValue = 0;
uint32_t ControllerTests::ms_screenIdOffset = 0;
uint32_t ControllerTests::ms_screenId = 0;
char *ControllerTests::ms_iviClientName = nullptr;
char *ControllerTests::ms_debugScopes = nullptr;
char *ControllerTests::ms_screenName = nullptr;
int32_t ControllerTests::ms_bkgndSurfaceId = 0;
uint32_t ControllerTests::ms_bkgndColor = 0;
bool ControllerTests::ms_enableCursor = false;
uint32_t ControllerTests::ms_westonConfigNextSection = 0;

static int custom_id_agent_module_init(struct weston_compositor *compositor, const struct ivi_layout_interface *interface)
{
    return ControllerTests::ms_idAgentModuleValue;
}
static int custom_input_controller_module_init(struct ivishell *shell)
{
    return ControllerTests::ms_inputControllerModuleValue;
}

static int custom_weston_config_section_get_uint(struct weston_config_section *section, const char *key, uint32_t *value, uint32_t default_value)
{
    if(strcmp(key, "screen-id-offset") == 0)
    {
        *value = ControllerTests::ms_screenIdOffset;
    }
    else if(strcmp(key, "screen-id") == 0)
    {
        *value = ControllerTests::ms_screenId;
    }
    return 0;
}

static int custom_weston_config_section_get_string(struct weston_config_section *section, const char *key, char **value, const char *default_value)
{
    if(strcmp(key, "ivi-client-name") == 0)
    {
        *value = (ControllerTests::ms_iviClientName == nullptr) ? nullptr : strdup(ControllerTests::ms_iviClientName);
    }
    else if(strcmp(key, "debug-scopes") == 0)
    {
        *value = (ControllerTests::ms_debugScopes == nullptr) ? nullptr : strdup(ControllerTests::ms_debugScopes);
    }
    else if(strcmp(key, "screen-name") == 0)
    {
        *value = (ControllerTests::ms_screenName == nullptr) ? nullptr : strdup(ControllerTests::ms_screenName);
    }
    return 0;
}

static int custom_weston_config_section_get_int(struct weston_config_section *section, const char *key, int32_t *value, int32_t default_value)
{
    if(strcmp(key, "bkgnd-surface-id") == 0)
    {
        *value = ControllerTests::ms_bkgndSurfaceId;
    }
    return 0;
}

static int custom_weston_config_section_get_color(struct weston_config_section *section, const char *key, uint32_t *color, uint32_t default_color)
{
    if(strcmp(key, "bkgnd-color") == 0)
    {
        *color = ControllerTests::ms_bkgndColor;
    }
    return 0;
}

static int custom_weston_config_section_get_bool(struct weston_config_section *section, const char *key, bool *value, bool default_value)
{
    if(strcmp(key, "enable-cursor") == 0)
    {
        *value = ControllerTests::ms_enableCursor;
    }
    return 0;
}

static int custom_weston_config_next_section(struct weston_config *config, struct weston_config_section **section, const char **name)
{
    switch (ControllerTests::ms_westonConfigNextSection)
    {
    case 0:
    {
        *name = "ivi-screen";
        ControllerTests::ms_westonConfigNextSection ++;
        return 1;
    }
    case 1:
    {
        *name = nullptr;
    }
    default:
        break;
    }
    return 0;
}

/** ================================================================================================
 * @test_id             send_surface_prop_noEvents
 * @brief               Test case of send_surface_prop() where event mask of ivi surface is 0
 * @test_procedure Steps:
 *                      -# Set event mask of ivi surface is 0
 *                      -# Calling the send_surface_prop()
 *                      -# Verification point:
 *                         +# get_id_of_surface() must be called once time
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, send_surface_prop_noEvents)
{
    m_layoutSurfaceProperties[0].event_mask = 0;

    send_surface_prop(&mp_iviSurface[0]->property_changed, nullptr);

    ASSERT_EQ(get_id_of_surface_fake.call_count, 1);
    ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             send_surface_prop_hasEvents
 * @brief               Test case of send_surface_prop() where event mask of ivi surface is the change of all properties
 * @test_procedure Steps:
 *                      -# Set event mask of ivi surface is the change of all properties
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the send_surface_prop()
 *                      -# Verification point:
 *                         +# get_id_of_surface() must be called once time
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# wl_resource_post_event() must be called 5 times
 */
TEST_F(ControllerTests, send_surface_prop_hasEvents)
{
    m_layoutSurfaceProperties[0].event_mask = IVI_NOTIFICATION_OPACITY | IVI_NOTIFICATION_SOURCE_RECT |
                                   IVI_NOTIFICATION_DEST_RECT | IVI_NOTIFICATION_VISIBILITY|
                                   IVI_NOTIFICATION_CONFIGURE;

    struct weston_surface l_westonSurface;
    l_westonSurface.width = 1920;
    l_westonSurface.height = 1080;
    struct weston_surface *lp_westonSurface = &l_westonSurface;
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westonSurface, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    send_surface_prop(&mp_iviSurface[0]->property_changed, nullptr);

    ASSERT_EQ(get_id_of_surface_fake.call_count, 1);
    ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 5);
}

/** ================================================================================================
 * @test_id             send_layer_prop_noEvents
 * @brief               Test case of send_layer_prop() where event mask of ivi layer is 0
 * @test_procedure Steps:
 *                      -# Set event mask of ivi layer is 0
 *                      -# Calling the send_layer_prop()
 *                      -# Verification point:
 *                         +# get_id_of_layer() must be called once time
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, send_layer_prop_noEvents)
{
    m_layoutLayerProperties[0].event_mask = 0;

    send_layer_prop(&mp_iviLayer[0]->property_changed, nullptr);

    ASSERT_EQ(get_id_of_layer_fake.call_count, 1);
    ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             send_layer_prop_hasEvents
 * @brief               Test case of send_layer_prop() where event mask of ivi layer is the change of all properties
 * @test_procedure Steps:
 *                      -# Set event mask of ivi layer is the change of all properties
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the send_layer_prop()
 *                      -# Verification point:
 *                         +# get_id_of_layer() must be called once time
 *                         +# wl_resource_get_user_data() must be called once time
 *                         +# wl_resource_post_event() must be called 4 times
 */
TEST_F(ControllerTests, send_layer_prop_hasEvents)
{
    m_layoutLayerProperties[0].event_mask = IVI_NOTIFICATION_OPACITY | IVI_NOTIFICATION_SOURCE_RECT |
                                            IVI_NOTIFICATION_DEST_RECT | IVI_NOTIFICATION_VISIBILITY;

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    send_layer_prop(&mp_iviLayer[0]->property_changed, nullptr);

    ASSERT_EQ(get_id_of_layer_fake.call_count, 1);
    ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 4);
}

/** ================================================================================================
 * @test_id             surface_event_create_idDifferentBkgnSurfaceId
 * @brief               Test case of surface_event_create() where id_surface different bkgnd_surface_id
 * @test_procedure Steps:
 *                      -# Set id_surface and bkgnd_surface_id
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the surface_event_create()
 *                      -# Verification point:
 *                         +# get_id_of_surface() must be called once time
 *                         +# get_properties_of_surface() must be called once time
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# surface_add_listener() must be called once time
 *                         +# wl_resource_post_event() must be called {OBJECT_NUMBER} times with opcode IVI_WM_SURFACE_CREATED
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_create_idDifferentBkgnSurfaceId)
{
    struct ivi_layout_surface l_layoutSurface;
    l_layoutSurface.id_surface = 2;
    mp_iviShell->bkgnd_surface_id = 1;

    struct weston_surface *lp_westonSurface = (struct weston_surface *)malloc(sizeof(struct weston_surface));
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westonSurface, 1);

    surface_event_create(&mp_iviShell->surface_created, &l_layoutSurface);

    EXPECT_EQ(get_id_of_surface_fake.call_count, 1);
    EXPECT_EQ(get_properties_of_surface_fake.call_count, 1);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(surface_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, OBJECT_NUMBER);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_CREATED);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_history[1], IVI_WM_SURFACE_CREATED);
    EXPECT_EQ(OBJECT_NUMBER, g_SurfaceCreatedCount);
    struct ivisurface *lp_iviSurf = (struct ivisurface*)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivisurface, notification_list));
    EXPECT_EQ(lp_iviSurf->shell, mp_iviShell);
    EXPECT_EQ(lp_iviSurf->layout_surface, &l_layoutSurface);
    EXPECT_EQ(mp_iviShell->bkgnd_surface, nullptr);

    free(lp_iviSurf);
    free(lp_westonSurface);
}

/** ================================================================================================
 * @test_id             surface_event_create_EventCreateADesktopSurface
 * @brief               surface_event_create() is called when a desktop surface is created
 * @test_procedure Steps:
 *                      -# Set id_surface and bkgnd_surface_id
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Mocking the get_id_of_surface() does return an invalid ID in first call, an valid ID in second call
 *                      -# Mocking the weston_surface_get_desktop_surface() does return an object
 *                      -# Calling the surface_event_create()
 *                      -# Verification point:
 *                         +# get_id_of_surface() must be called 2 times
 *                         +# get_properties_of_surface() must be called once time
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# surface_add_listener() must be called once time
 *                         +# wl_resource_post_event() must be called {OBJECT_NUMBER} times with opcode IVI_WM_SURFACE_CREATED
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_create_EventCreateADesktopSurface)
{
    struct ivi_layout_surface l_layoutSurface;
    l_layoutSurface.id_surface = IVI_INVALID_ID;
    mp_iviShell->bkgnd_surface_id = 1;

    struct weston_surface *lp_westonSurface = (struct weston_surface *)malloc(sizeof(struct weston_surface));
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westonSurface, 1);
    uint32_t IDSurfaceRetList[2] = {IVI_INVALID_ID, 1};
    SET_RETURN_SEQ(get_id_of_surface, IDSurfaceRetList, 2);
    struct weston_desktop_surface *lp_westonDesktopSurface = (struct weston_desktop_surface*) 0xFFFFFFFF; // a fake valid address
    SET_RETURN_SEQ(weston_surface_get_desktop_surface, &lp_westonDesktopSurface, 1);

    surface_event_create(&mp_iviShell->surface_created, &l_layoutSurface);

    EXPECT_EQ(get_id_of_surface_fake.call_count, 2);
    EXPECT_EQ(get_properties_of_surface_fake.call_count, 1);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 2);
    EXPECT_EQ(surface_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, OBJECT_NUMBER);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_CREATED);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_history[1], IVI_WM_SURFACE_CREATED);
    EXPECT_EQ(OBJECT_NUMBER, g_SurfaceCreatedCount);
    struct ivisurface *lp_iviSurf = (struct ivisurface*)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivisurface, notification_list));
    EXPECT_EQ(lp_iviSurf->shell, mp_iviShell);
    EXPECT_EQ(lp_iviSurf->layout_surface, &l_layoutSurface);
    EXPECT_EQ(mp_iviShell->bkgnd_surface, nullptr);

    free(lp_iviSurf);
    free(lp_westonSurface);
}

/** ================================================================================================
 * @test_id             surface_event_create_idSameBkgnSurfaceId
 * @brief               Test case of surface_event_create() where id_surface same bkgnd_surface_id
 * @test_procedure Steps:
 *                      -# Set id_surface and bkgnd_surface_id
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the surface_event_create()
 *                      -# Verification point:
 *                         +# get_id_of_surface() must be called once time
 *                         +# get_properties_of_surface() must be called once time
 *                         +# surface_get_weston_surface() must be called once time
 *                         +# surface_add_listener() not be called
 *                         +# wl_resource_post_event() not be called
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_create_idSameBkgnSurfaceId)
{
    struct ivi_layout_surface l_layoutSurface;
    l_layoutSurface.id_surface = 1;
    mp_iviShell->bkgnd_surface_id = 1;

    struct weston_surface *lp_westonSurface = (struct weston_surface *)malloc(sizeof(struct weston_surface));
    SET_RETURN_SEQ(surface_get_weston_surface, &lp_westonSurface, 1);

    surface_event_create(&mp_iviShell->surface_created, &l_layoutSurface);

    EXPECT_EQ(get_id_of_surface_fake.call_count, 1);
    EXPECT_EQ(get_properties_of_surface_fake.call_count, 1);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(surface_add_listener_fake.call_count, 0);
    EXPECT_EQ(0, g_SurfaceCreatedCount);
    struct ivisurface *lp_iviSurf = (struct ivisurface*)(uintptr_t(wl_list_init_fake.arg0_history[0]) - offsetof(struct ivisurface, notification_list));
    EXPECT_EQ(lp_iviSurf->shell, mp_iviShell);
    EXPECT_EQ(lp_iviSurf->layout_surface, &l_layoutSurface);
    EXPECT_EQ(mp_iviShell->bkgnd_surface, lp_iviSurf);

    free(lp_iviSurf);
    free(lp_westonSurface);
}

/** ================================================================================================
 * @test_id             layer_event_create
 * @brief               Test case of layer_event_create() where valid input params
 * @test_procedure Steps:
 *                      -# Set id layer of layout layer
 *                      -# Calling the layer_event_create()
 *                      -# Verification point:
 *                         +# get_id_of_layer() must be called once time
 *                         +# get_properties_of_layer() must be called once time
 *                         +# layer_add_listener() must be called once time
 *                         +# wl_resource_post_event() must be called {OBJECT_NUMBER} times
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, layer_event_create)
{
    struct ivi_layout_layer l_layoutLayer;
    l_layoutLayer.id_layer = 10;

    layer_event_create(&mp_iviShell->layer_created, &l_layoutLayer);

    EXPECT_EQ(get_id_of_layer_fake.call_count, 1);
    EXPECT_EQ(get_properties_of_layer_fake.call_count, 1);
    EXPECT_EQ(layer_add_listener_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, OBJECT_NUMBER);
    struct ivilayer *lp_iviLayer = (struct ivilayer*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct ivilayer, link));
    EXPECT_EQ(lp_iviLayer->shell, mp_iviShell);
    EXPECT_EQ(lp_iviLayer->layout_layer, &l_layoutLayer);

    free(lp_iviLayer);
}

/** ================================================================================================
 * @test_id             output_created_event_customScreen
 * @brief               Test case of output_created_event() where weston output is a custom name
 *                      different screen info default
 * @test_procedure Steps:
 *                      -# Set weston output with name is "custom_screen" and id is 1
 *                      -# Calling the output_created_event()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called once time
 *                         +# wl_list_init() must be called once time
 *                         +# weston_compositor_schedule_repaint() must be called once time
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, output_created_event_customScreen)
{
    struct weston_output l_createdOutput;
    l_createdOutput.id = 1;
    l_createdOutput.name = (char*)"custom_screen";

    output_created_event(&mp_iviShell->output_created, &l_createdOutput);

    EXPECT_EQ(wl_list_insert_fake.call_count, 1);
    EXPECT_EQ(wl_list_init_fake.call_count, 1);
    EXPECT_EQ(weston_compositor_schedule_repaint_fake.call_count, 1);
    struct iviscreen *lp_iviScreen = (struct iviscreen*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct iviscreen, link));
    EXPECT_EQ(lp_iviScreen->shell, mp_iviShell);
    EXPECT_EQ(lp_iviScreen->output, &l_createdOutput);
    EXPECT_EQ(lp_iviScreen->id_screen, mp_iviShell->screen_id_offset + l_createdOutput.id);

    free(lp_iviScreen);
}

/** ================================================================================================
 * @test_id             output_created_event_defaultConfigScreen
 * @brief               Test case of output_created_event() where weston output is a default name
 *                      same screen info default
 * @test_procedure Steps:
 *                      -# Set weston output with name is default name and id is 1000
 *                      -# Calling the output_created_event()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called once time
 *                         +# wl_list_init() must be called once time
 *                         +# weston_compositor_schedule_repaint() must be called once time
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, output_created_event_defaultConfigScreen)
{
    struct weston_output l_createdOutput;
    l_createdOutput.id = 1000;
    l_createdOutput.name = mp_screenInfo->screen_name;

    output_created_event(&mp_iviShell->output_created, &l_createdOutput);

    EXPECT_EQ(wl_list_insert_fake.call_count, 1);
    EXPECT_EQ(wl_list_init_fake.call_count, 1);
    EXPECT_EQ(weston_compositor_schedule_repaint_fake.call_count, 1);
    struct iviscreen *lp_iviScreen = (struct iviscreen*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct iviscreen, link));
    EXPECT_EQ(lp_iviScreen->shell, mp_iviShell);
    EXPECT_EQ(lp_iviScreen->output, &l_createdOutput);
    EXPECT_EQ(lp_iviScreen->id_screen, mp_screenInfo->screen_id);

    free(lp_iviScreen);
}

/** ================================================================================================
 * @test_id             output_created_event_whenBackgroundViewAndClientAvailable
 * @brief               Test case of output_created_event() where weston output is a default name
 *                      same screen info default, background view and client of shell are available
 * @test_procedure Steps:
 *                      -# Set weston output with name is default name and id is 1000
 *                      -# Set data for ivi shell with valid bkgnd_view and client
 *                      -# Calling the output_created_event()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 2 times
 *                         +# wl_list_init() must be called once time
 *                         +# wl_list_remove() must be called once time
 *                         +# weston_compositor_schedule_repaint() should not be called
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, output_created_event_whenBackgroundViewAndClientAvailable)
{
    struct weston_output l_createdOutput;
    l_createdOutput.id = 1000;
    l_createdOutput.name = mp_screenInfo->screen_name;

    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));
    mp_iviShell->bkgnd_view->surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    mp_iviShell->bkgnd_view->surface->width = 1;
    mp_iviShell->bkgnd_view->surface->height = 1;
    mp_iviShell->bkgnd_view->surface->compositor = (struct weston_compositor*)malloc(sizeof(weston_compositor));
    custom_wl_list_init(&mp_iviShell->bkgnd_view->surface->compositor->output_list);
    struct weston_output *l_output = (struct weston_output *)malloc(sizeof(struct weston_output));
    l_output->name = (char*)"default_screen";
    l_output->id = 1;
    l_output->x = 1;
    l_output->y = 1;
    l_output->width = 1;
    l_output->height = 1;
    custom_wl_list_insert(&mp_iviShell->compositor->output_list, &l_output->link);
    mp_iviShell->client = (struct wl_client*)&mp_fakeClient;

    output_created_event(&mp_iviShell->output_created, &l_createdOutput);

    EXPECT_EQ(wl_list_insert_fake.call_count, 2);
    EXPECT_EQ(wl_list_init_fake.call_count, 1);
    EXPECT_EQ(weston_compositor_schedule_repaint_fake.call_count, 0);
    struct iviscreen *lp_iviScreen = (struct iviscreen*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct iviscreen, link));
    EXPECT_EQ(lp_iviScreen->shell, mp_iviShell);
    EXPECT_EQ(lp_iviScreen->output, &l_createdOutput);
    EXPECT_EQ(lp_iviScreen->id_screen, mp_screenInfo->screen_id);
    EXPECT_EQ(wl_list_remove_fake.call_count, 1);

    free(lp_iviScreen);
    free(l_output);
    free(mp_iviShell->bkgnd_view->surface->compositor);
    free(mp_iviShell->bkgnd_view->surface);
    free(mp_iviShell->bkgnd_view);
}

/** ================================================================================================
 * @test_id             bind_ivi_controller_oldVersion
 * @brief               Call bind_ivi_controller() with version < 2
 * @test_procedure Steps:
 *                      -# Calling the bind_ivi_controller() with version < 2
 *                      -# Verification point:
 *                         +# wl_client_post_implementation_error() must be called once time
 */
TEST_F(ControllerTests, bind_ivi_controller_oldVersion)
{
    std::uint32_t lVersion{1};
    std::uint32_t lID{1};

    bind_ivi_controller(mp_fakeClient, mp_iviShell, lVersion, lID);

    ASSERT_EQ(wl_client_post_implementation_error_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             bind_ivi_controller_nullResource
 * @brief               Test case of bind_ivi_controller() where wl_resource_create() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the bind_ivi_controller()
 *                      -# Verification point:
 *                         +# wl_resource_create() must be called once time
 *                         +# wl_client_post_no_memory() must be called once time
 *                         +# wl_resource_set_implementation() not be called
 */
TEST_F(ControllerTests, bind_ivi_controller_nullResource)
{
    std::uint32_t lVersion{2};
    std::uint32_t lID{1};
    struct wl_resource *lResourceRetList[1] = {nullptr};

    SET_RETURN_SEQ(wl_resource_create, (struct wl_resource **)&lResourceRetList, 1);

    bind_ivi_controller(mp_fakeClient, mp_iviShell, lVersion, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(wl_client_post_no_memory_fake.call_count, 1);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             bind_ivi_controller_success
 * @brief               Test case of bind_ivi_controller() where wl_resource_create() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Calling the bind_ivi_controller()
 *                      -# Verification point:
 *                         +# wl_resource_create() must be called once time
 *                         +# wl_resource_set_implementation() must be called once time
 *                         +# get_id_of_surface() must be called {OBJECT_NUMBER} times
 *                         +# get_id_of_layer() must be called {OBJECT_NUMBER} times
 *                         +# wl_resource_post_event() must be called {OBJECT_NUMBER*2} times with valid opcode
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, bind_ivi_controller_success)
{
    std::uint32_t lVersion{2};
    std::uint32_t lID{1};
    SET_RETURN_SEQ(wl_resource_create, &mp_wlResourceDefault, 1);

    bind_ivi_controller(mp_fakeClient, mp_iviShell, lVersion, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 1);
    ASSERT_EQ(get_id_of_surface_fake.call_count, OBJECT_NUMBER);
    ASSERT_EQ(get_id_of_layer_fake.call_count, OBJECT_NUMBER);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, OBJECT_NUMBER*2);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_CREATED);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[1], IVI_WM_SURFACE_CREATED);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[2], IVI_WM_LAYER_CREATED);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[3], IVI_WM_LAYER_CREATED);
    struct ivicontroller *lp_iviController = (struct ivicontroller*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct ivicontroller, link));
    ASSERT_EQ(lp_iviController->shell, mp_iviShell);
    ASSERT_EQ(lp_iviController->client, mp_fakeClient);
    ASSERT_EQ(lp_iviController->id, lID);

    free(lp_iviController);
}

/** ================================================================================================
 * @test_id             controller_create_screen_wrongWestonHead
 * @brief               Test case of controller_create_screen() where wl_resource_get_user_data() fails, return wrong object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return wrong weston head
 *                      -# Calling the controller_create_screen()
 *                      -# Verification point:
 *                         +# wl_resource_create() not be called
 *                         +# wl_resource_set_implementation() not be called
 *                         +# wl_list_insert() not be called
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, controller_create_screen_wrongWestonHead)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lOutputResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lID{1};
    struct weston_head l_westonHead;
    struct weston_output *lp_fakeWestonOutput = (struct weston_output*)0xFFFFFFFF; // a fake valid address

    l_westonHead.output = lp_fakeWestonOutput;
    void *lpp_getUserData [] = {&l_westonHead, mp_iviController[0]};
    SET_RETURN_SEQ(wl_resource_get_user_data, lpp_getUserData, 2);

    controller_create_screen(lClient, lResource, lOutputResource, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 0);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 0);
    ASSERT_EQ(wl_list_insert_fake.call_count, 0);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_create_screen_nullScreenResource
 * @brief               Test case of controller_create_screen() where wl_resource_get_user_data() success, return an object
 *                      but wl_resource_create() fails, return null object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return valid weston head
 *                      -# Mocking the wl_resource_create() does return an valid object
 *                      -# Calling the controller_create_screen()
 *                      -# Verification point:
 *                         +# wl_resource_create() must be called once time
 *                         +# wl_resource_post_no_memory() must be called once time
 *                         +# wl_resource_set_implementation() not be called
 *                         +# wl_list_insert() not be called
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, controller_create_screen_nullScreenResource)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lOutputResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lID{1};
    struct weston_head l_westonHead;

    l_westonHead.output = &m_westonOutput[0];
    void *lpp_getUserData [] = {&l_westonHead, mp_iviController[0]};
    struct wl_resource *lResourceRetList[1] = {nullptr};
    SET_RETURN_SEQ(wl_resource_get_user_data, lpp_getUserData, 2);
    SET_RETURN_SEQ(wl_resource_create, (struct wl_resource **)&lResourceRetList, 1);

    controller_create_screen(lClient, lResource, lOutputResource, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_no_memory_fake.call_count, 1);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 0);
    ASSERT_EQ(wl_list_insert_fake.call_count, 0);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_create_screen_correctWestonHead
 * @brief               Test case of controller_create_screen() where wl_resource_get_user_data() success, return an object
 *                      and wl_resource_create() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return valid weston head
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Calling the controller_create_screen()
 *                      -# Verification point:
 *                         +# wl_resource_create() must be called once time
 *                         +# wl_resource_set_implementation() must be called once time
 *                         +# wl_list_insert() must be called once time
 *                         +# wl_resource_post_event() must be called 2 times
 */
TEST_F(ControllerTests, controller_create_screen_correctWestonHead)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lOutputResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lID{1};
    struct weston_head l_westonHead;

    l_westonHead.output = &m_westonOutput[0];
    void *lpp_getUserData [] = {&l_westonHead, mp_iviController[0]};
    SET_RETURN_SEQ(wl_resource_get_user_data, lpp_getUserData, 2);
    SET_RETURN_SEQ(wl_resource_create, &mp_wlResourceDefault, 1);

    controller_create_screen(lClient, lResource, lOutputResource, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 1);
    ASSERT_EQ(wl_list_insert_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 2);
}

/** ================================================================================================
 * @test_id             wet_module_init_cannotGetIviLayoutInterface
 * @brief               Test case of wet_module_init() where weston_plugin_api_get() fails, return null object
 * @test_procedure Steps:
 *                      -# Calling the wet_module_init()
 *                      -# Verification point:
 *                         +# wet_module_init() must not return 0
 *                         +# weston_plugin_api_get() must be called once time
 *                         +# wet_get_config() not be called
 */
TEST_F(ControllerTests, wet_module_init_cannotGetIviLayoutInterface)
{
    struct ivi_layout_interface *lPluginAPIRetList[1] = {nullptr};
    SET_RETURN_SEQ(weston_plugin_api_get, (struct ivi_layout_interface **)&lPluginAPIRetList, 1);
    ASSERT_NE(wet_module_init(&m_westonCompositor, nullptr, nullptr), 0);

    ASSERT_EQ(weston_plugin_api_get_fake.call_count, 1);
    ASSERT_EQ(wet_get_config_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             wet_module_init_cannotCreateGlobalWmIviInterface
 * @brief               Test case of wet_module_init() where weston_plugin_api_get() success, return an object
 *                      but wl_global_create() fails, return null object
 * @test_procedure Steps:
 *                      -# Mocking the weston_plugin_api_get() does return an object
 *                      -# Calling the wet_module_init()
 *                      -# Verification point:
 *                         +# wet_module_init() must not return 0
 *                         +# weston_plugin_api_get() must be called once time
 *                         +# wl_global_create() must be called once time
 *                         +# weston_load_module() not be called
 */
TEST_F(ControllerTests, wet_module_init_cannotCreateGlobalWmIviInterface)
{
    struct wl_global *lGlobalRetList[1] = {nullptr};
    struct ivi_layout_interface *lp_iviLayoutInterface = &g_iviLayoutInterfaceFake;
    SET_RETURN_SEQ(weston_plugin_api_get, (const void**)&lp_iviLayoutInterface, 1);
    SET_RETURN_SEQ(wl_global_create, (struct wl_global **)&lGlobalRetList, 1);

    ASSERT_NE(wet_module_init(&m_westonCompositor, nullptr, nullptr), 0);

    ASSERT_EQ(weston_plugin_api_get_fake.call_count, 1);
    ASSERT_EQ(wl_global_create_fake.call_count, 1);
    ASSERT_EQ(weston_load_module_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             wet_module_init_cannotInitIviInputModule
 * @brief               Test case of wet_module_init() where weston_plugin_api_get() success, return an object
 *                      and wl_global_create() success, return an object but cannot load input_module
 * @test_procedure Steps:
 *                      -# Set ms_inputControllerModuleValue is -1
 *                      -# Mocking the weston_plugin_api_get() does return an object
 *                      -# Mocking the wl_global_create() does return an object
 *                      -# Mocking the weston_load_module() does return an object
 *                      -# Calling the wet_module_init()
 *                      -# Verification point:
 *                         +# wet_module_init() must not return 0
 *                         +# weston_plugin_api_get() must be called once time
 *                         +# wl_global_create() must be called once time
 *                         +# weston_load_module() must be called once time
 */
TEST_F(ControllerTests, wet_module_init_cannotInitIviInputModule)
{
    ControllerTests::ms_inputControllerModuleValue = -1;

    struct ivi_layout_interface *lp_iviLayoutInterface = &g_iviLayoutInterfaceFake;
    SET_RETURN_SEQ(weston_plugin_api_get, (const void**)&lp_iviLayoutInterface, 1);
    struct wl_global *lp_wlGlobal = (struct wl_global*) 0xFFFFFFFF; // a fake valid address
    SET_RETURN_SEQ(wl_global_create, &lp_wlGlobal, 1);
    void *lp_iviInputInitModule = (void*)custom_input_controller_module_init;
    SET_RETURN_SEQ(weston_load_module, &lp_iviInputInitModule, 1);

    ASSERT_NE(wet_module_init(&m_westonCompositor, nullptr, nullptr), 0);

    ASSERT_EQ(weston_plugin_api_get_fake.call_count, 1);
    ASSERT_EQ(wl_global_create_fake.call_count, 1);
    ASSERT_EQ(weston_load_module_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             wet_module_init_cannotGetWestonConfig
 * @brief               Test case of wet_module_init() where weston_plugin_api_get() success, return an object
 *                      and wl_global_create() success, return an object and ms_inputControllerModuleValue is 0
 *                      but wet_get_config() return fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the enable_utility_funcs_of_array_list() to set real function for wl_list, wl_array
 *                      -# make load_input_module() is called successfully
 *                      -# make load_id_agent_module() is called failure
 *                      -# Mocking the weston_plugin_api_get() does return an object
 *                      -# Mocking the wl_global_create() does return an object
 *                      -# Mocking the weston_load_module() does return an object
 *                      -# Mocking the wet_get_config() does return an NULL pointer
 *                      -# Calling the wet_module_init()
 *                      -# Verification point:
 *                         +# wet_module_init() must return 0
 *                         +# weston_plugin_api_get() must be called once time
 *                         +# wl_global_create() must be called once time
 *                         +# weston_load_module() must be called 2 times
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, wet_module_init_cannotGetWestonConfig)
{
    enable_utility_funcs_of_array_list();

    ControllerTests::ms_inputControllerModuleValue = 0;
    ControllerTests::ms_idAgentModuleValue = -1;

    struct ivi_layout_interface *lp_iviLayoutInterface = &g_iviLayoutInterfaceFake;
    SET_RETURN_SEQ(weston_plugin_api_get, (const void**)&lp_iviLayoutInterface, 1);
    struct wl_global *lp_wlGlobal = (struct wl_global*) 0xFFFFFFFF; // a fake valid address
    SET_RETURN_SEQ(wl_global_create, &lp_wlGlobal, 1);
    void *lp_iviInputInitModule[] ={(void *)custom_input_controller_module_init, (void *)custom_id_agent_module_init};
    SET_RETURN_SEQ(weston_load_module, lp_iviInputInitModule, 2);
    struct weston_config *lConfigRetList[1] = {nullptr};
    SET_RETURN_SEQ(wet_get_config, (struct weston_config **)&lConfigRetList, 1);

    EXPECT_EQ(wet_module_init(&m_westonCompositor, nullptr, nullptr), 0);

    EXPECT_EQ(weston_plugin_api_get_fake.call_count, 1);
    EXPECT_EQ(wl_global_create_fake.call_count, 1);
    EXPECT_EQ(weston_load_module_fake.call_count, 2);

    struct ivishell *lp_iviShell = (struct ivishell*)wl_global_create_fake.arg3_history[0];
    EXPECT_EQ(lp_iviShell->interface, lp_iviLayoutInterface);
    EXPECT_EQ(lp_iviShell->screen_id_offset, 0);
    EXPECT_EQ(lp_iviShell->ivi_client_name, nullptr);
    EXPECT_EQ(lp_iviShell->bkgnd_surface_id, 0);
    EXPECT_EQ(lp_iviShell->debug_scopes, nullptr);
    EXPECT_EQ(lp_iviShell->bkgnd_color, 0);
    EXPECT_EQ(lp_iviShell->enable_cursor, 0);
    EXPECT_EQ(lp_iviShell->screen_ids.size, 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_surface), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_layer), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_screen), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_controller), 0);
    EXPECT_NE(lp_iviShell->layer_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->layer_removed.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_removed.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_configured.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_destroyed.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_resized.notify, nullptr);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_created_signal.listener_list), 1);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_destroyed_signal.listener_list), 1);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_resized_signal.listener_list), 1);

    free(lp_iviShell);
}

/** ================================================================================================
 * @test_id             wet_module_init_loadIdAgentModuleFailed
 * @brief               Test case of wet_module_init() where weston_plugin_api_get() success, return an object
 *                      and wl_global_create() success, return an object and ms_inputControllerModuleValue is 0
 *                      and wet_get_config() return success, return an pointer but cannot load id_agent_module
 * @test_procedure Steps:
 *                      -# Calling the enable_utility_funcs_of_array_list() to set real function for wl_list, wl_array
 *                      -# make load_input_module() is called successfully
 *                      -# make load_id_agent_module() is called failure
 *                      -# Mocking the weston_plugin_api_get() does return an object
 *                      -# Mocking the wl_global_create() does return an object
 *                      -# Mocking the wet_get_config() does return an object
 *                      -# Mocking the weston_config_get_section() does return an object
 *                      -# Mocking the weston_config_next_section() does return an object
 *                      -# Mocking the weston_load_module() does return an object
 *                      -# Set config data
 *                      -# Calling the wet_module_init()
 *                      -# Verification point:
 *                         +# wet_module_init() must return 0
 *                         +# weston_plugin_api_get() must be called once time
 *                         +# wl_global_create() must be called once time
 *                         +# weston_load_module() must be called 2 times
 *                         +# wet_get_config() must be called 3 times
 *                         +# weston_config_get_section() must be called 3 times
 *                         +# The result output should same with prepare data
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, wet_module_init_loadIdAgentModuleFailed)
{
    enable_utility_funcs_of_array_list();

    ControllerTests::ms_inputControllerModuleValue = 0;
    ControllerTests::ms_idAgentModuleValue = -1;

    struct ivi_layout_interface *lp_iviLayoutInterface = &g_iviLayoutInterfaceFake;
    struct wl_global *lp_wlGlobal = (struct wl_global*) 0xFFFFFFFF; // a fake valid address
    struct weston_config *lp_westonConfig = (struct weston_config*) 0xFFFFFFFF; // a fake valid address
    struct weston_config_section *lp_westonConfigSection = (struct weston_config_section*) 0xFFFFFFFF; // a fake valid address
    int lpp_westonConfigNextSection[] = {1, 0};
    void *lp_iviInputInitModule[] ={(void *)custom_input_controller_module_init, (void *)custom_id_agent_module_init};
    SET_RETURN_SEQ(weston_plugin_api_get, (const void**)&lp_iviLayoutInterface, 1);
    SET_RETURN_SEQ(wl_global_create, &lp_wlGlobal, 1);
    SET_RETURN_SEQ(wet_get_config, &lp_westonConfig, 1);
    SET_RETURN_SEQ(weston_config_get_section, &lp_westonConfigSection, 1);
    SET_RETURN_SEQ(weston_config_next_section, lpp_westonConfigNextSection, 2);
    SET_RETURN_SEQ(weston_load_module, lp_iviInputInitModule, 2);

    ControllerTests::ms_screenIdOffset = 100;
    ControllerTests::ms_screenId = 10;
    ControllerTests::ms_iviClientName = (char *)"ivi_client_app.elf";
    ControllerTests::ms_debugScopes = (char *)"controller";
    ControllerTests::ms_screenName = (char *)"screen_test";
    ControllerTests::ms_bkgndSurfaceId = 100;
    ControllerTests::ms_bkgndColor = 0xAABBCCDD; // a fake valid address
    ControllerTests::ms_enableCursor = true;
    ControllerTests::ms_westonConfigNextSection = 0;
    weston_config_section_get_uint_fake.custom_fake = custom_weston_config_section_get_uint;
    weston_config_section_get_string_fake.custom_fake = custom_weston_config_section_get_string;
    weston_config_section_get_int_fake.custom_fake = custom_weston_config_section_get_int;
    weston_config_section_get_color_fake.custom_fake = custom_weston_config_section_get_color;
    weston_config_section_get_bool_fake.custom_fake = custom_weston_config_section_get_bool;
    weston_config_next_section_fake.custom_fake = custom_weston_config_next_section;

    EXPECT_EQ(wet_module_init(&m_westonCompositor, nullptr, nullptr), 0);

    EXPECT_EQ(weston_plugin_api_get_fake.call_count, 1);
    EXPECT_EQ(wl_global_create_fake.call_count, 1);
    EXPECT_EQ(weston_load_module_fake.call_count, 2);
    EXPECT_EQ(wet_get_config_fake.call_count, 3);
    EXPECT_EQ(weston_config_get_section_fake.call_count, 3);

    struct ivishell *lp_iviShell = (struct ivishell*)wl_global_create_fake.arg3_history[0];
    EXPECT_EQ(lp_iviShell->interface, lp_iviLayoutInterface);
    EXPECT_EQ(lp_iviShell->screen_id_offset, ControllerTests::ms_screenIdOffset);
    EXPECT_EQ(strcmp(lp_iviShell->ivi_client_name, ControllerTests::ms_iviClientName), 0);
    EXPECT_EQ(lp_iviShell->bkgnd_surface_id, ControllerTests::ms_bkgndSurfaceId);
    EXPECT_EQ(strcmp(lp_iviShell->debug_scopes, ControllerTests::ms_debugScopes), 0);
    EXPECT_EQ(lp_iviShell->bkgnd_color, ControllerTests::ms_bkgndColor);
    EXPECT_EQ(lp_iviShell->enable_cursor, ControllerTests::ms_enableCursor);
    EXPECT_EQ(lp_iviShell->screen_ids.size, sizeof(struct screen_id_info));
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_surface), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_layer), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_screen), 0);
    EXPECT_EQ(custom_wl_list_length(&lp_iviShell->list_controller), 0);
    EXPECT_NE(lp_iviShell->layer_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->layer_removed.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_removed.notify, nullptr);
    EXPECT_NE(lp_iviShell->surface_configured.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_created.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_destroyed.notify, nullptr);
    EXPECT_NE(lp_iviShell->output_resized.notify, nullptr);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_created_signal.listener_list), 1);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_destroyed_signal.listener_list), 1);
    EXPECT_EQ(custom_wl_list_length(&m_westonCompositor.output_resized_signal.listener_list), 1);

    struct screen_id_info *screen_info = NULL;
    wl_array_for_each(screen_info, &lp_iviShell->screen_ids) {
        free(screen_info->screen_name);
    }
    wl_array_release(&lp_iviShell->screen_ids);
    free(lp_iviShell->ivi_client_name);
    free(lp_iviShell->debug_scopes);
    free(lp_iviShell);
}

/** ================================================================================================
 * @test_id             controller_screen_destroy_success
 * @brief               Test case of controller_screen_destroy() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the controller_screen_destroy()
 *                      -# Verification point:
 *                         +# wl_resource_destroy() must be called once time
 */
TEST_F(ControllerTests, controller_screen_destroy_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    controller_screen_destroy(lClient, lResource);
    ASSERT_EQ(wl_resource_destroy_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_screen_clear_cannotGetUserData
 * @brief               Test case of controller_screen_clear() where wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the controller_screen_clear()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SCREEN_ERROR
 */
TEST_F(ControllerTests, controller_screen_clear_cannotGetUserData)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    controller_screen_clear(lClient, lResource);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
}

/** ================================================================================================
 * @test_id             controller_screen_clear_success
 * @brief               Test case of controller_screen_clear() where wl_resource_get_user_data() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_clear()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 *                         +# screen_set_render_order() must be called once time
 */
TEST_F(ControllerTests, controller_screen_clear_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);

    controller_screen_clear(lClient, lResource);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(screen_set_render_order_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_screen_add_layer_nullScreen
 * @brief               Test case of controller_screen_add_layer() where wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the controller_screen_add_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time and opcode is IVI_WM_SCREEN_ERROR
 *                         +# get_layer_from_id() should not be called
 */
TEST_F(ControllerTests, controller_screen_add_layer_nullScreen)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct iviscreen *lIviScreenRetList[1] = {nullptr};
    uint32_t l_layer_id = 10;

    SET_RETURN_SEQ(wl_resource_get_user_data, (struct iviscreen **)&lIviScreenRetList, 1);
    controller_screen_add_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
    ASSERT_EQ(get_layer_from_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_add_layer_wrongLayerId
 * @brief               Test case of controller_screen_add_layer() where wl_resource_get_user_data() success, return an object
 *                      but get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_add_layer()
 *                      -# Verification point:
 *                         +# get_layer_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode is IVI_WM_SCREEN_ERROR
 *                         +# screen_add_layer() should not be called
 */
TEST_F(ControllerTests, controller_screen_add_layer_wrongLayerId)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    uint32_t l_layer_id = 10;
    controller_screen_add_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(get_layer_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
    ASSERT_EQ(screen_add_layer_fake.call_count, 0);

}

/** ================================================================================================
 * @test_id             controller_screen_add_layer_success
 * @brief               Test case of controller_screen_add_layer() where wl_resource_get_user_data() success, return an object
 *                      and get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_screen_add_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 *                         +# screen_add_layer() must be called once time
 */
TEST_F(ControllerTests, controller_screen_add_layer_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);

    uint32_t l_layer_id = 10;
    controller_screen_add_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(screen_add_layer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_screen_remove_layer_nullScreen
 * @brief               Test case of controller_screen_remove_layer() where wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the controller_screen_remove_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time and opcode is IVI_WM_SCREEN_ERROR
 *                         +# get_layer_from_id() should not be called
 */
TEST_F(ControllerTests, controller_screen_remove_layer_nullScreen)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t l_layer_id = 10;

    struct iviscreen *lIviScreenRetList[1] = {nullptr};
    SET_RETURN_SEQ(wl_resource_get_user_data, (struct iviscreen **)&lIviScreenRetList, 1);
    controller_screen_remove_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
    ASSERT_EQ(get_layer_from_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_remove_layer_wrongLayerId
 * @brief               Test case of controller_screen_remove_layer() where wl_resource_get_user_data() success, return an object
 *                      but get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_remove_layer()
 *                      -# Verification point:
 *                         +# get_layer_from_id() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode is IVI_WM_SCREEN_ERROR
 *                         +# screen_remove_layer() should not be called
 */
TEST_F(ControllerTests, controller_screen_remove_layer_wrongLayerId)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t l_layer_id = 10;
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_screen_remove_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
    ASSERT_EQ(screen_remove_layer_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_remove_layer_success
 * @brief               Test case of controller_screen_remove_layer() where wl_resource_get_user_data() success, return an object
 *                      and get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_screen_remove_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 *                         +# screen_remove_layer() must be called once time
 */
TEST_F(ControllerTests, controller_screen_remove_layer_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t l_layer_id = 10;

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);

    controller_screen_remove_layer(lClient, lResource, l_layer_id);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(screen_remove_layer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_screen_screenshot_nullScreenShot
 * @brief               Test case of controller_screen_screenshot() where wl_resource_create() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Calling the controller_screen_screenshot()
 *                      -# Verification point:
 *                         +# wl_resource_post_no_memory() must be called once time
 *                         +# wl_resource_post_event() and weston_buffer_from_resource() should not be called
 */
TEST_F(ControllerTests, controller_screen_screenshot_nullScreenShot)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lID = 10;
    struct wl_resource *lResourceRetList[1] = {nullptr};
    SET_RETURN_SEQ(wl_resource_create, (struct wl_resource **)&lResourceRetList, 1);
    controller_screen_screenshot(lClient, lResource, lBufferResource, lID);

    ASSERT_EQ(wl_resource_post_no_memory_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(weston_buffer_from_resource_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_screenshot_nullScreen
 * @brief               Test case of controller_screen_screenshot() where wl_resource_create() success, return an object
 *                      but wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Calling the controller_screen_screenshot()
 *                      -# Verification point:
 *                         +# wl_resource_create() must be called once time
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_SCREENSHOT_ERROR
 *                         +# weston_buffer_from_resource() should not be called
 *                         +# wl_resource_destroy() must be called once time
 */
TEST_F(ControllerTests, controller_screen_screenshot_nullScreen)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lID = 10;

    struct iviscreen *lIviScreenRetList[1] = {nullptr};
    SET_RETURN_SEQ(wl_resource_get_user_data, (struct iviscreen **)&lIviScreenRetList, 1);
    struct wl_resource * screenshot[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, screenshot, 1);
    
    controller_screen_screenshot(lClient, lResource, lBufferResource, lID);

    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(wl_resource_get_user_data_fake.return_val_history[0], NULL);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_SCREENSHOT_ERROR);
    ASSERT_EQ(wl_resource_destroy_fake.call_count, 1);
    ASSERT_EQ(weston_buffer_from_resource_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_screenshot_nullWestonBuffer
 * @brief               Test case of controller_screen_screenshot() where wl_resource_create() success, return an object
 *                      but wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Calling the controller_screen_screenshot()
 *                      -# Verification point:
 *                         +# wl_resource_post_no_memory() should not be called
 *                         +# wl_resource_create() and weston_buffer_from_resource() must be called once time
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_SCREENSHOT_ERROR
 *                         +# wl_resource_set_implementation() must be called once time
 */
TEST_F(ControllerTests, controller_screen_screenshot_nullWestonBuffer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lID = 10;
    void * buffer[1] = {nullptr};
    struct wl_resource * screenshot[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, screenshot, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    SET_RETURN_SEQ(weston_buffer_from_resource, buffer, 1);

    controller_screen_screenshot(lClient, lResource, lBufferResource, lID);

    ASSERT_EQ(wl_resource_post_no_memory_fake.call_count, 0);
    ASSERT_EQ(wl_resource_create_fake.call_count, 1);
    ASSERT_EQ(weston_buffer_from_resource_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_SCREENSHOT_ERROR);
    ASSERT_EQ(wl_resource_set_implementation_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_screenshot_success
 * @brief               Test case of controller_screen_screenshot() where wl_resource_create() success, return an object
 *                      and wl_resource_get_user_data() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_screenshot()
 *                      -# Verification point:
 *                         +# wl_resource_set_implementation() must be called once time
 *                         +# wl_list_insert() must be called 2 times
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, controller_screen_screenshot_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lID = 10;

    struct wl_resource * screenshot[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, screenshot, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);
    SET_RETURN_SEQ(weston_buffer_from_resource, &mp_westonBuffer, 1);

    controller_screen_screenshot(lClient, lResource, lBufferResource, lID);

    EXPECT_EQ(wl_resource_post_no_memory_fake.call_count, 0);
    EXPECT_EQ(wl_resource_create_fake.call_count, 1);
    EXPECT_EQ(weston_buffer_from_resource_fake.call_count, 1);
    EXPECT_EQ(wl_resource_destroy_fake.call_count, 0);

    struct screenshot_frame_listener *lp_l = (struct screenshot_frame_listener*)wl_resource_set_implementation_fake.arg2_history[0];
    free(lp_l);
}

/** ================================================================================================
 * @test_id             controller_screen_get_nullScreen
 * @brief               Test case of controller_screen_get() where wl_resource_get_user_data() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking wl_resource_get_user_data() 0 return nullptr
 *                      -# Calling the controller_screen_get()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SCREEN_ERROR
 */
TEST_F(ControllerTests, controller_screen_get_nullScreen)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lParam = 1;
    struct iviscreen *lIviScreenRetList[1] = {nullptr};
    SET_RETURN_SEQ(wl_resource_get_user_data, (struct iviscreen **)&lIviScreenRetList, 1);
    controller_screen_get(lClient, lResource, lParam);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SCREEN_ERROR);
}

/** ================================================================================================
 * @test_id             controller_screen_get_invalidParam
 * @brief               Test case of controller_screen_get() where wl_resource_get_user_data() success, return an object
 *                      but invalid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_get()
 *                      -# Verification point:
 *                         +# get_layers_on_screen() not be called
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, controller_screen_get_invalidParam)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lParam = 1;
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);

    controller_screen_get(lClient, lResource, lParam);

    ASSERT_EQ(get_layers_on_screen_fake.call_count, 0);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_screen_get_success
 * @brief               Test case of controller_screen_get() where wl_resource_get_user_data() success, return an object
 *                      but valid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_screen_get()
 *                      -# Verification point:
 *                         +# get_layers_on_screen() must be called once time
 *                         +# wl_resource_post_event() must be called
 */
TEST_F(ControllerTests, controller_screen_get_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lParam = 10;

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviScreen, 1);

    controller_screen_get(lClient, lResource, lParam);

    ASSERT_EQ(get_layers_on_screen_fake.call_count, 1);
    ASSERT_NE(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_commit_changes_failed
 * @brief               Test case of controller_commit_changes() where commit_changes() fails, return -1
 *                      and wl_resource_get_user_data() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the commit_changes() does return -1
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_commit_changes()
 *                      -# Verification point:
 *                         +# commit_changes() must be called once time
 */
TEST_F(ControllerTests, controller_commit_changes_failed)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(commit_changes, mp_failureResult, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    controller_commit_changes(lClient, lResource);

    ASSERT_EQ(commit_changes_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_commit_changes_success
 * @brief               Test case of controller_commit_changes() where commit_changes() success, return 0
 *                      and wl_resource_get_user_data() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the commit_changes() does return 0
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_commit_changes()
 *                      -# Verification point:
 *                         +# commit_changes() must be called once time
 */
TEST_F(ControllerTests, controller_commit_changes_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(commit_changes, mp_successResult, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    controller_commit_changes(lClient, lResource);

    ASSERT_EQ(commit_changes_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_surface_visibility_wrongLayoutSurface
 * @brief               Test case of controller_set_surface_visibility() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_surface_visibility()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 *                         +# surface_set_visibility() should not be called
 */
TEST_F(ControllerTests, controller_set_surface_visibility_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceId{1};
    std::uint32_t lVisibility{1};
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    controller_set_surface_visibility(lClient, lResource, lSurfaceId, lVisibility);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
    ASSERT_EQ(surface_set_visibility_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_surface_visibility_success
 * @brief               Test case of controller_set_surface_visibility() where get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_visibility()
 *                      -# Verification point:
 *                         +# surface_set_visibility() must be called once time
 */
TEST_F(ControllerTests, controller_set_surface_visibility_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceId{1};
    std::uint32_t lVisibility{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_visibility(lClient, lResource, lSurfaceId, lVisibility);

    ASSERT_EQ(surface_set_visibility_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_layer_visibility_wrongLayoutLayer
 * @brief               Test case of controller_set_layer_visibility() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_layer_visibility()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 *                         +# surface_set_visibility() should not be called
 */
TEST_F(ControllerTests, controller_set_layer_visibility_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerId{1};
    std::uint32_t lVisibility{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_set_layer_visibility(lClient, lResource, lLayerId, lVisibility);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
    ASSERT_EQ(layer_set_visibility_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_layer_visibility_success
 * @brief               Test case of controller_set_layer_visibility() where get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_visibility()
 *                      -# Verification point:
 *                         +# layer_set_visibility() must be called once time
 */
TEST_F(ControllerTests, controller_set_layer_visibility_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerId{1};
    std::uint32_t lVisibility{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_visibility(lClient, lResource, lLayerId, lVisibility);

    ASSERT_EQ(layer_set_visibility_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_surface_opacity_wrongLayoutSurface
 * @brief               Test case of controller_set_surface_opacity() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_surface_opacity()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 *                         +# surface_set_opacity() should not be called
 */
TEST_F(ControllerTests, controller_set_surface_opacity_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceId{1};
    wl_fixed_t lOpacity{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutLayerRetList, 1);

    controller_set_surface_opacity(lClient, lResource, lSurfaceId, lOpacity);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
    ASSERT_EQ(surface_set_opacity_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_surface_opacity_success
 * @brief               Test case of controller_set_surface_opacity() where get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_opacity()
 *                      -# Verification point:
 *                         +# surface_set_opacity() must be called once time
 */
TEST_F(ControllerTests, controller_set_surface_opacity_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceId{1};
    wl_fixed_t lOpacity{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_opacity(lClient, lResource, lSurfaceId, lOpacity);

    ASSERT_EQ(surface_set_opacity_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_layer_opacity_wrongLayoutLayer
 * @brief               Test case of controller_set_layer_opacity() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_layer_opacity()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 *                         +# surface_set_opacity() should not be called
 */
TEST_F(ControllerTests, controller_set_layer_opacity_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerId{1};
    wl_fixed_t lOpacity{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_set_layer_opacity(lClient, lResource, lLayerId, lOpacity);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
    ASSERT_EQ(layer_set_opacity_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_layer_opacity_success
 * @brief               Test case of controller_set_layer_opacity() where get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_opacity()
 *                      -# Verification point:
 *                         +# layer_set_opacity() must be called once time
 */
TEST_F(ControllerTests, controller_set_layer_opacity_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerId{1};
    wl_fixed_t lOpacity{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_opacity(lClient, lResource, lLayerId, lOpacity);

    ASSERT_EQ(layer_set_opacity_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_surface_source_rectangle_wrongLayoutSurface
 * @brief               Test case of controller_set_surface_source_rectangle() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_surface_source_rectangle()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 *                         +# get_properties_of_surface() should not be called
 */
TEST_F(ControllerTests, controller_set_surface_source_rectangle_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);
    controller_set_surface_source_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
    ASSERT_EQ(get_properties_of_surface_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_surface_source_rectangle_successWithPositiveValues
 * @brief               Test case of controller_set_surface_source_rectangle() where get_surface_from_id() success, return an object
 *                      and input x, y, width, height are positive values
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_source_rectangle()
 *                      -# Verification point:
 *                         +# surface_set_source_rectangle() must be called once time
 *                         +# Input data of surface_set_source_rectangle() must same input data of controller_set_surface_source_rectangle()
 */
TEST_F(ControllerTests, controller_set_surface_source_rectangle_successWithPositiveValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_source_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(surface_set_source_rectangle_fake.call_count, 1);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg1_val, x);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg2_val, y);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg3_val, width);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg4_val, height);
}

/** ================================================================================================
 * @test_id             controller_set_surface_source_rectangle_successWithNegativeValues
 * @brief               Test case of controller_set_surface_source_rectangle() where get_surface_from_id() success, return an object
 *                      and input x, y, width, height are negative values
 * @test_procedure Steps:
 *                      -# Set data of ivi_layout_surface_properties to default value {0}
 *                      -# Mocking the get_properties_of_surface() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_source_rectangle()
 *                      -# Verification point:
 *                         +# surface_set_source_rectangle() must be called once time
 *                         +# Input data of surface_set_source_rectangle() must same default value of ivi_layout_surface_properties
 */
TEST_F(ControllerTests, controller_set_surface_source_rectangle_successWithNegativeValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{-1};
    std::int32_t y{-1};
    std::int32_t width{-1};
    std::int32_t height{-1};
    const struct ivi_layout_surface_properties l_prop = {
        .opacity = 0,
        .source_x = 0,
        .source_y = 0,
        .source_width = 0,
        .source_height = 0,
        .start_x = 0,
        .start_y = 0,
        .start_width = 0,
        .start_height = 0,
        .dest_x = 0,
        .dest_y = 0,
        .dest_width = 0,
        .dest_height = 0,
        .orientation = WL_OUTPUT_TRANSFORM_NORMAL,
        .visibility = true,
        .transition_type = 0,
        .transition_duration = 0,
        .event_mask = 0,
    };
    const struct ivi_layout_surface_properties *prop[1] = {&l_prop};

    SET_RETURN_SEQ(get_properties_of_surface, prop, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_source_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(surface_set_source_rectangle_fake.call_count, 1);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg1_val, l_prop.source_x);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg2_val, l_prop.source_y);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg3_val, l_prop.source_width);
    ASSERT_EQ(surface_set_source_rectangle_fake.arg4_val, l_prop.source_height);
}

/** ================================================================================================
 * @test_id             controller_set_layer_source_rectangle_wrongLayoutLayer
 * @brief               Test case of controller_set_layer_source_rectangle() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 *                         +# get_properties_of_layer() should not be called
 */
TEST_F(ControllerTests, controller_set_layer_source_rectangle_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);
    controller_set_layer_source_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
    ASSERT_EQ(get_properties_of_layer_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_layer_source_rectangle_successWithPositiveValues
 * @brief               Test case of controller_set_layer_source_rectangle() where get_layer_from_id() success, return an object
 *                      and input x, y, width, height are positive values
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# layer_set_source_rectangle() must be called once time
 *                         +# Input data of layer_set_source_rectangle() must same input data of controller_set_layer_source_rectangle()
 */
TEST_F(ControllerTests, controller_set_layer_source_rectangle_successWithPositiveValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_source_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(layer_set_source_rectangle_fake.call_count, 1);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg1_val, x);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg2_val, y);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg3_val, width);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg4_val, height);
}

/** ================================================================================================
 * @test_id             controller_set_layer_source_rectangle_successWithNegativeValues
 * @brief               Test case of controller_set_layer_source_rectangle() where get_layer_from_id() success, return an object
 *                      and input x, y, width, height are negative values
 * @test_procedure Steps:
 *                      -# Set data of ivi_layout_layer_properties to default value {0}
 *                      -# Mocking the get_properties_of_layer() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_source_rectangle()
 *                      -# Verification point:
 *                         +# layer_set_source_rectangle() must be called once time
 *                         +# Input data of layer_set_source_rectangle() must same default value of ivi_layout_layer_properties
 */
TEST_F(ControllerTests, controller_set_layer_source_rectangle_successWithNegativeValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{-1};
    std::int32_t y{-1};
    std::int32_t width{-1};
    std::int32_t height{-1};
    const struct ivi_layout_layer_properties l_prop = {
        .opacity = 0,
        .source_x = 0,
        .source_y = 0,
        .source_width = 0,
        .source_height = 0,
        .dest_x = 0,
        .dest_y = 0,
        .dest_width = 0,
        .dest_height = 0,
        .orientation = WL_OUTPUT_TRANSFORM_NORMAL,
        .visibility = true,
        .transition_type = 0,
        .transition_duration = 0,
        .start_alpha = 0,
	    .end_alpha = 0,
	    .is_fade_in = 0,
        .event_mask = 0,
    };
    const struct ivi_layout_layer_properties *prop[1] = {&l_prop};

    SET_RETURN_SEQ(get_properties_of_layer, prop, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_source_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(layer_set_source_rectangle_fake.call_count, 1);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg1_val, l_prop.source_x);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg2_val, l_prop.source_y);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg3_val, l_prop.source_width);
    ASSERT_EQ(layer_set_source_rectangle_fake.arg4_val, l_prop.source_height);
}

/** ================================================================================================
 * @test_id             controller_set_surface_destination_rectangle_wrongLayoutSurface
 * @brief               Test case of controller_set_surface_destination_rectangle() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_surface_destination_rectangle()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 *                         +# get_properties_of_surface() should not be called
 */
TEST_F(ControllerTests, controller_set_surface_destination_rectangle_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_set_surface_destination_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
    ASSERT_EQ(get_properties_of_surface_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_surface_destination_rectangle_successWithPositiveValues
 * @brief               Test case of controller_set_surface_destination_rectangle() where get_surface_from_id() success, return an object
 *                      and input x, y, width, height are positive values
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_destination_rectangle()
 *                      -# Verification point:
 *                         +# surface_set_destination_rectangle() must be called once time
 *                         +# Input data of surface_set_destination_rectangle() must same input data of controller_set_surface_destination_rectangle()
 */
TEST_F(ControllerTests, controller_set_surface_destination_rectangle_successWithPositiveValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_destination_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(surface_set_destination_rectangle_fake.call_count, 1);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg1_val, x);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg2_val, y);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg3_val, width);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg4_val, height);
}

/** ================================================================================================
 * @test_id             controller_set_surface_destination_rectangle_successWithNegativeValues
 * @brief               Test case of controller_set_surface_destination_rectangle() where get_surface_from_id() success, return an object
 *                      and input x, y, width, height are negative values
 * @test_procedure Steps:
 *                      -# Set data of ivi_layout_surface_properties to default value {0}
 *                      -# Mocking the get_properties_of_surface() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_destination_rectangle()
 *                      -# Verification point:
 *                         +# surface_set_destination_rectangle() must be called once time
 *                         +# Input data of surface_set_destination_rectangle() must same default value of ivi_layout_surface_properties
 */
TEST_F(ControllerTests, controller_set_surface_destination_rectangle_successWithNegativeValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t x{-1};
    std::int32_t y{-1};
    std::int32_t width{-1};
    std::int32_t height{-1};
    const struct ivi_layout_surface_properties l_prop = {
        .opacity = 0,
        .source_x = 0,
        .source_y = 0,
        .source_width = 0,
        .source_height = 0,
        .start_x = 0,
        .start_y = 0,
        .start_width = 0,
        .start_height = 0,
        .dest_x = 0,
        .dest_y = 0,
        .dest_width = 0,
        .dest_height = 0,
        .orientation = WL_OUTPUT_TRANSFORM_NORMAL,
        .visibility = true,
        .transition_type = 0,
        .transition_duration = 0,
        .event_mask = 0,
    };
    const struct ivi_layout_surface_properties *prop[1] = {&l_prop};

    SET_RETURN_SEQ(get_properties_of_surface, prop, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_destination_rectangle(lClient, lResource, lSurfaceID, x, y, width, height);

    ASSERT_EQ(surface_set_destination_rectangle_fake.call_count, 1);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg1_val, l_prop.source_x);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg2_val, l_prop.source_y);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg3_val, l_prop.source_width);
    ASSERT_EQ(surface_set_destination_rectangle_fake.arg4_val, l_prop.source_height);
}

/** ================================================================================================
 * @test_id             controller_set_layer_destination_rectangle_wrongLayoutLayer
 * @brief               Test case of controller_set_layer_destination_rectangle() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_layer_destination_rectangle()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 *                         +# get_properties_of_layer() should not be called
 */
TEST_F(ControllerTests, controller_set_layer_destination_rectangle_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_set_layer_destination_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
    ASSERT_EQ(get_properties_of_layer_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_set_layer_destination_rectangle_successWithPositiveValues
 * @brief               Test case of controller_set_layer_destination_rectangle() where get_layer_from_id() success, return an object
 *                      and input x, y, width, height are positive values
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_destination_rectangle()
 *                      -# Verification point:
 *                         +# layer_set_destination_rectangle() must be called once time
 *                         +# Input data of layer_set_destination_rectangle() must same input data of controller_set_layer_destination_rectangle()
 */
TEST_F(ControllerTests, controller_set_layer_destination_rectangle_successWithPositiveValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{1};
    std::int32_t y{1};
    std::int32_t width{1};
    std::int32_t height{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_destination_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(layer_set_destination_rectangle_fake.call_count, 1);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg1_val, x);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg2_val, y);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg3_val, width);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg4_val, height);
}

/** ================================================================================================
 * @test_id             controller_set_layer_destination_rectangle_successWithNegativeValues
 * @brief               Test case of controller_set_layer_destination_rectangle() where get_layer_from_id() success, return an object
 *                      and input x, y, width, height are negative values
 * @test_procedure Steps:
 *                      -# Set data of ivi_layout_layer_properties to default value {0}
 *                      -# Mocking the get_properties_of_layer() does return an object
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_set_layer_destination_rectangle()
 *                      -# Verification point:
 *                         +# layer_set_destination_rectangle() must be called once time
 *                         +# Input data of layer_set_destination_rectangle() must same default value of ivi_layout_layer_properties
 */
TEST_F(ControllerTests, controller_set_layer_destination_rectangle_successWithNegativeValues)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t x{-1};
    std::int32_t y{-1};
    std::int32_t width{-1};
    std::int32_t height{-1};
    const struct ivi_layout_layer_properties l_prop = {
        .opacity = 0,
        .source_x = 0,
        .source_y = 0,
        .source_width = 0,
        .source_height = 0,
        .dest_x = 0,
        .dest_y = 0,
        .dest_width = 0,
        .dest_height = 0,
        .orientation = WL_OUTPUT_TRANSFORM_NORMAL,
        .visibility = true,
        .transition_type = 0,
        .transition_duration = 0,
        .start_alpha = 0,
	    .end_alpha = 0,
	    .is_fade_in = 0,
        .event_mask = 0,
    };
    const struct ivi_layout_layer_properties *prop[1] = {&l_prop};

    SET_RETURN_SEQ(get_properties_of_layer, prop, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_set_layer_destination_rectangle(lClient, lResource, lLayerID, x, y, width, height);

    ASSERT_EQ(layer_set_destination_rectangle_fake.call_count, 1);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg1_val, l_prop.source_x);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg2_val, l_prop.source_y);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg3_val, l_prop.source_width);
    ASSERT_EQ(layer_set_destination_rectangle_fake.arg4_val, l_prop.source_height);
}

/** ================================================================================================
 * @test_id             controller_surface_sync_wrongLayoutSurface
 * @brief               Test case of controller_surface_sync() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_surface_sync()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 */
TEST_F(ControllerTests, controller_surface_sync_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lSyncState{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_surface_sync(lClient, lResource, lSurfaceID, lSyncState);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
}

/** ================================================================================================
 * @test_id             controller_surface_sync_add
 * @brief               Test case of controller_surface_sync() where get_surface_from_id() success, return an object
 *                      and input sync_state is IVI_WM_SYNC_ADD {0}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_surface_sync()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 2 times
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, controller_surface_sync_add)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lSyncState{0};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {&m_layoutSurface[0]};
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_surface_sync(lClient, mp_surfaceNotification[0]->resource, lSurfaceID, lSyncState);

    EXPECT_EQ(wl_list_insert_fake.call_count, 2);

    struct notification *l_not = (struct notification*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct notification, link));
    free(l_not);
}

/** ================================================================================================
 * @test_id             controller_surface_sync_remove
 * @brief               Test case of controller_surface_sync() where get_surface_from_id() success, return an object
 *                      and input sync_state is IVI_WM_SYNC_REMOVE {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_surface_sync()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 2 times
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, controller_surface_sync_remove)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lSyncState{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {&m_layoutSurface[0]};
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_surface_sync(lClient, mp_surfaceNotification[0]->resource, lSurfaceID, lSyncState);

    EXPECT_EQ(wl_list_remove_fake.call_count, 2);

    mp_surfaceNotification[0] = NULL;
}

/** ================================================================================================
 * @test_id             controller_surface_sync_default
 * @brief               Test case of controller_surface_sync() where get_surface_from_id() success, return an object
 *                      and input sync_state is default value (not add/remove)
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_surface_sync()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 */
TEST_F(ControllerTests, controller_surface_sync_default)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lSyncState{2};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_surface_sync(lClient, lResource, lSurfaceID, lSyncState);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_sync_wrongLayoutLayer
 * @brief               Test case of controller_layer_sync() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_layer_sync()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_sync_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lSyncState{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_layer_sync(lClient, lResource, lLayerID, lSyncState);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_sync_add
 * @brief               Test case of controller_layer_sync() where get_layer_from_id() success, return an object
 *                      and input sync_state is IVI_WM_SYNC_ADD {0}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_sync()
 *                      -# Verification point:
 *                         +# wl_list_insert() must be called 2 times
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, controller_layer_sync_add)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lSyncState{0};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {&m_layoutLayer[0]};
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_layer_sync(lClient, mp_LayerNotification[0]->resource, lLayerID, lSyncState);

    EXPECT_EQ(wl_list_insert_fake.call_count, 2);

    struct notification *l_not = (struct notification*)(uintptr_t(wl_list_insert_fake.arg1_history[0]) - offsetof(struct notification, link));
    free(l_not);
}

/** ================================================================================================
 * @test_id             controller_layer_sync_remove
 * @brief               Test case of controller_layer_sync() where get_layer_from_id() success, return an object
 *                      and input sync_state is IVI_WM_SYNC_REMOVE {1}
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_sync()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 2 times
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, controller_layer_sync_remove)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lSyncState{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {&m_layoutLayer[0]};
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_layer_sync(lClient, mp_LayerNotification[0]->resource, lLayerID, lSyncState);

    EXPECT_EQ(wl_list_remove_fake.call_count, 2);

    mp_LayerNotification[0] = NULL;
}

/** ================================================================================================
 * @test_id             controller_layer_sync_default
 * @brief               Test case of controller_layer_sync() where get_layer_from_id() success, return an object
 *                      and input sync_state is default value (not add/remove)
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_sync()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_sync_default)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lSyncState{2};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_layer_sync(lClient, lResource, lLayerID, lSyncState);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_surface_get_wrongLayoutSurface
 * @brief               Test case of controller_surface_get() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_surface_get()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 */
TEST_F(ControllerTests, controller_surface_get_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lParam{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_surface_get(lClient, lResource, lSurfaceID, lParam);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
}

/** ================================================================================================
 * @test_id             controller_surface_get_success
 * @brief               Test case of controller_surface_get() where get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Mocking the surface_get_weston_surface() does return an object
 *                      -# Calling the controller_surface_get()
 *                      -# Verification point:
 *                         +# get_properties_of_surface() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, controller_surface_get_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lSurfaceID{1};
    std::int32_t lParam{0};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {&m_layoutSurface[0]};
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);
    struct weston_surface *l_surface[1] = {(struct weston_surface *)malloc(sizeof(struct weston_surface))};
    SET_RETURN_SEQ(surface_get_weston_surface, l_surface, 1);

    controller_surface_get(lClient, lResource, lSurfaceID, lParam);

    EXPECT_EQ(get_properties_of_surface_fake.call_count, 1);

    free(l_surface[0]);
}

/** ================================================================================================
 * @test_id             controller_layer_get_wrongLayoutLayer
 * @brief               Test case of controller_layer_get() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_layer_get()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_get_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lParam{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_layer_get(lClient, lResource, lLayerID, lParam);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_get_wrongParam
 * @brief               Test case of controller_layer_get() where get_layer_from_id() success, return an object
 *                      and invalid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_get()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 *                         +# get_surfaces_on_layer() not be called
 */
TEST_F(ControllerTests, controller_layer_get_wrongParam)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lParam{0};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);

    controller_layer_get(lClient, lResource, lLayerID, lParam);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
    ASSERT_EQ(get_surfaces_on_layer_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_layer_get_success
 * @brief               Test case of controller_layer_get() where get_layer_from_id() success, return an object
 *                      and valid input param
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Mocking the get_properties_of_layer() does return an object
 *                      -# Calling the controller_layer_get()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_SURFACE_ADDED
 *                         +# get_surfaces_on_layer() must be called once time
 */
TEST_F(ControllerTests, controller_layer_get_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    std::uint32_t lLayerID{1};
    std::int32_t lParam{8};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layout_layer[1] = {&m_layoutLayer[0]};
    SET_RETURN_SEQ(get_layer_from_id, l_layout_layer, 1);
    const struct ivi_layout_layer_properties *l_prop[1] = {&m_layoutLayerProperties[0]};
    SET_RETURN_SEQ(get_properties_of_layer, l_prop, 1);

    controller_layer_get(lClient, lResource, lLayerID, lParam);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_SURFACE_ADDED);
    ASSERT_EQ(get_surfaces_on_layer_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_nullScreenShot
 * @brief               Test case of controller_surface_screenshot() where wl_resource_create() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:`
 *                         +# wl_client_post_no_memory() must be called once time
 */
TEST_F(ControllerTests, controller_surface_screenshot_nullScreenShot)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    struct wl_resource * lScreenshotRetList[1] = {NULL};
    SET_RETURN_SEQ(wl_resource_get_user_data,(void**)mp_iviController, 1);
    SET_RETURN_SEQ(wl_resource_create, (void**)lScreenshotRetList, 1);

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    ASSERT_EQ(wl_client_post_no_memory_fake.call_count, 1);
    ASSERT_EQ(get_surface_from_id_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_GetLayoutSurfaceFailed
 * @brief               Test case of controller_surface_screenshot() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the wl_resource_create() does return an object
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_destroy() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode should be IVI_SCREENSHOT_ERROR
 *                         +# surface_get_size() should not be called
 */
TEST_F(ControllerTests, controller_surface_screenshot_GetLayoutSurfaceFailed)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct wl_resource * lScreenshotRetList[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, lScreenshotRetList, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_destroy_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    ASSERT_EQ(surface_get_size_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_ContentOfSurfaceInvalid
 * @brief               Test case of controller_surface_screenshot() where the content of surface are invalid
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an success object
 *                      -# Mocking the wl_resource_create() does return an success object
 *                      -# Mocking the get_surface_from_id() does return an success object
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_destroy() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode should be IVI_SCREENSHOT_ERROR
 *                         +# surface_get_size() should not be called
 *                         +# weston_buffer_from_resource() should not be called
 */
TEST_F(ControllerTests, controller_surface_screenshot_ContentOfSurfaceInvalid)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct wl_resource *lScreenshotRetList[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, lScreenshotRetList, 1);
    struct ivi_layout_surface *lLayoutSurfaceRetList[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, lLayoutSurfaceRetList, 1);

    surface_get_size_fake.custom_fake = nullptr;

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(wl_resource_destroy_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    ASSERT_EQ(surface_get_size_fake.call_count, 1);
    ASSERT_EQ(weston_buffer_from_resource_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_WhenGetWestonBuffer
 * @brief               Test case of controller_surface_screenshot() where cannot get weston buffer from buffer resource
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an success object
 *                      -# Mocking the wl_resource_create() does return an success object
 *                      -# Mocking the get_surface_from_id() does return an success object
 *                      -# Mocking the weston_buffer_from_resource() does return an null pointer
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_destroy() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode should be IVI_SCREENSHOT_ERROR
 *                         +# surface_get_size() must be called once time
 *                         +# weston_buffer_from_resource() must be called once time and return a NULL pointer
 *                         +# wl_shm_buffer_get_data() should not be called
 */
TEST_F(ControllerTests, controller_surface_screenshot_WhenGetWestonBuffer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct wl_resource *lScreenshotRetList[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, lScreenshotRetList, 1);
    struct ivi_layout_surface *lLayoutSurfaceRetList[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, lLayoutSurfaceRetList, 1);
    surface_get_size_fake.custom_fake = custom_surface_get_size;
    struct weston_buffer * lWestonBufferRetList[1] = {NULL};
    SET_RETURN_SEQ(weston_buffer_from_resource, (struct weston_buffer **)lWestonBufferRetList, 1);

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    ASSERT_EQ(get_surface_from_id_fake.call_count, 1);
    ASSERT_EQ(surface_get_size_fake.call_count, 1);
    ASSERT_EQ(weston_buffer_from_resource_fake.call_count, 1);
    ASSERT_EQ(weston_buffer_from_resource_fake.return_val, NULL);
    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    ASSERT_EQ(wl_shm_buffer_get_data_fake.call_count, 0);
    ASSERT_EQ(wl_resource_destroy_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_SurfaceDumpDataFailed
 * @brief               controller_surface_screenshot() is called when surface dump data to buffer failed
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an success object
 *                      -# Mocking the wl_resource_create() does return an success object
 *                      -# Mocking the get_surface_from_id() does return an success object
 *                      -# Mocking the weston_buffer_from_resource() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_stride() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_width() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_height() does return an success object
 *                      -# Mocking the surface_dump() does return an failure object
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_destroy() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode should be IVI_SCREENSHOT_ERROR
 *                         +# surface_get_size() must be called once time
 *                         +# weston_buffer_from_resource() must be called once time
 *                         +# wl_shm_buffer_get_data() must be called once time
 *                         +# surface_dump() must be called once time
 */
TEST_F(ControllerTests, controller_surface_screenshot_SurfaceDumpDataFailed)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct wl_resource *lScreenshotRetList[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, lScreenshotRetList, 1);
    struct ivi_layout_surface *lLayoutSurfaceRetList[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, lLayoutSurfaceRetList, 1);
    surface_get_size_fake.custom_fake = custom_surface_get_size;
    struct weston_buffer * westonBuffer = (struct weston_buffer*)malloc(sizeof(weston_buffer));
    westonBuffer->type = 0; // WESTON_BUFFER_SHM
    struct weston_buffer * lWestonBufferRetList[1] = {westonBuffer};
    SET_RETURN_SEQ(weston_buffer_from_resource, (struct weston_buffer **)lWestonBufferRetList, 1);
    int validStride[1] = {4};
    int validWidth[1] = {1};
    int validheight[1] = {1};
    SET_RETURN_SEQ(wl_shm_buffer_get_stride, validStride, 1);
    SET_RETURN_SEQ(wl_shm_buffer_get_width, validWidth, 1);
    SET_RETURN_SEQ(wl_shm_buffer_get_height, validheight, 1);
    SET_RETURN_SEQ(surface_dump, &mp_failureResult[0], 1);

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(wl_resource_destroy_fake.call_count, 1);
    EXPECT_EQ(surface_get_size_fake.call_count, 1);
    EXPECT_EQ(weston_buffer_from_resource_fake.call_count, 1);
    EXPECT_EQ(wl_shm_buffer_get_data_fake.call_count, 1);
    EXPECT_EQ(surface_dump_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    free(westonBuffer);
}

/** ================================================================================================
 * @test_id             controller_surface_screenshot_success
 * @brief               controller_surface_screenshot() is called successfully
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an success object
 *                      -# Mocking the wl_resource_create() does return an success object
 *                      -# Mocking the get_surface_from_id() does return an success object
 *                      -# Mocking the weston_buffer_from_resource() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_stride() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_width() does return an success object
 *                      -# Mocking the wl_shm_buffer_get_height() does return an success object
 *                      -# Mocking the surface_dump() does return an success object
 *                      -# Calling the controller_surface_screenshot()
 *                      -# Verification point:
 *                         +# get_surface_from_id() must be called once time
 *                         +# wl_resource_destroy() must be called once time
 *                         +# wl_resource_post_event() must be called once time and opcode should be IVI_SCREENSHOT_DONE
 *                         +# surface_get_size() must be called once time
 *                         +# weston_buffer_from_resource() must be called once time
 *                         +# wl_shm_buffer_get_data() must be called once time
 *                         +# surface_dump() must be called once time
 */
TEST_F(ControllerTests, controller_surface_screenshot_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lBufferResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lScreenshotId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct wl_resource *lScreenshotRetList[1] = {(struct wl_resource *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(wl_resource_create, lScreenshotRetList, 1);
    struct ivi_layout_surface *lLayoutSurfaceRetList[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, lLayoutSurfaceRetList, 1);
    surface_get_size_fake.custom_fake = custom_surface_get_size;
    struct weston_buffer * westonBuffer = (struct weston_buffer*)malloc(sizeof(weston_buffer));
    westonBuffer->type = 0; // WESTON_BUFFER_SHM
    struct weston_buffer * lWestonBufferRetList[1] = {westonBuffer};
    SET_RETURN_SEQ(weston_buffer_from_resource, (struct weston_buffer **)lWestonBufferRetList, 1);
    int validStride[1] = {4};
    int validWidth[1] = {1};
    int validheight[1] = {1};
    SET_RETURN_SEQ(wl_shm_buffer_get_stride, validStride, 1);
    SET_RETURN_SEQ(wl_shm_buffer_get_width, validWidth, 1);
    SET_RETURN_SEQ(wl_shm_buffer_get_height, validheight, 1);

    controller_surface_screenshot(lClient, lResource, lBufferResource, lScreenshotId, lSurfaceId);

    EXPECT_EQ(get_surface_from_id_fake.call_count, 1);
    EXPECT_EQ(wl_resource_destroy_fake.call_count, 1);
    EXPECT_EQ(surface_get_size_fake.call_count, 1);
    EXPECT_EQ(weston_buffer_from_resource_fake.call_count, 1);
    EXPECT_EQ(wl_shm_buffer_get_data_fake.call_count, 1);
    EXPECT_EQ(surface_dump_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_DONE);
    free(westonBuffer);
}

/** ================================================================================================
 * @test_id             controller_screenshooter_done_WESTON_SCREENSHOOTER_SUCCESS_option
 * @brief               Test case of controller_screenshooter_done() when process WESTON_SCREENSHOOTER_SUCCESS option
 * @test_procedure Steps:
 *                      -# Call controller_screenshooter_done() with arg is WESTON_SCREENSHOOTER_SUCCESS option
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode is IVI_SCREENSHOT_DONE
 */
TEST_F(ControllerTests, controller_screenshooter_done_WESTON_SCREENSHOOTER_SUCCESS_option)
{
    struct ivi_screenshooter *screenshooter = (struct ivi_screenshooter *)malloc(sizeof(struct ivi_screenshooter));
    screenshooter->output = &m_westonOutput[0];
    weston_screenshooter_outcome outcome = WESTON_SCREENSHOOTER_SUCCESS;
    controller_screenshooter_done(screenshooter, outcome);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_DONE);
    free(screenshooter);
}

/** ================================================================================================
 * @test_id             controller_screenshooter_done_WESTON_SCREENSHOOTER_NO_MEMORY_option
 * @brief               Test case of controller_screenshooter_done() when process WESTON_SCREENSHOOTER_NO_MEMORY option
 * @test_procedure Steps:
 *                      -# Call controller_screenshooter_done() with arg is WESTON_SCREENSHOOTER_NO_MEMORY option
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode is IVI_SCREENSHOT_ERROR
 */
TEST_F(ControllerTests, controller_screenshooter_done_WESTON_SCREENSHOOTER_NO_MEMORY_option)
{
    struct ivi_screenshooter *screenshooter = (struct ivi_screenshooter *)malloc(sizeof(struct ivi_screenshooter));
    screenshooter->output = &m_westonOutput[0];
    weston_screenshooter_outcome outcome = WESTON_SCREENSHOOTER_NO_MEMORY;
    controller_screenshooter_done(screenshooter, outcome);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    free(screenshooter);
}

/** ================================================================================================
 * @test_id             controller_screenshooter_done_WESTON_SCREENSHOOTER_BAD_BUFFER_option
 * @brief               Test case of controller_screenshooter_done() when process WESTON_SCREENSHOOTER_BAD_BUFFER option
 * @test_procedure Steps:
 *                      -# Call controller_screenshooter_done() with arg is WESTON_SCREENSHOOTER_BAD_BUFFER option
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode is IVI_SCREENSHOT_ERROR
 */
TEST_F(ControllerTests, controller_screenshooter_done_WESTON_SCREENSHOOTER_BAD_BUFFER_option)
{
    struct ivi_screenshooter *screenshooter = (struct ivi_screenshooter *)malloc(sizeof(struct ivi_screenshooter));
    screenshooter->output = &m_westonOutput[0];
    weston_screenshooter_outcome outcome = WESTON_SCREENSHOOTER_BAD_BUFFER;
    controller_screenshooter_done(screenshooter, outcome);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 1);
    EXPECT_EQ(wl_resource_post_event_fake.arg1_val, IVI_SCREENSHOT_ERROR);
    free(screenshooter);
}

/** ================================================================================================
 * @test_id             controller_screenshooter_done_InvalidArg
 * @brief               Test case of controller_screenshooter_done() when process invalid option
 * @test_procedure Steps:
 *                      -# Call controller_screenshooter_done() with invalid option
 *                      -# Verification point:
 *                         +# wl_resource_post_event() should not be called
 */
TEST_F(ControllerTests, controller_screenshooter_done_InvalidArg)
{
    struct ivi_screenshooter *screenshooter = (struct ivi_screenshooter *)malloc(sizeof(struct ivi_screenshooter));
    screenshooter->output = &m_westonOutput[0];
    weston_screenshooter_outcome outcome = (weston_screenshooter_outcome)10;
    controller_screenshooter_done(screenshooter, outcome);
    EXPECT_EQ(wl_resource_post_event_fake.call_count, 0);
    free(screenshooter);
}

/** ================================================================================================
 * @test_id             controller_screenshoot_destroy_call
 * @brief               Check behavior of controller_screenshoot_destroy()
 * @test_procedure Steps:
 *                      -# Mocking wl_resource_get_user_data() to return an success object
 *                      -# Call controller_screenshoot_destroy() with invalid option
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() should be called 1 time
 */
TEST_F(ControllerTests, controller_screenshoot_destroy_call)
{
    struct wl_resource *lResource{0xFFFFFFFF};
    struct ivi_screenshooter * screenshooter = (struct ivi_screenshooter *)malloc(sizeof(struct ivi_screenshooter));
    screenshooter->output = &m_westonOutput[0];
    struct ivi_screenshooter * screenshooterRet[1] = {screenshooter};
    SET_RETURN_SEQ(wl_resource_get_user_data, (struct ivi_screenshooter **)screenshooterRet, 1);
    controller_screenshoot_destroy(lResource);
    ASSERT_EQ(wl_resource_get_user_data_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_set_surface_type_wrongLayoutSurface
 * @brief               Test case of controller_set_surface_type() where get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_set_surface_type()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_SURFACE_ERROR
 */
TEST_F(ControllerTests, controller_set_surface_type_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lSurfaceId{1};
    int32_t lType{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_set_surface_type(lClient, lResource, lSurfaceId, lType);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_SURFACE_ERROR);
}

/** ================================================================================================
 * @test_id             controller_set_surface_type_success
 * @brief               Test case of controller_set_surface_type() where get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_set_surface_type()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() not be called
 */
TEST_F(ControllerTests, controller_set_surface_type_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lSurfaceId{1};
    int32_t lType{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {&m_layoutSurface[0]};
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_set_surface_type(lClient, lResource, lSurfaceId, lType);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_layer_clear_wrongLayoutLayer
 * @brief               Test case of controller_layer_clear() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_layer_clear()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_clear_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_layer_clear(lClient, lResource, lLayerId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_clear_success
 * @brief               Test case of controller_layer_clear() where get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_clear()
 *                      -# Verification point:
 *                         +# layer_set_render_order() must be called once time
 */
TEST_F(ControllerTests, controller_layer_clear_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);

    controller_layer_clear(lClient, lResource, lLayerId);

    ASSERT_EQ(layer_set_render_order_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_layer_add_surface_wrongLayoutLayer
 * @brief               Test case of controller_layer_add_surface() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_layer_add_surface()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_add_surface_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_layer_add_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_add_surface_wrongLayoutSurface
 * @brief               Test case of controller_layer_add_surface() where get_layer_from_id() success, return an object
 *                      but get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_add_surface()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_add_surface_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_layer_add_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_add_surface_success
 * @brief               Test case of controller_layer_add_surface() where get_layer_from_id() success, return an object
 *                      and get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_layer_add_surface()
 *                      -# Verification point:
 *                         +# layer_add_surface() must be called once time
 */
TEST_F(ControllerTests, controller_layer_add_surface_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_layer_add_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(layer_add_surface_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_layer_remove_surface_wrongLayoutLayer
 * @brief               Test case of controller_layer_remove_surface() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_layer_remove_surface()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_remove_surface_wrongLayoutLayer)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_layer_remove_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_remove_surface_wrongLayoutSurface
 * @brief               Test case of controller_layer_remove_surface() where get_layer_from_id() success, return an object
 *                      and get_surface_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_layer_remove_surface()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_layer_remove_surface_wrongLayoutSurface)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);
    struct ivi_layout_surface *lIviLayoutSurfaceRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_surface_from_id, (struct ivi_layout_surface **)&lIviLayoutSurfaceRetList, 1);

    controller_layer_remove_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_layer_remove_surface_success
 * @brief               Test case of controller_layer_remove_surface() where get_layer_from_id() success, return an object
 *                      and get_surface_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Mocking the get_surface_from_id() does return an object
 *                      -# Calling the controller_layer_remove_surface()
 *                      -# Verification point:
 *                         +# layer_remove_surface() must be called once time
 */
TEST_F(ControllerTests, controller_layer_remove_surface_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    uint32_t lSurfaceId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);
    struct ivi_layout_surface *l_layout_surface[1] = {(struct ivi_layout_surface *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_surface_from_id, l_layout_surface, 1);

    controller_layer_remove_surface(lClient, lResource, lLayerId, lSurfaceId);

    ASSERT_EQ(layer_remove_surface_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_create_layout_layer_error
 * @brief               Test case of controller_create_layout_layer() where layer_create_with_dimension() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_create_layout_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_no_memory() must be called once time
 */
TEST_F(ControllerTests, controller_create_layout_layer_error)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    int32_t lWidth{1};
    int32_t lHeight{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(layer_create_with_dimension, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_create_layout_layer(lClient, lResource, lLayerId, lWidth, lHeight);

    ASSERT_EQ(wl_resource_post_no_memory_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             controller_create_layout_layer_success
 * @brief               Test case of controller_create_layout_layer() where layer_create_with_dimension() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the layer_create_with_dimension() does return an object
 *                      -# Calling the controller_create_layout_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_no_memory() not be called
 */
TEST_F(ControllerTests, controller_create_layout_layer_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    int32_t lWidth{1};
    int32_t lHeight{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(layer_create_with_dimension, l_layoutLayer, 1);

    controller_create_layout_layer(lClient, lResource, lLayerId, lWidth, lHeight);

    ASSERT_EQ(wl_resource_post_no_memory_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             controller_destroy_layout_layer_error
 * @brief               Test case of controller_destroy_layout_layer() where get_layer_from_id() fails, return null pointer
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the controller_destroy_layout_layer()
 *                      -# Verification point:
 *                         +# wl_resource_post_event() must be called once time with opcode IVI_WM_LAYER_ERROR
 */
TEST_F(ControllerTests, controller_destroy_layout_layer_error)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *lIviLayoutLayerRetList[1] = {nullptr};
    SET_RETURN_SEQ(get_layer_from_id, (struct ivi_layout_layer **)&lIviLayoutLayerRetList, 1);

    controller_destroy_layout_layer(lClient, lResource, lLayerId);

    ASSERT_EQ(wl_resource_post_event_fake.call_count, 1);
    ASSERT_EQ(wl_resource_post_event_fake.arg1_history[0], IVI_WM_LAYER_ERROR);
}

/** ================================================================================================
 * @test_id             controller_destroy_layout_layer_success
 * @brief               Test case of controller_destroy_layout_layer() where get_layer_from_id() success, return an object
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Mocking the get_layer_from_id() does return an object
 *                      -# Calling the controller_destroy_layout_layer()
 *                      -# Verification point:
 *                         +# layer_destroy() must be called once time
 */
TEST_F(ControllerTests, controller_destroy_layout_layer_success)
{
    struct wl_client * lClient{0xFFFFFFFF}; // a fake valid address
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    uint32_t lLayerId{1};

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);
    struct ivi_layout_layer *l_layoutLayer[1] = {(struct ivi_layout_layer *)0xFFFFFFFF}; // a fake valid address
    SET_RETURN_SEQ(get_layer_from_id, l_layoutLayer, 1);

    controller_destroy_layout_layer(lClient, lResource, lLayerId);

    ASSERT_EQ(layer_destroy_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             destroy_ivicontroller_screen_success
 * @brief               Test case of destroy_ivicontroller_screen() where valid input param
 * @test_procedure Steps:
 *                      -# Calling the destroy_ivicontroller_screen()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 */
TEST_F(ControllerTests, destroy_ivicontroller_screen_success)
{
    struct wl_resource * lResource{0xFFFFFFFF}; // a fake valid address
    destroy_ivicontroller_screen(lResource);
    ASSERT_EQ(wl_list_remove_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             output_destroyed_event_invalidOutput
 * @brief               Test case of output_destroyed_event() where invalid ivi shell
 * @test_procedure Steps:
 *                      -# Calling the output_destroyed_event()
 *                      -# Verification point:
 *                         +# weston_compositor_schedule_repaint() must be called once time
 *                         +# wl_list_remove() not be called
 */
TEST_F(ControllerTests, output_destroyed_event_invalidOutput)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    output_destroyed_event(&mp_iviShell->output_destroyed, lData);

    ASSERT_EQ(weston_compositor_schedule_repaint_fake.call_count, 1);
    ASSERT_EQ(wl_list_remove_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             output_destroyed_event_success
 * @brief               Test case of output_destroyed_event() where valid ivi shell
 * @test_procedure Steps:
 *                      -# Set data input for ivi shell
 *                      -# Calling the output_destroyed_event()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 2 times
 *                         +# wl_list_insert() must be called once time
 *                         +# Free resources are allocated when running the test
 *                      -# Free resources are allocated when running the test and set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, output_destroyed_event_success)
{
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));
    mp_iviShell->bkgnd_view->surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    mp_iviShell->bkgnd_view->surface->width = 1;
    mp_iviShell->bkgnd_view->surface->height = 1;
    mp_iviShell->bkgnd_view->surface->compositor = (struct weston_compositor*)malloc(sizeof(weston_compositor));
    custom_wl_list_init(&mp_iviShell->compositor->output_list);
    struct weston_output *l_output = (struct weston_output *)malloc(sizeof(struct weston_output));
    l_output->name = (char*)"default";
    l_output->id = 1;
    l_output->x = 1;
    l_output->y = 1;
    l_output->width = 1;
    l_output->height = 1;
    custom_wl_list_insert(&mp_iviShell->compositor->output_list, &l_output->link);
    custom_wl_list_init(&mp_iviShell->bkgnd_view->surface->compositor->output_list);
    struct weston_output *l_output2 = (struct weston_output *)malloc(sizeof(struct weston_output));
    l_output2->name = (char*)"default";
    l_output2->id = 1;
    l_output2->x = 1;
    l_output2->y = 1;
    l_output2->width = 1;
    l_output2->height = 1;
    custom_wl_list_insert(&mp_iviShell->bkgnd_view->surface->compositor->output_list, &l_output2->link);
    mp_iviShell->client = (struct wl_client*)&mp_fakeClient;

    output_destroyed_event(&mp_iviShell->output_destroyed, mp_iviScreen[0]->output);

    EXPECT_EQ(wl_list_remove_fake.call_count, 2);
    EXPECT_EQ(wl_list_insert_fake.call_count, 1);

    free(l_output);
    free(l_output2);
    free(mp_iviShell->bkgnd_view->surface->compositor);
    free(mp_iviShell->bkgnd_view->surface);
    free(mp_iviShell->bkgnd_view);
    mp_iviScreen[0] = NULL;
}

/** ================================================================================================
 * @test_id             output_resized_event_nullBkgndView
 * @brief               Test case of output_resized_event() where ivi shell bkgnd_view is null pointer
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with bkgnd_view is null pointer
 *                      -# Calling the output_resized_event()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(ControllerTests, output_resized_event_nullBkgndView)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    mp_iviShell->bkgnd_view = nullptr;
    mp_iviShell->client = (struct wl_client*)&mp_fakeClient;

    output_resized_event(&mp_iviShell->output_resized, lData);

    ASSERT_EQ(wl_list_remove_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             output_resized_event_nullClient
 * @brief               Test case of output_resized_event() where ivi shell client is null pointer
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with client is null pointer
 *                      -# Calling the output_resized_event()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, output_resized_event_nullClient)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));
    mp_iviShell->client = nullptr;

    output_resized_event(&mp_iviShell->output_resized, lData);

    EXPECT_EQ(wl_list_remove_fake.call_count, 0);

    free(mp_iviShell->bkgnd_view);
}

/** ================================================================================================
 * @test_id             output_resized_event_success
 * @brief               Test case of output_resized_event() where valid ivi shell bkgnd_view and client
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with valid bkgnd_view and client
 *                      -# Calling the output_resized_event()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called once time
 *                         +# wl_list_insert() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, output_resized_event_success)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));
    mp_iviShell->bkgnd_view->surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    mp_iviShell->bkgnd_view->surface->width = 1;
    mp_iviShell->bkgnd_view->surface->height = 1;
    mp_iviShell->bkgnd_view->surface->compositor = (struct weston_compositor*)malloc(sizeof(weston_compositor));
    custom_wl_list_init(&mp_iviShell->bkgnd_view->surface->compositor->output_list);
    struct weston_output *l_output = (struct weston_output *)malloc(sizeof(struct weston_output));
    l_output->name = (char*)"default";
    l_output->id = 1;
    l_output->x = 1;
    l_output->y = 1;
    l_output->width = 1;
    l_output->height = 1;
    custom_wl_list_insert(&mp_iviShell->compositor->output_list, &l_output->link);
    mp_iviShell->client = (struct wl_client*)&mp_fakeClient;

    output_resized_event(&mp_iviShell->output_resized, lData);

    EXPECT_EQ(wl_list_remove_fake.call_count, 1);
    EXPECT_EQ(wl_list_insert_fake.call_count, 1);

    free(l_output);
    free(mp_iviShell->bkgnd_view->surface->compositor);
    free(mp_iviShell->bkgnd_view->surface);
    free(mp_iviShell->bkgnd_view);
}

/** ================================================================================================
 * @test_id             surface_committed_success
 * @brief               Test case of surface_committed() where valid input params
 * @test_procedure Steps:
 *                      -# Set ivi surface frame_count to 0
 *                      -# Calling the surface_committed()
 *                      -# Verification point:
 *                         +# frame_count must be increased by 1
 */
TEST_F(ControllerTests, surface_committed_success)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    mp_iviSurface[0]->frame_count = 0;
    surface_committed(&mp_iviSurface[0]->committed, lData);

    ASSERT_EQ(mp_iviSurface[0]->frame_count, 1);
}

/** ================================================================================================
 * @test_id             layer_event_remove_wrongIviLayer
 * @brief               Test case of layer_event_remove() where input ivi layout layer is null pointer
 * @test_procedure Steps:
 *                      -# Calling the layer_event_remove()
 *                      -# Verification point:
 *                         +# wl_list_remove() not be called
 */
TEST_F(ControllerTests, layer_event_remove_wrongIviLayer)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    layer_event_remove(&mp_iviShell->layer_removed, lData);
    ASSERT_EQ(wl_list_remove_fake.call_count, 0);
}

/** ================================================================================================
 * @test_id             layer_event_remove_success
 * @brief               Test case of layer_event_remove() where valid input ivi layout layer
 * @test_procedure Steps:
 *                      -# Calling the layer_event_remove()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 4 times
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, layer_event_remove_success)
{
    layer_event_remove(&mp_iviShell->layer_removed, m_layoutLayer);

    EXPECT_EQ(wl_list_remove_fake.call_count, 4);

    mp_LayerNotification[0] = NULL;
    mp_iviLayer[0] = NULL;
}

/** ================================================================================================
 * @test_id             surface_event_remove_wrongIdSurface
 * @brief               Test case of surface_event_remove() where valid input ivi layout surfacey
 *                      but ivi shell bkgnd_surface_id is null pointer
 * @test_procedure Steps:
 *                      -# Calling the surface_event_remove()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 5 times
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, surface_event_remove_wrongIdSurface)
{
    surface_event_remove(&mp_iviShell->surface_removed, m_layoutSurface);

    EXPECT_EQ(wl_list_remove_fake.call_count, 5);

    mp_surfaceNotification[0] = NULL;
    mp_iviSurface[0] = NULL;
}

/** ================================================================================================
 * @test_id             surface_event_remove_success
 * @brief               Test case of surface_event_remove() where valid input ivi layout surfacey
 *                      and valid ivi shell bkgnd_surface_id
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with valid bkgnd_surface_id
 *                      -# Calling the surface_event_remove()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 5 times
 *                         +# weston_layer_entry_remove() not be called
 *                         +# weston_view_destroy() not be called
 *                      -# Free resources are allocated when running the test and set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, surface_event_remove_success)
{
    mp_iviShell->bkgnd_surface_id = 10;
    mp_iviShell->bkgnd_surface = mp_iviSurface[0];
    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));

    surface_event_remove(&mp_iviShell->surface_removed, m_layoutSurface);

    EXPECT_EQ(wl_list_remove_fake.call_count, 5);
    EXPECT_EQ(weston_layer_entry_remove_fake.call_count, 0);
    EXPECT_EQ(weston_view_destroy_fake.call_count, 0);

    free(mp_iviShell->bkgnd_view);
    mp_surfaceNotification[0] = NULL;
    mp_iviSurface[0] = NULL;
}

/** ================================================================================================
 * @test_id             surface_event_configure_wrongSurfaceId
 * @brief               Test case of surface_event_configure() where invalid ivi shell bkgnd_surface_id
 * @test_procedure Steps:
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the surface_event_configure()
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_configure_wrongSurfaceId)
{
    m_layoutSurface[0].surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    m_layoutSurface[0].surface->width = 1;
    m_layoutSurface[0].surface->height = 1;
    struct weston_surface* tmp[1] = {m_layoutSurface[0].surface};
    SET_RETURN_SEQ(surface_get_weston_surface, (struct weston_surface**)(tmp), 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    mp_iviShell->bkgnd_surface_id = 0;
    surface_event_configure(&mp_iviShell->surface_configured, &m_layoutSurface[0]);
    EXPECT_EQ(wl_resource_get_user_data_fake.call_count, 1);
    free(m_layoutSurface[0].surface);
}

/** ================================================================================================
 * @test_id             surface_event_configure_validBkgndView
 * @brief               Test case of surface_event_configure() where valid ivi shell bkgnd_surface_id
 *                      and valid bkgnd_view
 * @test_procedure Steps:
 *                      -# Set data for ivi shell
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the surface_event_configure()
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() not be called
 *                         +# wl_list_remove() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_configure_validBkgndView)
{
    mp_iviShell->bkgnd_surface_id = 10;
    mp_iviShell->bkgnd_view = (struct weston_view*)malloc(sizeof(struct weston_view));
    mp_iviShell->bkgnd_view->surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    mp_iviShell->bkgnd_view->surface->width = 1;
    mp_iviShell->bkgnd_view->surface->height = 1;
    mp_iviShell->bkgnd_view->surface->compositor = (struct weston_compositor*)malloc(sizeof(weston_compositor));
    custom_wl_list_init(&mp_iviShell->bkgnd_view->surface->compositor->output_list);
    uint32_t tmp[1] = {m_layoutSurface[0].id_surface};
    SET_RETURN_SEQ(get_id_of_surface, (uint32_t *)tmp, 1);
    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    surface_event_configure(&mp_iviShell->surface_configured, m_layoutSurface);

    EXPECT_EQ(wl_resource_get_user_data_fake.call_count, 0);
    EXPECT_EQ(wl_list_remove_fake.call_count, 1);
    EXPECT_EQ(surface_get_weston_surface_fake.call_count, 1);
    EXPECT_EQ(get_id_of_surface_fake.call_count, 1);
    EXPECT_EQ(weston_matrix_init_fake.call_count, 1);

    free(mp_iviShell->bkgnd_view->surface->compositor);
    free(mp_iviShell->bkgnd_view->surface);
    free(mp_iviShell->bkgnd_view);
}

/** ================================================================================================
 * @test_id             surface_event_configure_nullBkgndView
 * @brief               Test case of surface_event_configure() where valid ivi shell bkgnd_surface_id
 *                      and invalid bkgnd_view
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with bkgnd_view is null pointer
 *                      -# Mocking the weston_view_create() does return an object
 *                      -# Calling the surface_event_configure()
 *                      -# Verification point:
 *                         +# wl_resource_get_user_data() not be called
 *                         +# wl_list_remove() must be called once time
 *                         +# weston_view_create() must be called once time
 *                      -# Free resources are allocated when running the test
 */
TEST_F(ControllerTests, surface_event_configure_nullBkgndView)
{
    mp_iviShell->bkgnd_surface_id = 10;
    mp_iviShell->bkgnd_view = nullptr;
    struct weston_view *l_bkgnd_view[1];
    l_bkgnd_view[0] = (struct weston_view*)malloc(sizeof(struct weston_view));
    l_bkgnd_view[0]->surface = (struct weston_surface*)malloc(sizeof(struct weston_surface));
    l_bkgnd_view[0]->surface->width = 1;
    l_bkgnd_view[0]->surface->height = 1;
    l_bkgnd_view[0]->surface->compositor = (struct weston_compositor*)malloc(sizeof(weston_compositor));
    custom_wl_list_init(&l_bkgnd_view[0]->surface->compositor->output_list);

    uint32_t tmp[1] = {m_layoutSurface[0].id_surface};
    SET_RETURN_SEQ(get_id_of_surface, (uint32_t *)tmp, 1);
    SET_RETURN_SEQ(weston_view_create, l_bkgnd_view, 1);

    surface_event_configure(&mp_iviShell->surface_configured, m_layoutSurface);

    EXPECT_EQ(wl_resource_get_user_data_fake.call_count, 0);
    EXPECT_EQ(wl_list_remove_fake.call_count, 1);
    EXPECT_EQ(wl_list_init_fake.call_count, 1);
    EXPECT_EQ(weston_view_create_fake.call_count, 1);
    EXPECT_EQ(weston_layer_entry_insert_fake.call_count, 1);
    EXPECT_EQ(weston_surface_map_fake.call_count, 1);

    free(l_bkgnd_view[0]->surface->compositor);
    free(l_bkgnd_view[0]->surface);
    free(l_bkgnd_view[0]);
}

/** ================================================================================================
 * @test_id             ivi_shell_destroy_nullClient
 * @brief               Test case of ivi_shell_destroy() where ivi shell client is null pointer
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with client is null pointer
 *                      -# Calling the ivi_shell_destroy()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 16 times
 *                         +# Allocate memory for resources are freed when running the test
 *                      -# Free resources are allocated when running the test and set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, ivi_shell_destroy_nullClient)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    mp_iviShell->client = nullptr;
    mp_screenInfo->screen_name = (char *)malloc(30);

    ivi_shell_destroy(&mp_iviShell->destroy_listener, lData);

    EXPECT_EQ(wl_list_remove_fake.call_count, 16);
    EXPECT_EQ(wl_client_destroy_fake.call_count, 0);

    for(uint8_t i = 0; i < OBJECT_NUMBER; i++)
    {
        mp_iviSurface[i] = NULL;
        mp_iviLayer[i] = NULL;
        mp_iviScreen[i] = NULL;
    }
    mp_iviShell = NULL;
    free(mp_screenInfo);
}

/** ================================================================================================
 * @test_id             ivi_shell_destroy_success
 * @brief               Test case of ivi_shell_destroy() where ivi shell client and screen name are valid values
 * @test_procedure Steps:
 *                      -# Set data for ivi shell with client is valid value
 *                      -# Calling the ivi_shell_destroy()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 17 times
 *                         +# Allocate memory for resources are freed when running the test
 *                      -# Free resources are allocated when running the test and set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, ivi_shell_destroy_success)
{
    void * lData{0xFFFFFFFF}; // a fake valid address
    wl_resource_from_link_fake.custom_fake = custom_wl_resource_from_link;
    wl_resource_get_link_fake.custom_fake = custom_wl_resource_get_link;
    mp_iviShell->client = (struct wl_client *)0xFFFFFFFF; // a fake valid address
    mp_screenInfo->screen_name = (char *)malloc(30);

    ivi_shell_destroy(&mp_iviShell->destroy_listener, lData);

    EXPECT_EQ(wl_list_remove_fake.call_count, 17);

    for(uint8_t i = 0; i < OBJECT_NUMBER; i++)
    {
        mp_iviSurface[i] = NULL;
        mp_iviLayer[i] = NULL;
        mp_iviScreen[i] = NULL;
    }
    mp_iviShell = NULL;
    free(mp_screenInfo);
}

/** ================================================================================================
 * @test_id             launch_client_process_success
 * @brief               Test case of launch_client_process() where valid input params
 * @test_procedure Steps:
 *                      -# Calling the launch_client_process()
 *                      -# Verification point:
 *                         +# weston_client_start() must be called once time
 *                         +# wl_client_add_destroy_listener() must be called once time
 */
TEST_F(ControllerTests, launch_client_process_success)
{
    launch_client_process(&mp_iviShell);

    ASSERT_EQ(weston_client_start_fake.call_count, 1);
    ASSERT_EQ(wl_client_add_destroy_listener_fake.call_count, 1);
}

/** ================================================================================================
 * @test_id             unbind_resource_controller_success
 * @brief               Test case of unbind_resource_controller() where wl_resource_get_user_data() success, return an object
 * @test_procedure Steps:
 *                      -# Set data for layer_notifications and surface_notifications
 *                      -# Mocking the wl_resource_get_user_data() does return an object
 *                      -# Calling the unbind_resource_controller()
 *                      -# Verification point:
 *                         +# wl_list_remove() must be called 3 times
 *                      -# Set NULL for resources are freed when running the test
 */
TEST_F(ControllerTests, unbind_resource_controller_success)
{
    struct wl_resource *lResource{0xFFFFFFFF};

    struct notification *l_not = (struct notification *)malloc(sizeof(struct notification));
    custom_wl_list_init(&mp_iviController[0]->layer_notifications);
    custom_wl_list_insert(&mp_iviController[0]->layer_notifications, &l_not->link);
    custom_wl_list_init(&mp_iviController[0]->surface_notifications);

    SET_RETURN_SEQ(wl_resource_get_user_data, (void**)mp_iviController, 1);

    unbind_resource_controller(lResource);

    EXPECT_EQ(wl_list_remove_fake.call_count, 3);

    mp_iviController[0] = NULL;
}
