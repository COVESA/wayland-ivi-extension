/***************************************************************************
*
* Copyright 2010,2011 BMW Car IT GmbH
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*               http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
****************************************************************************/
#include "Bitmap.h"
#include "Log.h"
#include <stdint.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>

typedef struct BMPHeaderStruct
{
    uint32_t size; // filesize
    uint16_t a;
    uint16_t b;
    uint32_t offset; // offset
} BMPHeader;

typedef struct headerStruct
{
    uint32_t headersize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compress;
    uint32_t imagesize;
    int32_t xresolution;
    int32_t yresolution;
    uint32_t color1;
    uint32_t color2;
} InfoHeader;

void writeBitmap(std::string FileName, char* imagedataRGB, int width, int height)
{
    LOG_DEBUG("Bitmap", "writing Bitmap to file:" << FileName);

    BMPHeader bmpHeader;
    InfoHeader header;
    int imagebytes = width*height*3;
    char firstbytes[2];
    firstbytes[0] = 'B';
    firstbytes[1] = 'M';
    bmpHeader.size = 2 + sizeof(BMPHeader) + sizeof(header) + imagebytes;
    bmpHeader.a = 0;
    bmpHeader.b = 0;
    bmpHeader.offset = 2 + sizeof(BMPHeader) + sizeof(header);
    header.headersize = sizeof(header);
    header.width = width;
    header.height = height;
    header.planes = 1;
    header.bits = 24; // bitmap does not do alpha
    header.compress = 0;
    header.imagesize = imagebytes;
    header.xresolution = 2835; // because of dpi
    header.yresolution = 2835;
    header.color1 = 0;
    header.color2 = 0;

    // make sure parent directory exists
    std::size_t currentPos = 0;
    std::size_t lastPos = FileName.find_first_of("/", currentPos);
    while (lastPos != std::string::npos)
    {
        std::string directory = FileName.substr(0, lastPos);
        LOG_DEBUG("Bitmap", "Creating directory " << directory);
        mkdir(directory.c_str(), 0755);
        currentPos = lastPos;
        lastPos = FileName.find_first_of("/", currentPos + 1);
    }

    FILE* file = fopen(FileName.c_str(), "wb");

    if (file)
    {
        fwrite(firstbytes, 2, 1, file);
        fwrite((void*)&bmpHeader, sizeof(BMPHeader), 1, file);
        fwrite((void*)&header, sizeof(header), 1, file);
        fwrite(imagedataRGB, header.imagesize, 1, file);
        fclose(file);
    }
    else
    {
        LOG_DEBUG("Bitmap", "File could not be opened for writing");
    }
}
