/***************************************************************************
 *
 * Copyright 2010-2014 BMW Car IT GmbH
 * Copyright (C) 2012 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
 * Copyright (C) 2016 Advanced Driver Information Technology Joint Venture GmbH
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
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>

#include <iostream>

#include "TestBase.h"

extern "C" {
    #include "ilm_control.h"
    #include "ilm_input.h"
}

template <typename T>
bool contains(T const *actual, size_t as, T expected)
{
   for (unsigned i = 0; i < as; i++)
      if (actual[i] == expected)
         return true;
   return false;
}

class IlmNullPointerTest : public TestBase, public ::testing::Test {
public:
    void SetUp()
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_initWithNativedisplay((t_ilm_nativedisplay)wlDisplay));
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

        EXPECT_EQ(ILM_SUCCESS, ilm_commitChanges());
        EXPECT_EQ(ILM_SUCCESS, ilm_destroy());
    }
};

TEST_F(IlmNullPointerTest, ilm_set_input_focus_null_pointer) {
    ASSERT_EQ(ILM_FAILED, ilm_setInputFocus(NULL, 5, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));
}

TEST_F(IlmNullPointerTest, ilm_get_input_focus_null_pointer) {
    const uint32_t surfaceCount = 4;
    t_ilm_surface surfaces[] = {1010, 2020, 3030, 4040};
    struct ivi_surface* ivi_surfaces[surfaceCount];
    t_ilm_surface *surfaceIDs;
    ilmInputDevice *bitmasks;
    t_ilm_uint num_ids;

    for (unsigned int i = 0; i < surfaceCount; i++) {
        ivi_surfaces[i] = (struct ivi_surface*) ivi_application_surface_create(iviApp, surfaces[i], wlSurfaces[i]);
    }

    EXPECT_EQ(ILM_FAILED, ilm_getInputFocus(0, &bitmasks, &num_ids));

    EXPECT_EQ(ILM_FAILED, ilm_getInputFocus(&surfaceIDs, NULL, &num_ids));

    EXPECT_EQ(ILM_FAILED, ilm_getInputFocus(&surfaceIDs, &bitmasks, NULL));

    for (unsigned int i = 0; i < surfaceCount; i++) {
        ivi_surface_destroy(ivi_surfaces[i]);
    }
}

TEST_F(IlmNullPointerTest, ilm_set_input_event_acceptance_null_pointer) {
    t_ilm_surface surface1 = 1010;
    char const *set_seats = "default";
    t_ilm_uint set_seats_count = 1;

    struct ivi_surface* ivi_surface = (struct ivi_surface*)
        ivi_application_surface_create(iviApp, surface1, wlSurfaces[0]);

    EXPECT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(surface1, set_seats_count, NULL));

    EXPECT_EQ(ILM_FAILED, ilm_setInputAcceptanceOn(0, set_seats_count, (t_ilm_string*)&set_seats));

    ivi_surface_destroy(ivi_surface);
}

TEST_F(IlmNullPointerTest, ilm_get_input_event_acceptance_null_pointer) {
    t_ilm_surface surface1 = 1010;
    t_ilm_uint num_seats = 0;
    t_ilm_string *seats = NULL;

    struct ivi_surface* ivi_surface = (struct ivi_surface*)
        ivi_application_surface_create(iviApp, surface1, wlSurfaces[0]);

    EXPECT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(0, &num_seats,
                                                    NULL));

    EXPECT_EQ(ILM_FAILED, ilm_getInputAcceptanceOn(surface1, NULL,
                                                    &seats));

    ivi_surface_destroy(ivi_surface);
}

TEST_F(IlmNullPointerTest, ilm_get_input_devices_null_pointer) {
    t_ilm_surface surface1 = 1010;
    t_ilm_string *seats = NULL;
    t_ilm_uint num_seats = 0;
    ilmInputDevice bitmask = ILM_INPUT_DEVICE_POINTER;

    struct ivi_surface* ivi_surface = (struct ivi_surface*)
        ivi_application_surface_create(iviApp, surface1, wlSurfaces[0]);

    EXPECT_EQ(ILM_FAILED, ilm_getInputDevices(bitmask, &num_seats,
                                                    NULL));

    EXPECT_EQ(ILM_FAILED, ilm_getInputDevices(bitmask, NULL,
                                                    &seats));

    ivi_surface_destroy(ivi_surface);
}

TEST_F(IlmNullPointerTest, ilm_get_input_device_capabilities_null_pointer) {
	char const *seats = "default";
    ilmInputDevice bitmask;

    EXPECT_EQ(ILM_FAILED, ilm_getInputDeviceCapabilities(NULL, &bitmask));
    EXPECT_EQ(ILM_FAILED, ilm_getInputDeviceCapabilities((t_ilm_string)seats, NULL));
}
