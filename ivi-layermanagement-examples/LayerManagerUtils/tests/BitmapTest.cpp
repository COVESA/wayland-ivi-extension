/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
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

#include "Bitmap.h"

#define WIDTH 1
#define HEIGHT 1

char* imageData = new char[3];

bool fileExists(std::string filename)
{
    struct stat stFileInfo;
    int status = stat(filename.c_str(),&stFileInfo);
    return status == 0;
}

TEST(BitmapTest, WriteBitmapIntoCurrentDirectory) {
    const char* filename = "test.bmp";
    writeBitmap(filename, imageData, WIDTH, HEIGHT);

    ASSERT_TRUE(fileExists(filename));
}

TEST(BitmapTest, WriteBitmapIntoAbsoluteDirectory) {
    const char* filename = "/tmp/test2.bmp";
    writeBitmap(filename, imageData, WIDTH, HEIGHT);

    ASSERT_TRUE(fileExists(filename));
}

TEST(BitmapTest, WriteBitmapIntoNonExistingRelativeDirectory) {
    const char* filename = "./testa/testb/testc/test2.bmp";
    writeBitmap(filename, imageData, WIDTH, HEIGHT);

    ASSERT_TRUE(fileExists(filename));
}

TEST(BitmapTest, WriteBitmapIntoNonExistingAbsoluteDirectory) {
    const char* filename = "/tmp/testa/testb/test2.bmp";
    writeBitmap(filename, imageData, WIDTH, HEIGHT);

    ASSERT_TRUE(fileExists(filename));
}

TEST(BitmapTest, WriteToFileOrDirectoryNotAllowed) {
    const char* invalidFile = "//invalid";
    writeBitmap(invalidFile, imageData, 1, 1);

    // this test ensures that if the bitmap is not writeable nothing else happens
}
