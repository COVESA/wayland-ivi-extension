/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
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

/* METHODS THAT ARE CURRENTLY NOT TESTED:
 *
 *     ilm_surfaceInvalidateRectangle
 *     ilm_layerSetChromaKey
 *     ilm_getNumberOfHardwareLayers
 *     ilm_layerGetType(t_ilm_layer layerId,ilmLayerType* layerType);
     ilm_layerGetCapabilities(t_ilm_layer layerId, t_ilm_layercapabilities *capabilities);
     ilm_layerTypeGetCapabilities(ilmLayerType layerType, t_ilm_layercapabilities *capabilities);
 *
 * */

#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
    #include "ilm_client.h"
    #include "ilm_control.h"
}

class IlmCommandTest : public ::testing::Test {
public:
    IlmCommandTest(){
    }

    static void SetUpTestCase() {
        ilm_init();
     }
    static void TearDownTestCase() {
        ilm_destroy();
    }

    void TearDown() {
        removeAll();
    }

    void removeAll(){
        t_ilm_layer* layers = NULL;
        t_ilm_int numLayer=0;
        ilm_getLayerIDs(&numLayer, &layers);
        for (t_ilm_int i=0; i<numLayer; i++ ){
            ilm_layerRemove(layers[i]);
        };

        t_ilm_surface* surfaces = NULL;
        t_ilm_int numSurfaces=0;
        ilm_getSurfaceIDs(&numSurfaces, &surfaces);
        for (t_ilm_int i=0; i<numSurfaces; i++ ){
            ilm_surfaceRemove(surfaces[i]);
        };

        ilm_commitChanges();
      }

};

TEST_F(IlmCommandTest, SetGetSurfaceDimension) {
    uint surface = 36;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface);

    t_ilm_uint dim[2] = {15,25};
    ilm_surfaceSetDimension(surface,dim);
    ilm_commitChanges();

    t_ilm_uint dimreturned[2];
    ilm_surfaceGetDimension(surface,dimreturned);
    ASSERT_EQ(dim[0],dimreturned[0]);
    ASSERT_EQ(dim[1],dimreturned[1]);
}

TEST_F(IlmCommandTest, SetGetLayerDimension) {
    uint layer = 4316;

    ilm_layerCreateWithDimension(&layer, 800, 480);

    t_ilm_uint dim[2] = {115,125};
    ilm_layerSetDimension(layer,dim);
    ilm_commitChanges();

    t_ilm_uint dimreturned[2];
    ilm_layerGetDimension(layer,dimreturned);
    ASSERT_EQ(dim[0],dimreturned[0]);
    ASSERT_EQ(dim[1],dimreturned[1]);
}

TEST_F(IlmCommandTest, SetGetSurfacePosition) {
    uint surface = 36;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface);

    t_ilm_uint pos[2] = {15,25};
    ilm_surfaceSetPosition(surface,pos);
    ilm_commitChanges();

    t_ilm_uint posreturned[2];
    ilm_surfaceGetPosition(surface,posreturned);
    ASSERT_EQ(pos[0],posreturned[0]);
    ASSERT_EQ(pos[1],posreturned[1]);
}

TEST_F(IlmCommandTest, SetGetLayerPosition) {
    uint layer = 4316;

    ilm_layerCreateWithDimension(&layer, 800, 480);

    t_ilm_uint pos[2] = {115,125};
    ilm_layerSetPosition(layer,pos);
    ilm_commitChanges();

    t_ilm_uint posreturned[2];
    ilm_layerGetPosition(layer,posreturned);
    ASSERT_EQ(pos[0],posreturned[0]);
    ASSERT_EQ(pos[1],posreturned[1]);
}

TEST_F(IlmCommandTest, SetGetSurfaceOrientation) {
    uint surface = 36;
    ilmOrientation returned;
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface);

    ilm_surfaceSetOrientation(surface,ILM_NINETY);
    ilm_commitChanges();
    ilm_surfaceGetOrientation(surface,&returned);
    ASSERT_EQ(ILM_NINETY,returned);

    ilm_surfaceSetOrientation(surface,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    ilm_surfaceGetOrientation(surface,&returned);
    ASSERT_EQ(ILM_ONEHUNDREDEIGHTY,returned);

    ilm_surfaceSetOrientation(surface,ILM_TWOHUNDREDSEVENTY);
    ilm_commitChanges();
    ilm_surfaceGetOrientation(surface,&returned);
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY,returned);

    ilm_surfaceSetOrientation(surface,ILM_ZERO);
    ilm_commitChanges();
    ilm_surfaceGetOrientation(surface,&returned);
    ASSERT_EQ(ILM_ZERO,returned);
}

TEST_F(IlmCommandTest, SetGetLayerOrientation) {
    uint layer = 4316;
    ilm_layerCreateWithDimension(&layer, 800, 480);
    ilm_commitChanges();
    ilmOrientation returned;

    ilm_layerSetOrientation(layer,ILM_NINETY);
    ilm_commitChanges();
    ilm_layerGetOrientation(layer,&returned);
    ASSERT_EQ(ILM_NINETY,returned);

    ilm_layerSetOrientation(layer,ILM_ONEHUNDREDEIGHTY);
    ilm_commitChanges();
    ilm_layerGetOrientation(layer,&returned);
    ASSERT_EQ(ILM_ONEHUNDREDEIGHTY,returned);

    ilm_layerSetOrientation(layer,ILM_TWOHUNDREDSEVENTY);
    ilm_commitChanges();
    ilm_layerGetOrientation(layer,&returned);
    ASSERT_EQ(ILM_TWOHUNDREDSEVENTY,returned);

    ilm_layerSetOrientation(layer,ILM_ZERO);
    ilm_commitChanges();
    ilm_layerGetOrientation(layer,&returned);
    ASSERT_EQ(ILM_ZERO,returned);
}

TEST_F(IlmCommandTest, SetGetSurfaceOpacity) {
    uint surface1 = 36;
    uint surface2 = 44;
    t_ilm_float opacity;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);

    ilm_surfaceSetOpacity(surface1,0.88);
    ilm_commitChanges();
    ilm_surfaceGetOpacity(surface1,&opacity);
    ASSERT_DOUBLE_EQ(0.88, opacity);

    ilm_surfaceSetOpacity(surface2,0.001);
    ilm_commitChanges();
    ilm_surfaceGetOpacity(surface2,&opacity);
    ASSERT_DOUBLE_EQ(0.001, opacity);
}

TEST_F(IlmCommandTest, SetGetLayerOpacity) {
    uint layer1 = 36;
    uint layer2 = 44;
    t_ilm_float opacity;

    ilm_layerCreateWithDimension(&layer1, 800, 480);
    ilm_layerCreateWithDimension(&layer2, 800, 480);

    ilm_layerSetOpacity(layer1,0.88);
    ilm_commitChanges();
    ilm_layerGetOpacity(layer1,&opacity);
    ASSERT_DOUBLE_EQ(0.88, opacity);

    ilm_layerSetOpacity(layer2,0.001);
    ilm_commitChanges();
    ilm_layerGetOpacity(layer2,&opacity);
    ASSERT_DOUBLE_EQ(0.001, opacity);
}

TEST_F(IlmCommandTest, SetGetSurfaceVisibility) {
    uint surface1 = 36;
    t_ilm_bool visibility;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);

    ilm_surfaceSetVisibility(surface1,ILM_TRUE);
    ilm_commitChanges();
    ilm_surfaceGetVisibility(surface1,&visibility);
    ASSERT_EQ(ILM_TRUE, visibility);

    ilm_surfaceSetVisibility(surface1,ILM_FALSE);
    ilm_commitChanges();
    ilm_surfaceGetVisibility(surface1,&visibility);
    ASSERT_EQ(ILM_FALSE, visibility);

    ilm_surfaceSetVisibility(surface1,ILM_TRUE);
    ilm_commitChanges();
    ilm_surfaceGetVisibility(surface1,&visibility);
    ASSERT_EQ(ILM_TRUE, visibility);
}

TEST_F(IlmCommandTest, SetGetLayerVisibility) {
    uint layer1 = 36;
    t_ilm_bool visibility;

    ilm_layerCreateWithDimension(&layer1, 800, 480);

    ilm_layerSetVisibility(layer1,ILM_TRUE);
    ilm_commitChanges();
    ilm_layerGetVisibility(layer1,&visibility);
    ASSERT_EQ(ILM_TRUE, visibility);

    ilm_layerSetVisibility(layer1,ILM_FALSE);
    ilm_commitChanges();
    ilm_layerGetVisibility(layer1,&visibility);
    ASSERT_EQ(ILM_FALSE, visibility);

    ilm_layerSetVisibility(layer1,ILM_TRUE);
    ilm_commitChanges();
    ilm_layerGetVisibility(layer1,&visibility);
    ASSERT_EQ(ILM_TRUE, visibility);
}

TEST_F(IlmCommandTest, ilm_getScreenIDs) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ilm_getScreenIDs(&numberOfScreens,&screenIDs);
    ASSERT_GT(numberOfScreens,0u);
}

TEST_F(IlmCommandTest, ilm_getScreenResolution) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ilm_getScreenIDs(&numberOfScreens,&screenIDs);
    ASSERT_TRUE(numberOfScreens>0);

    uint firstScreen = screenIDs[0];
    t_ilm_uint width = 0, height = 0;
    ilm_getScreenResolution(firstScreen, &width, &height);
    ASSERT_GT(width,0u);
    ASSERT_GT(height,0u);
}

TEST_F(IlmCommandTest, ilm_getLayerIDs) {
    uint layer1 = 3246;
    uint layer2 = 46586;

    ilm_layerCreateWithDimension(&layer1, 800, 480);
    ilm_layerCreateWithDimension(&layer2, 800, 480);
    ilm_commitChanges();

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getLayerIDs(&length,&IDs);

    ASSERT_EQ(layer1,IDs[0]);
    ASSERT_EQ(layer2,IDs[1]);
}

TEST_F(IlmCommandTest, ilm_getLayerIDsOfScreen) {
    t_ilm_layer layer1 = 3246;
    t_ilm_layer layer2 = 46586;
    t_ilm_uint roLength = 2;
    t_ilm_layer idRenderOrder[2] = {layer1,layer2};
    ilm_layerCreateWithDimension(&layer1, 800, 480);
    ilm_layerCreateWithDimension(&layer2, 800, 480);
    ilm_displaySetRenderOrder(0,idRenderOrder,roLength);
    ilm_commitChanges();

    t_ilm_int length = 0;
    t_ilm_layer* IDs;
    ilm_getLayerIDsOnScreen(0,&length,&IDs);

    ASSERT_NE(length,0);
    ASSERT_EQ(layer1,IDs[0]);
    ASSERT_EQ(layer2,IDs[1]);
}

TEST_F(IlmCommandTest, ilm_getSurfaceIDs) {
    uint surface1 = 3246;
    uint surface2 = 46586;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);
    ilm_commitChanges();

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getSurfaceIDs(&length,&IDs);

    ASSERT_EQ(surface1,IDs[0]);
    ASSERT_EQ(surface2,IDs[1]);
}

TEST_F(IlmCommandTest, ilm_surfaceCreate_Remove) {
    uint surface1 = 3246;
    uint surface2 = 46586;
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);
    ilm_commitChanges();

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getSurfaceIDs(&length,&IDs);

    ASSERT_EQ(length,2);
    ASSERT_EQ(surface1,IDs[0]);
    ASSERT_EQ(surface2,IDs[1]);

    ilm_surfaceRemove(surface1);
    ilm_commitChanges();
    ilm_getSurfaceIDs(&length,&IDs);
    ASSERT_EQ(length,1);
    ASSERT_EQ(surface2,IDs[0]);

    ilm_surfaceRemove(surface2);
    ilm_commitChanges();
    ilm_getSurfaceIDs(&length,&IDs);
    ASSERT_EQ(length,0);
}

TEST_F(IlmCommandTest, ilm_layerCreate_Remove) {
    uint layer1 = 3246;
    uint layer2 = 46586;
    ilm_layerCreateWithDimension(&layer1, 800, 480);
    ilm_layerCreateWithDimension(&layer2, 800, 480);
    ilm_commitChanges();

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getLayerIDs(&length,&IDs);

    ASSERT_EQ(length,2);
    ASSERT_EQ(layer1,IDs[0]);
    ASSERT_EQ(layer2,IDs[1]);

    ilm_layerRemove(layer1);
    ilm_commitChanges();
    ilm_getLayerIDs(&length,&IDs);
    ASSERT_EQ(length,1);
    ASSERT_EQ(layer2,IDs[0]);

    ilm_layerRemove(layer2);
    ilm_commitChanges();
    ilm_getLayerIDs(&length,&IDs);
    ASSERT_EQ(length,0);
}

TEST_F(IlmCommandTest, ilm_surface_initialize) {
    uint surface_10 = 10;
    uint surface_20 = 20;
    ilm_surfaceInitialize(&surface_10);
    ilm_surfaceInitialize(&surface_20);

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getSurfaceIDs(&length,&IDs);

    ASSERT_EQ(length,2);
    ASSERT_EQ(surface_10,IDs[0]);
    ASSERT_EQ(surface_20,IDs[1]);
}


TEST_F(IlmCommandTest, ilm_layerAddSurface_ilm_layerRemoveSurface_ilm_getSurfaceIDsOnLayer) {
    uint layer = 3246;
    ilm_layerCreateWithDimension(&layer, 800, 480);
    uint surface1 = 3246;
    uint surface2 = 46586;
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);
    ilm_commitChanges();

    t_ilm_int length;
    t_ilm_uint* IDs;
    ilm_getSurfaceIDsOnLayer(layer,&length,&IDs);
    ASSERT_EQ(length,0);

    ilm_layerAddSurface(layer,surface1);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer,&length,&IDs);
    ASSERT_EQ(length,1);
    ASSERT_EQ(surface1,IDs[0]);

    ilm_layerAddSurface(layer,surface2);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer,&length,&IDs);
    ASSERT_EQ(length,2);
    ASSERT_EQ(surface1,IDs[0]);
    ASSERT_EQ(surface2,IDs[1]);

    ilm_layerRemoveSurface(layer,surface1);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer,&length,&IDs);
    ASSERT_EQ(length,1);
    ASSERT_EQ(surface2,IDs[0]);

    ilm_layerRemoveSurface(layer,surface2);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer,&length,&IDs);
    ASSERT_EQ(length,0);
}

TEST_F(IlmCommandTest, ilm_getPropertiesOfSurface_ilm_surfaceSetSourceRectangle_ilm_surfaceSetDestinationRectangle_ilm_surfaceSetChromaKey) {
    t_ilm_uint surface;
    t_ilm_int chromaKey[3] = {3, 22, 111};
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface);
    ilm_commitChanges();

    ilm_surfaceSetOpacity(surface,0.8765);
    ilm_surfaceSetSourceRectangle(surface,89,6538,638,4);
    ilm_surfaceSetDestinationRectangle(surface,54,47,947,9);
    ilm_surfaceSetOrientation(surface,ILM_NINETY);
    ilm_surfaceSetVisibility(surface,true);
    ilm_surfaceSetChromaKey(surface,&chromaKey[0]);
    ilm_commitChanges();

    ilmSurfaceProperties surfaceProperties;
    ilm_getPropertiesOfSurface(surface, &surfaceProperties);
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

    ilm_surfaceSetOpacity(surface,0.436);
    ilm_surfaceSetSourceRectangle(surface,784,546,235,78);
    ilm_surfaceSetDestinationRectangle(surface,536,5372,3,4316);
    ilm_surfaceSetOrientation(surface,ILM_TWOHUNDREDSEVENTY);
    ilm_surfaceSetVisibility(surface,false);
    ilm_surfaceSetChromaKey(surface,NULL);
    ilm_commitChanges();

    ilmSurfaceProperties surfaceProperties2;
    ilm_getPropertiesOfSurface(surface, &surfaceProperties2);
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
    ilm_layerCreateWithDimension(&layer, 800, 480);
    ilm_commitChanges();

    ilm_layerSetOpacity(layer,0.8765);
    ilm_layerSetSourceRectangle(layer,89,6538,638,4);
    ilm_layerSetDestinationRectangle(layer,54,47,947,9);
    ilm_layerSetOrientation(layer,ILM_NINETY);
    ilm_layerSetVisibility(layer,true);
    ilm_layerSetChromaKey(layer,chromaKey);
    ilm_commitChanges();

    ilmLayerProperties layerProperties1;
    ilm_getPropertiesOfLayer(layer, &layerProperties1);
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

    ilm_layerSetOpacity(layer,0.436);
    ilm_layerSetSourceRectangle(layer,784,546,235,78);
    ilm_layerSetDestinationRectangle(layer,536,5372,3,4316);
    ilm_layerSetOrientation(layer,ILM_TWOHUNDREDSEVENTY);
    ilm_layerSetVisibility(layer,false);
    ilm_layerSetChromaKey(layer,NULL);
    ilm_commitChanges();

    ilmLayerProperties layerProperties2;
    ilm_getPropertiesOfLayer(layer, &layerProperties2);
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
    FILE* f = fopen("/tmp/test.bmp","r");
    if (f!=NULL){
        fclose(f);
        int result = remove("/tmp/test.bmp");
        ASSERT_EQ(0,result);
    }

    ilm_takeScreenshot(0, "/tmp/test.bmp");

    sleep(1);
    f = fopen("/tmp/test.bmp","r");
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

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_4444,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_5551,&surface2);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_6661,&surface3);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface4);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGB_565,&surface5);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGB_888,&surface6);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_R_8,&surface7);
    ilm_commitChanges();

    ilmPixelFormat p1,p2,p3,p4,p5,p6,p7;

    ilm_surfaceGetPixelformat(surface1,&p1);
    ilm_surfaceGetPixelformat(surface2,&p2);
    ilm_surfaceGetPixelformat(surface3,&p3);
    ilm_surfaceGetPixelformat(surface4,&p4);
    ilm_surfaceGetPixelformat(surface5,&p5);
    ilm_surfaceGetPixelformat(surface6,&p6);
    ilm_surfaceGetPixelformat(surface7,&p7);

    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_4444,p1);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_5551,p2);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_6661,p3);
    ASSERT_EQ(ILM_PIXELFORMAT_RGBA_8888,p4);
    ASSERT_EQ(ILM_PIXELFORMAT_RGB_565,p5);
    ASSERT_EQ(ILM_PIXELFORMAT_RGB_888,p6);
    ASSERT_EQ(ILM_PIXELFORMAT_R_8,p7);
}

TEST_F(IlmCommandTest, ilm_keyboard_focus)
{
    uint surface;
    uint surface1 = 36;
    uint surface2 = 44;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);

    ilm_GetKeyboardFocusSurfaceId(&surface);
    EXPECT_EQ(0xFFFFFFFF, surface);

    ilm_SetKeyboardFocusOn(surface1);
    ilm_GetKeyboardFocusSurfaceId(&surface);
    EXPECT_EQ(surface1, surface);
}


TEST_F(IlmCommandTest, ilm_input_event_acceptance)
{
    uint surface;
    uint surface1 = 36;
    uint surface2 = 44;
    ilmSurfaceProperties surfaceProperties;

    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface1);
    ilm_surfaceCreate(0,0,0,ILM_PIXELFORMAT_RGBA_8888,&surface2);

    ilm_getPropertiesOfSurface(surface1, &surfaceProperties);
    EXPECT_EQ(ILM_INPUT_DEVICE_ALL, surfaceProperties.inputDevicesAcceptance);

    ilm_UpdateInputEventAcceptanceOn(surface1, (ilmInputDevice) (ILM_INPUT_DEVICE_KEYBOARD | ILM_INPUT_DEVICE_POINTER), false);
    ilm_commitChanges();


    ilm_getPropertiesOfSurface(surface1, &surfaceProperties);
    EXPECT_FALSE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_KEYBOARD);
    EXPECT_FALSE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_POINTER);
    EXPECT_TRUE(surfaceProperties.inputDevicesAcceptance & ILM_INPUT_DEVICE_TOUCH);

    ilm_SetKeyboardFocusOn(surface1);
    ilm_GetKeyboardFocusSurfaceId(&surface);
    EXPECT_NE(surface1, surface);
}

void calculateTimeout(struct timeval* currentTime, int giventimeout, struct timespec* timeout)
{
    /* nanoseconds is old value in nanoseconds + the given milliseconds as nanoseconds */
    t_ilm_ulong newNanoSeconds = currentTime->tv_usec * 1000 + giventimeout * (1000 * 1000);

    /* only use non full seconds, otherwise overflow! */
    timeout->tv_nsec = newNanoSeconds % (1000000000);

    /* new seconds are old seconds + full seconds from new nanoseconds part */
    timeout->tv_sec = currentTime->tv_sec + (newNanoSeconds / 1000000000);
}

TEST(Calc, TimeCalcTestWith1SecondOverflow)
{
   struct timeval currentTime;
   struct timespec timeAdded;
   currentTime.tv_usec = 456000;
   currentTime.tv_sec = 3;
   calculateTimeout(&currentTime, 544, &timeAdded);
   ASSERT_EQ(4, timeAdded.tv_sec);
   ASSERT_EQ(0, timeAdded.tv_nsec);
}

TEST(Calc, TimeCalcTestWithMultipleSecondsOverflow)
{
   struct timeval currentTime;
   struct timespec timeAdded;
   currentTime.tv_usec = 123456;
   currentTime.tv_sec = 3;
   calculateTimeout(&currentTime, 3500, &timeAdded);
   ASSERT_EQ(6, timeAdded.tv_sec);
   ASSERT_EQ(623456000, timeAdded.tv_nsec);
}

TEST(Calc, TimeCalcTestWithoutOverflow)
{
   struct timeval currentTime;
   struct timespec timeAdded;
   currentTime.tv_usec = 123456;
   currentTime.tv_sec = 3;
   calculateTimeout(&currentTime, 544, &timeAdded);
   ASSERT_EQ(3, timeAdded.tv_sec);
   ASSERT_EQ(667456000, timeAdded.tv_nsec);
}

TEST_F(IlmCommandTest, SetGetOptimizationMode) {
    ilmOptimization id;
    ilmOptimizationMode mode;
    ilmOptimizationMode retmode;

    id = ILM_OPT_MULTITEXTURE;
    mode = ILM_OPT_MODE_FORCE_OFF;
    ilm_SetOptimizationMode(id, mode);
    ilm_commitChanges();
    ilm_GetOptimizationMode(id, &retmode);
    ASSERT_EQ(mode, retmode);

    id = ILM_OPT_SKIP_CLEAR;
    mode = ILM_OPT_MODE_TOGGLE;
    ilm_SetOptimizationMode(id, mode);
    ilm_commitChanges();
    ilm_GetOptimizationMode(id, &retmode);
    ASSERT_EQ(mode, retmode);

    id = ILM_OPT_MULTITEXTURE;
    mode = ILM_OPT_MODE_HEURISTIC;
    ilm_SetOptimizationMode(id, mode);
    ilm_commitChanges();
    ilm_GetOptimizationMode(id, &retmode);
    ASSERT_EQ(mode, retmode);
}

TEST_F(IlmCommandTest, ilm_getPropertiesOfScreen) {
    t_ilm_uint numberOfScreens = 0;
    t_ilm_uint* screenIDs = NULL;
    ilm_getScreenIDs(&numberOfScreens,&screenIDs);
    ASSERT_TRUE(numberOfScreens>0);

    t_ilm_display screen = screenIDs[0];
    ilmScreenProperties screenProperties;

    t_ilm_layer layerIds[3] = {100, 200, 300};//t_ilm_layer layerIds[3] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    ilm_layerCreateWithDimension(layerIds, 800, 480);
    ilm_layerCreateWithDimension(layerIds + 1, 800, 480);
    ilm_layerCreateWithDimension(layerIds + 2, 800, 480);

    ilm_commitChanges();

    ilm_displaySetRenderOrder(screen, layerIds, 3);

    ilm_commitChanges();


    ilm_getPropertiesOfScreen(screen, &screenProperties);
    ASSERT_EQ(3, screenProperties.layerCount);
    ASSERT_EQ(layerIds[0], screenProperties.layerIds[0]);
    ASSERT_EQ(layerIds[1], screenProperties.layerIds[1]);
    ASSERT_EQ(layerIds[2], screenProperties.layerIds[2]);

    ASSERT_GT(screenProperties.screenWidth, 0u);
    ASSERT_GT(screenProperties.screenHeight, 0u);

    t_ilm_uint numberOfHardwareLayers;
    ilm_getNumberOfHardwareLayers(screen, &numberOfHardwareLayers);
    ASSERT_EQ(numberOfHardwareLayers, screenProperties.harwareLayerCount);
}

TEST_F(IlmCommandTest, DisplaySetRenderOrder_growing) {
    //prepare needed layers
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ilm_layerCreateWithDimension(renderOrder + i, 300, 300);
        ilm_commitChanges();
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    for(unsigned int i = 0; i < screenCount; ++i)
    {
        t_ilm_display screen = screenIDs[i];
        ilmScreenProperties screenProps;

        //trying different render orders with increasing sizes
        for (unsigned int j = layerCount; j <= layerCount; --j) // note: using overflow here
        {
            //put them from end to beginning, so that in each loop iteration the order of layers change
            ilm_displaySetRenderOrder(screen, renderOrder + j, layerCount - j);
            ilm_commitChanges();
            ilm_getPropertiesOfScreen(screen, &screenProps);

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
        ilm_layerCreateWithDimension(renderOrder + i, 300, 300);
        ilm_commitChanges();
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    for(unsigned int i = 0; i < screenCount; ++i)
    {
        t_ilm_display screen = screenIDs[i];
        ilmScreenProperties screenProps;

        //trying different render orders with decreasing sizes
        for (unsigned int j = 0; j <= layerCount; ++j)
        {
            //put them from end to beginning, so that in each loop iteration the order of layers change
            ilm_displaySetRenderOrder(screen, renderOrder + j, layerCount - j);
            ilm_commitChanges();
            ilm_getPropertiesOfScreen(screen, &screenProps);

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
        ilm_surfaceCreate(0,100,100,ILM_PIXELFORMAT_RGBA_8888, renderOrder + i);
        ilm_commitChanges();
    }

    t_ilm_layer layerIDs[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(layerIDs) / sizeof(layerIDs[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ilm_layerCreateWithDimension(layerIDs + i, 300, 300);
        ilm_commitChanges();
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    for(unsigned int i = 0; i < layerCount; ++i)
    {
        t_ilm_layer layer = layerIDs[i];

        t_ilm_int layerSurfaceCount;
        t_ilm_surface* layerSurfaceIDs;

        //trying different render orders with increasing sizes
        for (unsigned int j = surfaceCount; j <= surfaceCount; --j) // note: using overflow here
        {
            //put them from end to beginning, so that in each loop iteration the order of surafces change
            ilm_layerSetRenderOrder(layer, renderOrder + j, surfaceCount - j);
            ilm_commitChanges();
            ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs);

            ASSERT_EQ(surfaceCount - j, layerSurfaceCount);
            for(unsigned int k = 0; k < surfaceCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], layerSurfaceIDs[k]);
            }
        }

        //set empty render order again
        ilm_layerSetRenderOrder(layer, renderOrder, 0);
        ilm_commitChanges();
    }
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_shrinking) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ilm_surfaceCreate(0,100,100,ILM_PIXELFORMAT_RGBA_8888, renderOrder + i);
        ilm_commitChanges();
    }

    t_ilm_layer layerIDs[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint layerCount = sizeof(layerIDs) / sizeof(layerIDs[0]);

    for (unsigned int i = 0; i < layerCount; ++i)
    {
        ilm_layerCreateWithDimension(layerIDs + i, 300, 300);
        ilm_commitChanges();
    }

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    for(unsigned int i = 0; i < layerCount; ++i)
    {
        t_ilm_layer layer = layerIDs[i];

        t_ilm_int layerSurfaceCount;
        t_ilm_surface* layerSurfaceIDs;

        //trying different render orders with decreasing sizes
        for (unsigned int j = 0; j <= layerCount; ++j)
        {
            //put them from end to beginning, so that in each loop iteration the order of surafces change
            ilm_layerSetRenderOrder(layer, renderOrder + j, surfaceCount - j);
            ilm_commitChanges();
            ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs);

            ASSERT_EQ(surfaceCount - j, layerSurfaceCount);
            for(unsigned int k = 0; k < surfaceCount - j; ++k)
            {
                ASSERT_EQ(renderOrder[j + k], layerSurfaceIDs[k]);
            }
        }

        //set empty render order again
        ilm_layerSetRenderOrder(layer, renderOrder, 0);
        ilm_commitChanges();
    }
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_duplicates) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ilm_surfaceCreate(0,100,100,ILM_PIXELFORMAT_RGBA_8888, renderOrder + i);
        ilm_commitChanges();
    }

    t_ilm_surface duplicateRenderOrder[] = {renderOrder[0], renderOrder[1], renderOrder[0], renderOrder[1], renderOrder[0]};
    t_ilm_int duplicateSurfaceCount = sizeof(duplicateRenderOrder) / sizeof(duplicateRenderOrder[0]);

    t_ilm_layer layer;
    ilm_layerCreateWithDimension(&layer, 300, 300);
    ilm_commitChanges();

    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    t_ilm_int layerSurfaceCount;
    t_ilm_surface* layerSurfaceIDs;

    //trying duplicates
    ilm_layerSetRenderOrder(layer, duplicateRenderOrder, duplicateSurfaceCount);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs);

    ASSERT_EQ(2, layerSurfaceCount);
}

TEST_F(IlmCommandTest, LayerSetRenderOrder_empty) {
    //prepare needed layers and surfaces
    t_ilm_layer renderOrder[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    t_ilm_uint surfaceCount = sizeof(renderOrder) / sizeof(renderOrder[0]);

    for (unsigned int i = 0; i < surfaceCount; ++i)
    {
        ilm_surfaceCreate(0,100,100,ILM_PIXELFORMAT_RGBA_8888, renderOrder + i);
        ilm_commitChanges();
    }

    t_ilm_layer layer;
    ilm_layerCreateWithDimension(&layer, 300, 300);
    ilm_commitChanges();


    t_ilm_display* screenIDs;
    t_ilm_uint screenCount;
    ilm_getScreenIDs(&screenCount, &screenIDs);

    t_ilm_int layerSurfaceCount;
    t_ilm_surface* layerSurfaceIDs;

    //test start
    ilm_layerSetRenderOrder(layer, renderOrder, surfaceCount);
    ilm_commitChanges();

    //set empty render order
    ilm_layerSetRenderOrder(layer, renderOrder, 0);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(layer, &layerSurfaceCount, &layerSurfaceIDs);

    ASSERT_EQ(0, layerSurfaceCount);
}
