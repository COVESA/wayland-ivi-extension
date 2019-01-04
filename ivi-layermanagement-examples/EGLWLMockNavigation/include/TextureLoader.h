/***************************************************************************
 *
 * Copyright (C) 2018 Advanced Driver Information Technology Joint Venture GmbH
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
#ifndef _TEXTURE_LOADER_H
#define _TEXTURE_LOADER_H

#include <string.h>

#include <iostream>
using std::cout;
using std::endl;

#include <GLES2/gl2.h>

class TextureLoader
{
public:
    TextureLoader();

    bool loadBMP(const char * imagePath);
    bool loadArray(void * data, unsigned int width, unsigned int height, unsigned int pixelSizeBits);
    GLuint getId();

private:
    GLuint textureID;
    bool loaded = false;
};

#endif
