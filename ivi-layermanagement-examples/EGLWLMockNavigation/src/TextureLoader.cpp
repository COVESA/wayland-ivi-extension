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
#include "TextureLoader.h"

TextureLoader::TextureLoader(){
    ;
}

// source: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
bool TextureLoader::loadBMP(const char * imagePath) {
    // Data read from the header of the BMP file
    const unsigned char headerSize = 54; // Each BMP file begins by a 54-bytes header
    unsigned char header[headerSize];
    GLsizei width, height;
    uint16_t pixelSizeBits;
    uint8_t pixelSizeBytes;
    // imageSize == width*height*pixelSize, because each pixel consists of pixelSize bytes (red, green and blue)
    unsigned int imageSize;
    unsigned char * data;
    FILE * file = fopen(imagePath,"rb");
    if (!file) {
        //cout << "Image file could not be opened: " << imagePath << endl;
        return false;
    }
    if (fread(header, 1, headerSize, file) != headerSize) {
        cout << "Not a correct BMP file (could not read the header): " << imagePath << endl;
        fclose(file);
        return false;
    }
    if (header[0]!='B' || header[1]!='M' ) {
        cout << "Not a correct BMP file (wrong header format) :" << imagePath << endl;
        fclose(file);
        return false;
    }
    // Read fields from the BMP file header
    imageSize  = *(int*)(header + 0x22);
    width      = *(GLsizei*)(header + 0x12);
    height     = *(GLsizei*)(header + 0x16);
    pixelSizeBits  = *(uint16_t*)(header + 0x1C);
    pixelSizeBytes = pixelSizeBits / 8;

    if ((pixelSizeBits != 24) and (pixelSizeBits != 32)) {
        cout << "unsupported pixel size of " << pixelSizeBits << " bits" << endl;
        fclose(file);
        return false;
    }

    // calculate the image size if there is no information in the header
    if (imageSize==0) {
        imageSize=width*height*pixelSizeBytes;
    }

    data = new unsigned char [imageSize];
    fread(data,1,imageSize,file);
    fclose(file);

    if (pixelSizeBits == 32) {
        // BMP files with an alpha channel use ABGR format for storing pixels
        // OpenGLES does not have an ABGR format specifier
        // therefore change pixel format from ABGR to RGBA
        for (long long i = 0; i < width*height; i++) {
            unsigned char a = data[i*pixelSizeBytes + 0];
            unsigned char b = data[i*pixelSizeBytes + 1];
            unsigned char g = data[i*pixelSizeBytes + 2];
            unsigned char r = data[i*pixelSizeBytes + 3];
            data[i*pixelSizeBytes]     = r;
            data[i*pixelSizeBytes + 1] = g;
            data[i*pixelSizeBytes + 2] = b;
            data[i*pixelSizeBytes + 3] = a;
        }
    } else if (pixelSizeBits == 24) {
        for (long long i = 0; i < width*height; i++) {
            unsigned char b = data[i*pixelSizeBytes];
            unsigned char g = data[i*pixelSizeBytes + 1];
            unsigned char r = data[i*pixelSizeBytes + 2];
            data[i*pixelSizeBytes]     = r;
            data[i*pixelSizeBytes + 1] = g;
            data[i*pixelSizeBytes + 2] = b;
        }
    }

    bool loaded = loadArray(data, width, height, pixelSizeBits);
    delete[] data;
    return loaded;
}

// This function loads an array with image data
bool TextureLoader::loadArray(void * data, unsigned int width, unsigned int height, unsigned int pixelSizeBits) {
    if (loaded)
        // don't load the thexture if a texture was already loaded for this object
        return false;
    else
        loaded = true;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (pixelSizeBits == 32) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else if (pixelSizeBits == 24) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return loaded;
}

GLuint TextureLoader::getId() {
    return textureID;
}
