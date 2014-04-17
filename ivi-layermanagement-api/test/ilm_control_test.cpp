/***************************************************************************
 *
 * Copyright 2010-2014 BMW Car IT GmbH
 * Copyright (C) 2012 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#include "TestBase.h"

extern "C" {
    #include "ilm_client.h"
    #include "ilm_control.h"
}

class IlmCommandTest : public TestBase, public ::testing::Test {
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

        t_ilm_surface* surfaces = NULL;
        t_ilm_int numSurfaces=0;
        EXPECT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&numSurfaces, &surfaces));
        for (t_ilm_int i=0; i<numSurfaces; i++)
        {
            EXPECT_EQ(ILM_SUCCESS, ilm_surfaceRemove(surfaces[i]));
        };

        EXPECT_EQ(ILM_SUCCESS, ilm_commitChanges());
        EXPECT_EQ(ILM_SUCCESS, ilm_destroy());
    }
};

TEST_F(IlmCommandTest, SetGetSurfaceDimension) {
    uint surface = 36;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 10, 10, ILM_PIXELFORMAT_RGBA_8888, &surface));

    t_ilm_uint dim[2] = {15, 25};
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetDimension(surface, dim));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_uint dimreturned[2];
    EXPECT_EQ(ILM_SUCCESS, ilm_surfaceGetDimension(surface, dimreturned));
    EXPECT_EQ(dim[0], dimreturned[0]);
    EXPECT_EQ(dim[1], dimreturned[1]);
}

TEST_F(IlmCommandTest, SetGetLayerDimension) {
    uint layer = 4316;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));

    t_ilm_uint dim[2] = {115, 125};
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetDimension(layer, dim));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_uint dimreturned[2];
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetDimension(layer, dimreturned));
    EXPECT_EQ(dim[0], dimreturned[0]);
    EXPECT_EQ(dim[1], dimreturned[1]);
}

TEST_F(IlmCommandTest, SetGetSurfacePosition) {
    uint surface = 37;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 10, 10, ILM_PIXELFORMAT_RGBA_8888, &surface));

    t_ilm_uint pos[2] = {15, 25};
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetPosition(surface, pos));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_uint posreturned[2];
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPosition(surface, posreturned));
    EXPECT_EQ(pos[0], posreturned[0]);
    EXPECT_EQ(pos[1], posreturned[1]);
}

TEST_F(IlmCommandTest, SetGetLayerPosition) {
    uint layer = 4316;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));

    t_ilm_uint pos[2] = {115, 125};
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetPosition(layer, pos));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_uint posreturned[2];
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetPosition(layer, posreturned));
    ASSERT_EQ(pos[0], posreturned[0]);
    ASSERT_EQ(pos[1], posreturned[1]);
}

TEST_F(IlmCommandTest, SetGetSurfaceOrientation) {
    uint surface = 36;
    ilmOrientation returned;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface));

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_NINETY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOrientation(surface, &returned));
    ASSERT_EQ(ILM_NINETY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_ONEHUNDREDEIGHTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOrientation(surface, &returned));
    ASSERT_EQ(ILM_ONEHUNDREDEIGHTY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_TWOHUNDREDSEVENTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOrientation(surface, &returned));
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_ZERO));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOrientation(surface, &returned));
    ASSERT_EQ(ILM_ZERO, returned);
}

TEST_F(IlmCommandTest, SetGetLayerOrientation) {
    uint layer = 4316;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ilmOrientation returned;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_NINETY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOrientation(layer, &returned));
    ASSERT_EQ(ILM_NINETY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_ONEHUNDREDEIGHTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOrientation(layer, &returned));
    ASSERT_EQ(ILM_ONEHUNDREDEIGHTY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_TWOHUNDREDSEVENTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOrientation(layer, &returned));
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY, returned);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_ZERO));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOrientation(layer, &returned));
    ASSERT_EQ(ILM_ZERO, returned);
}

TEST_F(IlmCommandTest, SetGetSurfaceOpacity) {
    uint surface1 = 36;
    uint surface2 = 44;
    t_ilm_float opacity;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOpacity(surface1, 0.88));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOpacity(surface1, &opacity));
    EXPECT_FLOAT_EQ(0.88, opacity);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOpacity(surface2, 0.001));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetOpacity(surface2, &opacity));
    EXPECT_FLOAT_EQ(0.001, opacity);
}

TEST_F(IlmCommandTest, SetGetLayerOpacity) {
    uint layer1 = 36;
    uint layer2 = 44;
    t_ilm_float opacity;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer1, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer2, 800, 480));

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOpacity(layer1, 0.88));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOpacity(layer1, &opacity));
    EXPECT_FLOAT_EQ(0.88, opacity);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOpacity(layer2, 0.001));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetOpacity(layer2, &opacity));
    EXPECT_FLOAT_EQ(0.001, opacity);
}

TEST_F(IlmCommandTest, SetGetSurfaceVisibility) {
    uint surface1 = 36;
    t_ilm_bool visibility;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(surface1, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetVisibility(surface1, &visibility));
    ASSERT_EQ(ILM_TRUE, visibility);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(surface1, ILM_FALSE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetVisibility(surface1, &visibility));
    ASSERT_EQ(ILM_FALSE, visibility);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(surface1, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetVisibility(surface1, &visibility));
    ASSERT_EQ(ILM_TRUE, visibility);
}

TEST_F(IlmCommandTest, SetGetLayerVisibility) {
    uint layer1 = 36;
    t_ilm_bool visibility;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer1, 800, 480));

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(layer1, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetVisibility(layer1, &visibility));
    ASSERT_EQ(ILM_TRUE, visibility);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(layer1, ILM_FALSE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetVisibility(layer1, &visibility));
    ASSERT_EQ(ILM_FALSE, visibility);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(layer1, ILM_TRUE));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_layerGetVisibility(layer1, &visibility));
    ASSERT_EQ(ILM_TRUE, visibility);
}

TEST_F(IlmCommandTest, ilm_getScreenIDs) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&numberOfScreens, &screenIDs));
    ASSERT_GT(numberOfScreens, 0u);
}

TEST_F(IlmCommandTest, ilm_getScreenResolution) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&numberOfScreens, &screenIDs));
    ASSERT_TRUE(numberOfScreens>0);

    uint firstScreen = screenIDs[0];
    t_ilm_uint width = 0, height = 0;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenResolution(firstScreen, &width, &height));
    ASSERT_GT(width, 0u);
    ASSERT_GT(height, 0u);
}

TEST_F(IlmCommandTest, ilm_getLayerIDs) {
    uint layer1 = 3246;
    uint layer2 = 46586;

    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer1, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer2, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&length, &IDs));

    ASSERT_EQ(layer1, IDs[0]);
    ASSERT_EQ(layer2, IDs[1]);
}

TEST_F(IlmCommandTest, ilm_getLayerIDsOfScreen) {
    t_ilm_layer layer1 = 3246;
    t_ilm_layer layer2 = 46586;
    t_ilm_uint roLength = 2;
    t_ilm_layer idRenderOrder[2] = {layer1, layer2};
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer1, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer2, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_displaySetRenderOrder(0, idRenderOrder, roLength));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length = 0;
    t_ilm_layer* IDs = 0;
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDsOnScreen(0, &length, &IDs));

    ASSERT_EQ(2, length);
    EXPECT_EQ(layer1, IDs[0]);
    EXPECT_EQ(layer2, IDs[1]);
}

TEST_F(IlmCommandTest, ilm_getSurfaceIDs) {
    uint surface1 = 3246;
    uint surface2 = 46586;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&length, &IDs));

    ASSERT_EQ(2, length);
    ASSERT_EQ(surface1, IDs[0]);
    ASSERT_EQ(surface2, IDs[1]);
}

TEST_F(IlmCommandTest, ilm_surfaceCreate_Remove) {
    uint surface1 = 3246;
    uint surface2 = 46586;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&length, &IDs));

    ASSERT_EQ(length, 2);
    ASSERT_EQ(surface1, IDs[0]);
    ASSERT_EQ(surface2, IDs[1]);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceRemove(surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&length, &IDs));
    ASSERT_EQ(length, 1);
    ASSERT_EQ(surface2, IDs[0]);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceRemove(surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&length, &IDs));
    ASSERT_EQ(length, 0);
}

TEST_F(IlmCommandTest, ilm_layerCreate_Remove) {
    uint layer1 = 3246;
    uint layer2 = 46586;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer1, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer2, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&length, &IDs));

    ASSERT_EQ(length, 2);
    ASSERT_EQ(layer1, IDs[0]);
    ASSERT_EQ(layer2, IDs[1]);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemove(layer1));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&length, &IDs));
    ASSERT_EQ(length, 1);
    ASSERT_EQ(layer2, IDs[0]);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemove(layer2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getLayerIDs(&length, &IDs));
    ASSERT_EQ(length, 0);
}

TEST_F(IlmCommandTest, ilm_surface_initialize) {
    uint surface_10 = 10;
    uint surface_20 = 20;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceInitialize(&surface_10));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceInitialize(&surface_20));

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDs(&length, &IDs));

    ASSERT_EQ(length, 2);
    ASSERT_EQ(surface_10, IDs[0]);
    ASSERT_EQ(surface_20, IDs[1]);
}


TEST_F(IlmCommandTest, ilm_layerAddSurface_ilm_layerRemoveSurface_ilm_getSurfaceIDsOnLayer) {
    uint layer = 3246;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));
    uint surface1 = 3246;
    uint surface2 = 46586;
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_int length;
    t_ilm_uint* IDs;
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &length, &IDs));
    ASSERT_EQ(length, 0);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddSurface(layer, surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &length, &IDs));
    ASSERT_EQ(length, 1);
    ASSERT_EQ(surface1, IDs[0]);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerAddSurface(layer, surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &length, &IDs));
    ASSERT_EQ(length, 2);
    ASSERT_EQ(surface1, IDs[0]);
    ASSERT_EQ(surface2, IDs[1]);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemoveSurface(layer, surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &length, &IDs));
    ASSERT_EQ(length, 1);
    ASSERT_EQ(surface2, IDs[0]);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerRemoveSurface(layer, surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &length, &IDs));
    ASSERT_EQ(length, 0);
}

TEST_F(IlmCommandTest, ilm_getPropertiesOfSurface_ilm_surfaceSetSourceRectangle_ilm_surfaceSetDestinationRectangle_ilm_surfaceSetChromaKey) {
    t_ilm_uint surface;
    t_ilm_int chromaKey[3] = {3, 22, 111};
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOpacity(surface, 0.8765));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetSourceRectangle(surface, 89, 6538, 638, 4));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetDestinationRectangle(surface, 54, 47, 947, 9));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_NINETY));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(surface, true));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetChromaKey(surface, &chromaKey[0]));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ilmSurfaceProperties surfaceProperties;
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfSurface(surface, &surfaceProperties));
    ASSERT_EQ(0.8765, surfaceProperties.opacity);
    ASSERT_EQ(89u, surfaceProperties.sourceX);
    ASSERT_EQ(6538u, surfaceProperties.sourceY);
    ASSERT_EQ(638u, surfaceProperties.sourceWidth);
    ASSERT_EQ(4u, surfaceProperties.sourceHeight);
    ASSERT_EQ(54u, surfaceProperties.destX);
    ASSERT_EQ(47u, surfaceProperties.destY);
    ASSERT_EQ(947u, surfaceProperties.destWidth);
    ASSERT_EQ(9u, surfaceProperties.destHeight);
    ASSERT_EQ(ILM_NINETY, surfaceProperties.orientation);
    ASSERT_TRUE( surfaceProperties.visibility);
    ASSERT_TRUE( surfaceProperties.chromaKeyEnabled);
    ASSERT_EQ(3u, surfaceProperties.chromaKeyRed);
    ASSERT_EQ(22u, surfaceProperties.chromaKeyGreen);
    ASSERT_EQ(111u, surfaceProperties.chromaKeyBlue);

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOpacity(surface, 0.436));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetSourceRectangle(surface, 784, 546, 235, 78));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetDestinationRectangle(surface, 536, 5372, 3, 4316));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetOrientation(surface, ILM_TWOHUNDREDSEVENTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetVisibility(surface, false));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceSetChromaKey(surface, NULL));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ilmSurfaceProperties surfaceProperties2;
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfSurface(surface, &surfaceProperties2));
    ASSERT_EQ(0.436, surfaceProperties2.opacity);
    ASSERT_EQ(784u, surfaceProperties2.sourceX);
    ASSERT_EQ(546u, surfaceProperties2.sourceY);
    ASSERT_EQ(235u, surfaceProperties2.sourceWidth);
    ASSERT_EQ(78u, surfaceProperties2.sourceHeight);
    ASSERT_EQ(536u, surfaceProperties2.destX);
    ASSERT_EQ(5372u, surfaceProperties2.destY);
    ASSERT_EQ(3u, surfaceProperties2.destWidth);
    ASSERT_EQ(4316u, surfaceProperties2.destHeight);
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY, surfaceProperties2.orientation);
    ASSERT_FALSE(surfaceProperties2.visibility);
    ASSERT_FALSE(surfaceProperties2.chromaKeyEnabled);
}

TEST_F(IlmCommandTest, ilm_getPropertiesOfLayer_ilm_layerSetSourceRectangle_ilm_layerSetDestinationRectangle_ilm_layerSetChromaKey) {
    t_ilm_uint layer;
    t_ilm_int chromaKey[3] = {3, 22, 111};
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOpacity(layer, 0.8765));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetSourceRectangle(layer, 89, 6538, 638, 4));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetDestinationRectangle(layer, 54, 47, 947, 9));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_NINETY));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(layer, true));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetChromaKey(layer, chromaKey));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ilmLayerProperties layerProperties1;
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfLayer(layer, &layerProperties1));
    ASSERT_EQ(0.8765, layerProperties1.opacity);
    ASSERT_EQ(89u, layerProperties1.sourceX);
    ASSERT_EQ(6538u, layerProperties1.sourceY);
    ASSERT_EQ(638u, layerProperties1.sourceWidth);
    ASSERT_EQ(4u, layerProperties1.sourceHeight);
    ASSERT_EQ(54u, layerProperties1.destX);
    ASSERT_EQ(47u, layerProperties1.destY);
    ASSERT_EQ(947u, layerProperties1.destWidth);
    ASSERT_EQ(9u, layerProperties1.destHeight);
    ASSERT_EQ(ILM_NINETY, layerProperties1.orientation);
    ASSERT_TRUE( layerProperties1.visibility);
    ASSERT_TRUE(layerProperties1.chromaKeyEnabled);
    ASSERT_EQ(3u,   layerProperties1.chromaKeyRed);
    ASSERT_EQ(22u,  layerProperties1.chromaKeyGreen);
    ASSERT_EQ(111u, layerProperties1.chromaKeyBlue);

    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOpacity(layer, 0.436));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetSourceRectangle(layer, 784, 546, 235, 78));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetDestinationRectangle(layer, 536, 5372, 3, 4316));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetOrientation(layer, ILM_TWOHUNDREDSEVENTY));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetVisibility(layer, false));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetChromaKey(layer, NULL));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ilmLayerProperties layerProperties2;
    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfLayer(layer, &layerProperties2));
    ASSERT_EQ(0.436, layerProperties2.opacity);
    ASSERT_EQ(784u, layerProperties2.sourceX);
    ASSERT_EQ(546u, layerProperties2.sourceY);
    ASSERT_EQ(235u, layerProperties2.sourceWidth);
    ASSERT_EQ(78u, layerProperties2.sourceHeight);
    ASSERT_EQ(536u, layerProperties2.destX);
    ASSERT_EQ(5372u, layerProperties2.destY);
    ASSERT_EQ(3u, layerProperties2.destWidth);
    ASSERT_EQ(4316u, layerProperties2.destHeight);
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY, layerProperties2.orientation);
    ASSERT_FALSE(layerProperties2.visibility);
    ASSERT_FALSE(layerProperties2.chromaKeyEnabled);
}

TEST_F(IlmCommandTest, ilm_takeScreenshot) {
    // make sure the file is not there before
    FILE* f = fopen("/tmp/test.bmp", "r");
    if (f!=NULL){
        fclose(f);
        int result = remove("/tmp/test.bmp");
        ASSERT_EQ(0, result);
    }

    ASSERT_EQ(ILM_SUCCESS, ilm_takeScreenshot(0, "/tmp/test.bmp"));

    sleep(1);
    f = fopen("/tmp/test.bmp", "r");
    ASSERT_TRUE(f!=NULL);
    fclose(f);
}

TEST_F(IlmCommandTest, ilm_surfaceGetPixelformat) {
    t_ilm_uint surface1=0;
    t_ilm_uint surface2=1;
    t_ilm_uint surface3=2;
    t_ilm_uint surface4=3;
    t_ilm_uint surface5=4;
    t_ilm_uint surface6=5;
    t_ilm_uint surface7=6;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_4444, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_5551, &surface2));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_6661, &surface3));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface4));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGB_565, &surface5));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGB_888, &surface6));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_R_8, &surface7));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ilmPixelFormat p1, p2, p3, p4, p5, p6, p7;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface1, &p1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface2, &p2));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface3, &p3));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface4, &p4));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface5, &p5));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface6, &p6));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceGetPixelformat(surface7, &p7));

    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_4444, p1);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_5551, p2);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_6661, p3);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_8888, p4);
    ASSERT_EQ(ILM_PIXELFORMAT_RGB_565, p5);
    ASSERT_EQ(ILM_PIXELFORMAT_RGB_888, p6);
    ASSERT_EQ(ILM_PIXELFORMAT_R_8, p7);
}

TEST_F(IlmCommandTest, ilm_keyboard_focus)
{
    uint surface;
    uint surface1 = 36;
    uint surface2 = 44;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));

    ASSERT_EQ(ILM_SUCCESS, ilm_GetKeyboardFocusSurfaceId(&surface));
    EXPECT_EQ(0xFFFFFFFF, surface);

    ASSERT_EQ(ILM_SUCCESS, ilm_SetKeyboardFocusOn(surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_GetKeyboardFocusSurfaceId(&surface));
    EXPECT_EQ(surface1, surface);
}


TEST_F(IlmCommandTest, ilm_input_event_acceptance)
{
    uint surface;
    uint surface1 = 36;
    uint surface2 = 44;
    ilmSurfaceProperties surfaceProperties;

    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 0, 0, ILM_PIXELFORMAT_RGBA_8888, &surface2));

    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfSurface(surface1, &surfaceProperties));
    EXPECT_EQ(ILM_INPUT_DEVICE_ALL, surfaceProperties.inputDevicesAcceptance);

    ASSERT_EQ(ILM_SUCCESS, ilm_UpdateInputEventAcceptanceOn(surface1, (ilmInputDevice) (ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_POINTER), false));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());


    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfSurface(surface1, &surfaceProperties));
    EXPECT_FALSE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_KEYBOARD);
    EXPECT_FALSE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_POINTER);
    EXPECT_TRUE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_TOUCH);

    ASSERT_EQ(ILM_SUCCESS, ilm_SetKeyboardFocusOn(surface1));
    ASSERT_EQ(ILM_SUCCESS, ilm_GetKeyboardFocusSurfaceId(&surface));
    EXPECT_NE(surface1, surface);
}

TEST_F(IlmCommandTest, SetGetOptimizationMode) {
    ilmOptimization id;
    ilmOptimizationMode mode;
    ilmOptimizationMode retmode;

    id = ILM_OPT_MULTITEXTURE;
    mode = ILM_OPT_MODE_FORCE_OFF;
    ASSERT_EQ(ILM_SUCCESS, ilm_SetOptimizationMode(id, mode));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_GetOptimizationMode(id, &retmode));
    ASSERT_EQ(mode, retmode);

    id = ILM_OPT_SKIP_CLEAR;
    mode = ILM_OPT_MODE_TOGGLE;
    ASSERT_EQ(ILM_SUCCESS, ilm_SetOptimizationMode(id, mode));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_GetOptimizationMode(id, &retmode));
    ASSERT_EQ(mode, retmode);

    id = ILM_OPT_MULTITEXTURE;
    mode = ILM_OPT_MODE_HEURISTIC;
    ASSERT_EQ(ILM_SUCCESS, ilm_SetOptimizationMode(id, mode));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_GetOptimizationMode(id, &retmode));
    ASSERT_EQ(mode, retmode);
}

TEST_F(IlmCommandTest, ilm_getPropertiesOfScreen) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&numberOfScreens, &screenIDs));
    ASSERT_TRUE(numberOfScreens>0);

    t_ilm_display screen = screenIDs[0];
    ilmScreenProperties screenProperties;

    t_ilm_layer layerIds[3] = {100, 200, 300};//t_ilm_layer layerIds[3] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(layerIds, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(layerIds + 1, 800, 480));
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(layerIds + 2, 800, 480));

    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    ASSERT_EQ(ILM_SUCCESS, ilm_displaySetRenderOrder(screen, layerIds, 3));

    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());


    ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfScreen(screen, &screenProperties));
    ASSERT_EQ(3, screenProperties.layerCount);
    ASSERT_EQ(layerIds[0], screenProperties.layerIds[0]);
    ASSERT_EQ(layerIds[1], screenProperties.layerIds[1]);
    ASSERT_EQ(layerIds[2], screenProperties.layerIds[2]);

    ASSERT_GT(screenProperties.screenWidth, 0u);
    ASSERT_GT(screenProperties.screenHeight, 0u);

    t_ilm_uint numberOfHardwareLayers;
    ASSERT_EQ(ILM_SUCCESS, ilm_getNumberOfHardwareLayers(screen, &numberOfHardwareLayers));
    ASSERT_EQ(numberOfHardwareLayers, screenProperties.harwareLayerCount);
}

TEST_F(IlmCommandTest, DisplaySetRenderOrder_growing) {
    //prepare needed layers
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(renderOrder + i, 300, 300));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    for(unsigned int i = 0; i < screenCount; ++i)
    {
        t_ilm_display screen = screenIDs[i];
        ilmScreenProperties screenProps;

        //trying different render orders with increasing sizes
        for (unsigned int j = layerCount; j <= layerCount; --j) // note: using overflow here
        {
            //put them from end to beginning, so that in each loop iteration the order of layers change
            ASSERT_EQ(ILM_SUCCESS, ilm_displaySetRenderOrder(screen, renderOrder + j, layerCount - j));
            ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
            ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfScreen(screen, &screenProps));

            ASSERT_EQ(layerCount - j, screenProps.layerCount);
            for(unsigned int k = 0; k < layerCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], screenProps.layerIds[k]);
            }
        }
    }
}

TEST_F(IlmCommandTest, DisplaySetRenderOrder_shrinking) {
    //prepare needed layers
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(renderOrder + i, 300, 300));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    for(unsigned int i = 0; i < screenCount; ++i)
    {
        t_ilm_display screen = screenIDs[i];
        ilmScreenProperties screenProps;

        //trying different render orders with decreasing sizes
        for (unsigned int j = 0; j <= layerCount; ++j)
        {
            //put them from end to beginning, so that in each loop iteration the order of layers change
            ASSERT_EQ(ILM_SUCCESS, ilm_displaySetRenderOrder(screen, renderOrder + j, layerCount - j));
            ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
            ASSERT_EQ(ILM_SUCCESS, ilm_getPropertiesOfScreen(screen, &screenProps));

            ASSERT_EQ(layerCount - j, screenProps.layerCount);
            for(unsigned int k = 0; k < layerCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], screenProps.layerIds[k]);
            }
        }
    }
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_growing) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 100, 100, ILM_PIXELFORMAT_RGBA_8888, renderOrder + i));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_layer layerIDs[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(layerIDs) / sizeof(layerIDs[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(layerIDs + i, 300, 300));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    for(unsigned int i = 0; i < layerCount; ++i)
    {
        t_ilm_layer layer = layerIDs[i];

        t_ilm_int layerSurfaceCount;
        t_ilm_surface* layerSurfaceIDs;

        //trying different render orders with increasing sizes
        for (unsigned int j = surfaceCount; j <= surfaceCount; --j) // note: using overflow here
        {
            //put them from end to beginning, so that in each loop iteration the order of surafces change
            ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder + j, surfaceCount - j));
            ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
            ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs));

            ASSERT_EQ(surfaceCount - j, layerSurfaceCount);
            for(unsigned int k = 0; k < surfaceCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], layerSurfaceIDs[k]);
            }
        }

        //set empty render order again
        ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder, 0));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_shrinking) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 100, 100, ILM_PIXELFORMAT_RGBA_8888, renderOrder + i));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_layer layerIDs[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(layerIDs) / sizeof(layerIDs[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(layerIDs + i, 300, 300));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    for(unsigned int i = 0; i < layerCount; ++i)
    {
        t_ilm_layer layer = layerIDs[i];

        t_ilm_int layerSurfaceCount;
        t_ilm_surface* layerSurfaceIDs;

        //trying different render orders with decreasing sizes
        for (unsigned int j = 0; j <= layerCount; ++j)
        {
            //put them from end to beginning, so that in each loop iteration the order of surafces change
            ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder + j, surfaceCount - j));
            ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
            ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs));

            ASSERT_EQ(surfaceCount - j, layerSurfaceCount);
            for(unsigned int k = 0; k < surfaceCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], layerSurfaceIDs[k]);
            }
        }

        //set empty render order again
        ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder, 0));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_duplicates) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 100, 100, ILM_PIXELFORMAT_RGBA_8888, renderOrder + i));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_surface duplicateRenderOrder[] = {renderOrder[0], renderOrder[1], renderOrder[0], renderOrder[1], renderOrder[0]};
    t_ilm_int duplicateSurfaceCount = sizeof(duplicateRenderOrder) / sizeof(duplicateRenderOrder[0]);

    t_ilm_layer layer;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 300, 300));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    t_ilm_int layerSurfaceCount;
    t_ilm_surface* layerSurfaceIDs;

    //trying duplicates
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, duplicateRenderOrder, duplicateSurfaceCount));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs));

    ASSERT_EQ(2, layerSurfaceCount);
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_empty) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ASSERT_EQ(ILM_SUCCESS, ilm_surfaceCreate((t_ilm_nativehandle)wlSurface, 100, 100, ILM_PIXELFORMAT_RGBA_8888, renderOrder + i));
        ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    }

    t_ilm_layer layer;
    ASSERT_EQ(ILM_SUCCESS, ilm_layerCreateWithDimension(&layer, 300, 300));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());


    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ASSERT_EQ(ILM_SUCCESS, ilm_getScreenIDs(&screenCount, &screenIDs));

    t_ilm_int layerSurfaceCount;
    t_ilm_surface* layerSurfaceIDs;

    //test start
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder, surfaceCount));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());

    //set empty render order
    ASSERT_EQ(ILM_SUCCESS, ilm_layerSetRenderOrder(layer, renderOrder, 0));
    ASSERT_EQ(ILM_SUCCESS, ilm_commitChanges());
    ASSERT_EQ(ILM_SUCCESS, ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs));

    ASSERT_EQ(0, layerSurfaceCount);
}
