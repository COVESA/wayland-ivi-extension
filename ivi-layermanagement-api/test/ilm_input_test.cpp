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

class IlmInputTest : public TestBase, public ::testing::Test {
public:
    void SetUp()
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_initWithNativedisplay((t_ilm_nativedisplay)wlDisplay));

        iviSurfaces.reserve(10);
        struct iviSurface surf;
        for (int i = 0; i < (int)iviSurfaces.capacity(); ++i)
        {
            surf.surface = ivi_application_surface_create(iviApp, i+500, wlSurfaces[i]);
            surf.surface_id = i+500;
            iviSurfaces.push_back(surf);
        }

        wl_display_flush(wlDisplay);
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

        for (std::vector<iviSurface>::reverse_iterator it = iviSurfaces.rbegin();
             it != iviSurfaces.rend();
             ++it)
        {
            ivi_surface_destroy((*it).surface);
        }
        iviSurfaces.clear();

        EXPECT_EQ(ILM_SUCCESS, ilm_commitChanges());
        EXPECT_EQ(ILM_SUCCESS, ilm_destroy());
    }
};

TEST_F(IlmInputTest, ilm_input_focus) {
    const uint32_t surfaceCount = 4;
    t_ilm_surface surfaces[] = {iviSurfaces[0].surface_id,
                                iviSurfaces[1].surface_id,
                                iviSurfaces[2].surface_id,
                                iviSurfaces[3].surface_id};
    t_ilm_surface *surfaceIDs;
    ilmInputDevice *bitmasks;
    t_ilm_uint num_ids;
    t_ilm_uint layer = 0xbeef;

    /* We have to add surfaces to a layer. Otherwise, they would not have a view.
       We need a view to be able to set pointer focus. */
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));
    EXPECT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, surfaces, surfaceCount));

    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids));
    /* All the surfaces are returned */
    ASSERT_EQ(num_ids, iviSurfaces.size());
    int surfaces_found = 0;

    for (unsigned int i = 0; i < num_ids; i++) {
        /* The bitmasks all start unset */
        EXPECT_EQ(bitmasks[i], 0);
        if (contains(surfaceIDs, iviSurfaces.size(), iviSurfaces[i].surface_id))
            surfaces_found++;
    }
    free(surfaceIDs);
    free(bitmasks);
    /* The surfaces returned are the correct ones */
    ASSERT_EQ(surfaces_found, iviSurfaces.size());

    /* Can set all focus to keyboard */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputFocus(&surfaces[0], surfaceCount, ILM_INPUT_DEVICE_KEYBOARD, ILM_TRUE));

    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids));
    for (unsigned int i = 0; i < num_ids; i++) {
        /* All surfaces now have keyboard focus */
        for (unsigned int j = 0; j < surfaceCount; j++) {
            if (surfaceIDs[i] == surfaces[j])
                EXPECT_EQ(bitmasks[i], ILM_INPUT_DEVICE_KEYBOARD);
        }
    }
    free(surfaceIDs);
    free(bitmasks);

    /* Can remove keyboard focus from one surface */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputFocus(&surfaces[0], 1, ILM_INPUT_DEVICE_KEYBOARD, ILM_FALSE));
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids));
    /* keyboard focus now removed for surfaces[0] */
    for (unsigned int i = 0; i < num_ids; i++)
        if (surfaceIDs[i] == surfaces[0])
            EXPECT_EQ(bitmasks[i], 0);
    free(surfaceIDs);
    free(bitmasks);

    /* Pointer focus set for surfaces[1] */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputFocus(&surfaces[1], 1, ILM_INPUT_DEVICE_POINTER, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids));
    /* surfaces[1] now has pointer and keyboard focus */
    for (unsigned int i = 0; i < num_ids; i++)
        if (surfaceIDs[i] == surfaces[1])
            EXPECT_EQ(bitmasks[i], ILM_INPUT_DEVICE_POINTER | ILM_INPUT_DEVICE_KEYBOARD);
    free(surfaceIDs);
    free(bitmasks);

    /* Touch focus set for surfaces[2] */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputFocus(&surfaces[2], 1, ILM_INPUT_DEVICE_TOUCH, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputFocus(&surfaceIDs, &bitmasks, &num_ids));
    /* surfaces[2] now has keyboard and touch focus */
    for (unsigned int i = 0; i < num_ids; i++)
        if (surfaceIDs[i] == surfaces[2])
            EXPECT_EQ(bitmasks[i], ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_TOUCH);
    free(surfaceIDs);
    free(bitmasks);
}

TEST_F(IlmInputTest, ilm_input_event_acceptance) {
    t_ilm_surface surface1 = iviSurfaces[0].surface_id;
    t_ilm_uint num_seats = 0;
    t_ilm_string *seats = NULL;
    char const *set_seats = "default";
    t_ilm_uint set_seats_count = 1;

    /* All seats accept the "default" seat when created */
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputAcceptanceOn(surface1, &num_seats,
                                                    &seats));
    EXPECT_EQ(1, num_seats);
    /* googletest doesn't like comparing to null pointers */
    ASSERT_FALSE(seats == NULL);
    EXPECT_STREQ("default", seats[0]);
    free(seats[0]);
    free(seats);

    /* Can remove a seat from acceptance */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(surface1, 0, NULL));
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputAcceptanceOn(surface1, &num_seats,
                                                    &seats));
    EXPECT_EQ(0, num_seats);
    free(seats);

    /* Can add a seat to acceptance */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(surface1, set_seats_count,
                                                    (t_ilm_string*)&set_seats));
    ASSERT_EQ(ILM_SUCCESS, ilm_getInputAcceptanceOn(surface1, &num_seats,
                                                    &seats));
    EXPECT_EQ(set_seats_count, num_seats);
    bool found = false;
    if (!strcmp(*seats, (t_ilm_string)set_seats))
        found = true;
    EXPECT_EQ(true, found) << set_seats[0] << " not found in returned seats";

    free(seats[0]);
    free(seats);

    /* Seats can be set, unset, then reset */
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(surface1, 1, (t_ilm_string*)&set_seats));
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(surface1, 0, NULL));
    ASSERT_EQ(ILM_SUCCESS, ilm_setInputAcceptanceOn(surface1, 1, (t_ilm_string*)&set_seats));
}
