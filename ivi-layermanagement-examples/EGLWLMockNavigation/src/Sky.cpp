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
#include "Sky.h"
#include "ShaderBase.h"
#include <string.h>

#include <iostream>
using std::cout;
using std::endl;

#include <GLES2/gl2.h>

Sky::Sky(vec3f position, vec3f size, vec4f color, ShaderGradient* shader, float sunriseSpeed, float sunsetSpeed, float daySpeed, float nightSpeed)
: m_position(position)
, m_size(size)
, m_color(color)
, m_index(0, 1, 2, 3)
, skyState(sunrise)
, sunriseSpeed(sunriseSpeed)
, sunsetSpeed(sunsetSpeed)
, daySpeed(daySpeed)
, nightSpeed(nightSpeed)
, m_pShader(shader)
{
    m_vertex[0].x = m_position.x;
    m_vertex[0].y = m_position.y;
    m_vertex[0].z = m_position.z;

    m_vertex[1].x = m_position.x;
    m_vertex[1].y = m_position.y + m_size.y;
    m_vertex[1].z = m_position.z;

    m_vertex[2].x = m_position.x + m_size.x;
    m_vertex[2].y = m_position.y + m_size.y;
    m_vertex[2].z = m_position.z;

    m_vertex[3].x = m_position.x + m_size.x;
    m_vertex[3].y = m_position.y;
    m_vertex[3].z = m_position.z;
}

void Sky::render()
{
    m_pShader->use(&m_position, colors);

    // draw
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, m_vertex);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &m_index);
}

void Sky::update(int currentTimeInMs, int lastFrameTime)
{
    (void)currentTimeInMs; //prevent warning
    (void)lastFrameTime;
    static float clock = 0;

    if (skyState == sunrise) {
        if (colors[0].r < 1.0) {
            colors[0].r += sunriseSpeed;
            colors[0].g += sunriseSpeed/2;

            colors[1].b += sunriseSpeed/4;
            colors[2].b += sunriseSpeed/4;

            colors[3].r += sunriseSpeed;
            colors[3].g += sunriseSpeed/2;
        } else {
            skyState = daytime;
        }
    }  else if (skyState == daytime) {
        if (clock < 1.0) {
            clock += daySpeed;
        } else {
            clock = 0.0;
            skyState = sunset;
        }
    } else if (skyState == sunset) {
        if (colors[0].r > 0.0) {
            colors[0].r -= sunsetSpeed;
            colors[0].g -= sunsetSpeed/2;

            colors[1].b -= sunsetSpeed/4;
            colors[2].b -= sunsetSpeed/4;

            colors[3].r -= sunsetSpeed;
            colors[3].g -= sunsetSpeed/2;
        } else {
            skyState = nighttime;
        }
    } else if (skyState == nighttime) {
        if (clock < 1.0) {
            clock += nightSpeed;
        } else {
            clock = 0.0;
            skyState = sunrise;
        }
    }
}
