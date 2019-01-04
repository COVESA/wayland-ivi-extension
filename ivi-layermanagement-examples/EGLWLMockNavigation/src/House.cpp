/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#include "House.h"
#include "ShaderLighting.h"

#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#include <iostream>
using std::cout;
using std::endl;

#include <GLES2/gl2.h>

House::House(vec3f position, vec3f size, vec4f houseColor, ShaderBase* pShader, TextureLoader* houseTexture, float numElements)
: m_position(position)
, m_size(size)
, m_color(houseColor)
, numElements(numElements)
, m_pShader(pShader)
{
	/*
    //     5-------------6
    //    /|            /|
    //   / |           / |
    //  1-------------2  |
    //  |  |          |  |
    //  |  |          |  |         x
    //  |  |          |  |         |  z
    //  |  4----------|--7         | /
    //  | /           | /          |/
    //  |/            |/            ------y
    //  0-------------3
    */

    float height = 0.1 + 1.5 * random() / INT_MAX;

    if(houseTexture != nullptr){
        texture = houseTexture;
        withTexture = true;
        // use square houses (1.0 x 1.0) when using textures
        height = 1.0f;
    }

    m_index[0] = vec3u(7, 4, 0); // bottom
    m_index[1] = vec3u(0, 3, 7); // bottom
    m_index[2]  = vec3u(3, 2, 6); // right
    m_index[3]  = vec3u(6, 7, 3); // right
    m_index[4]  = vec3u(7, 6, 5); // back
    m_index[5]  = vec3u(5, 4, 7); // back
    m_index[6]  = vec3u(4, 5, 1); // left
    m_index[7]  = vec3u(1, 0, 4); // left
    m_index[8]  = vec3u(5, 6, 2); // top
    m_index[9]  = vec3u(2, 1, 5); // top
    m_index[10] = vec3u(0, 1, 2); // front
    m_index[11] = vec3u(2, 3, 0); // front


    m_vertex[0].x =  0.0f; m_vertex[0].y =  0.0f;   m_vertex[0].z =  1.0f;
    m_vertex[1].x =  1.0f; m_vertex[1].y =  0.0f;   m_vertex[1].z =  1.0f;
    m_vertex[2].x =  1.0f; m_vertex[2].y =  height; m_vertex[2].z =  1.0f;
    m_vertex[3].x =  0.0f; m_vertex[3].y =  height; m_vertex[3].z =  1.0f;
    m_vertex[4].x =  0.0f; m_vertex[4].y =  0.0f;   m_vertex[4].z =  0.0f;
    m_vertex[5].x =  1.0f; m_vertex[5].y =  0.0f;   m_vertex[5].z =  0.0f;
    m_vertex[6].x =  1.0f; m_vertex[6].y =  height; m_vertex[6].z =  0.0f;
    m_vertex[7].x =  0.0f; m_vertex[7].y =  height; m_vertex[7].z =  0.0f;
}

House::~House()
{
}

void House::render()
{
    if (withTexture) {
        GLuint textureID = texture->getId();
        ((ShaderTexture *)m_pShader)->use(&m_position, textureID);
    } else {
        m_pShader->use(&m_position, &m_color);
    }

    // draw
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, m_vertex);
    if(not withTexture) {
        glDrawElements(GL_TRIANGLES, 3 * sizeof(m_index)/sizeof(m_index[0]), GL_UNSIGNED_SHORT, m_index);
    } else {
        for(int i = 0; i < 6; i++) {
            // change texture mapping for each size of the house
            // numElements == number of textures used per side of a house
            if(i*2 == 0.0) { // left
                m_texCoords[0].x = numElements;
                m_texCoords[0].y = 0.0;

                m_texCoords[7].x = 0.0;
                m_texCoords[7].y = numElements;

                m_texCoords[4].x = 0.0;
                m_texCoords[4].y = 0.0;

                m_texCoords[3].x = numElements;
                m_texCoords[3].y = numElements;
            } else if(i*2 == 10.0) { // front
                m_texCoords[0].x = 0.0;
                m_texCoords[0].y = 0.0;

                m_texCoords[1].x = numElements;
                m_texCoords[1].y = 0.0;

                m_texCoords[2].x = numElements;
                m_texCoords[2].y = numElements;

                m_texCoords[3].x = 0.0;
                m_texCoords[3].y = numElements;
            } else if(i*2 == 8.0) { // right
                m_texCoords[5].x = numElements;
                m_texCoords[5].y = 0.0;

                m_texCoords[1].x = 0.0;
                m_texCoords[1].y = 0.0;

                m_texCoords[2].x = 0.0;
                m_texCoords[2].y = numElements;

                m_texCoords[6].x = numElements;
                m_texCoords[6].y = numElements;
            } else {
                memset(m_texCoords, 0, sizeof(m_texCoords));
            }
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, m_vertex);
            ((ShaderTexture *)m_pShader)->setTexCoords(m_texCoords);
            glDrawElements(GL_TRIANGLE_FAN, 6, GL_UNSIGNED_SHORT, &m_index[i*2]);
        }
    }
}

void House::update(int currentTimeInMs, int lastFrameTime)
{
    (void)currentTimeInMs; //prevent warning

    m_position.z += 0.0005f * (GLfloat)lastFrameTime;

    if (m_position.z > 3.0)
    {
        m_position.z -= 15.0 * 2.0 * 1.0;
    }
}
