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
#include "Car.h"
#include "ShaderBase.h"
#include <string.h>

#include <iostream>
using std::cout;
using std::endl;

#include <GLES2/gl2.h>

Car::Car(vec3f position, vec3f size, vec4f color, ShaderBase* shader)
: m_position(position)
, m_size(size)
, m_color(color)
, m_index(0, 1, 2, 3)
, m_pShader(shader)
, texture(nullptr)
, withTexture(false)
{
	m_vertex[0].x = 0.0;
	m_vertex[0].y = 0.0;
	m_vertex[0].z = 0.0;

	m_vertex[1].x = 0.1 * m_size.x;
	m_vertex[1].y = 0.0;
	m_vertex[1].z = -m_size.z;

	m_vertex[2].x = 0.9 * m_size.x;
	m_vertex[2].y = 0.0;
	m_vertex[2].z = -m_size.z;

	m_vertex[3].x = m_size.x;
	m_vertex[3].y = 0.0;
	m_vertex[3].z = 0.0;
}

Car::Car(vec3f position, vec3f size, vec4f color, ShaderTexture* shader, TextureLoader* texture)
: m_position(position)
, m_size(size)
, m_color(color)
, m_index(0, 1, 2, 3)
, m_pShader(shader)
, texture(texture)
, withTexture(true)
{
    m_vertex[0].x = 0.0;
    m_vertex[0].y = 0.0;
    m_vertex[0].z = 0.0;

    m_vertex[1].x = 0.0;
    m_vertex[1].y = 0.0;
    m_vertex[1].z = -m_size.z;

    m_vertex[2].x = m_size.x;
    m_vertex[2].y = 0.0;
    m_vertex[2].z = -m_size.z;

    m_vertex[3].x = m_size.x;
    m_vertex[3].y = 0.0;
    m_vertex[3].z = 0.0;

    m_texCoords[0].x = 0.0;
    m_texCoords[0].y = 0.0;

    m_texCoords[1].x = 0.0;
    m_texCoords[1].y = 1.0;

    m_texCoords[2].x = 1.0;
    m_texCoords[2].y = 1.0;

    m_texCoords[3].x = 1.0;
    m_texCoords[3].y = 0.0;
}

void Car::render()
{
    // draw
    if (withTexture) {
        GLuint textureID = texture->getId();
        ((ShaderTexture *)m_pShader)->use(&m_position, textureID);
        ((ShaderTexture *)m_pShader)->setTexCoords(m_texCoords);
    } else {
        m_pShader->use(&m_position, &m_color);
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, m_vertex);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &m_index);
}
