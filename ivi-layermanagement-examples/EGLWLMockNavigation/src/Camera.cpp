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
#include "Camera.h"
#include "vec.h"

#include <math.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

Camera::Camera(vec3f position, vec3f target, float viewportWidth, float viewportHeight)
: m_position(position)
, m_target(target)
{
    calculateTranslationMatrix();

    m_fov = M_PI / 4; // 45°
    m_near = 0.1f;
    m_far = 1000.0f;
    m_aspectRatio = viewportWidth/viewportHeight;
    calculateProjectionMatrix();
    calculateViewProjectionMatrix();
}

Camera::~Camera()
{
}

void Camera::calculateProjectionMatrix()
{
    // Precompute borders for projection
    float range = m_near * tan(m_fov / 2.0);
    float right = range * m_aspectRatio;
    float top = range;

    // Column 1
    m_projectionMatrix[0] = m_near / right;
    m_projectionMatrix[1] = 0.0;
    m_projectionMatrix[2] = 0.0;
    m_projectionMatrix[3] = 0.0;

    // Column 2
    m_projectionMatrix[4] = 0.0;
    m_projectionMatrix[5] = m_near / top;
    m_projectionMatrix[6] = 0.0;
    m_projectionMatrix[7] = 0.0;

    // Column 3
    m_projectionMatrix[8] = 0.0;
    m_projectionMatrix[9] = 0.0;
    m_projectionMatrix[10] = -(m_far + m_near) / (m_far - m_near);
    m_projectionMatrix[11] = -1;

    // Column 4
    m_projectionMatrix[12] = 0.0;
    m_projectionMatrix[13] = 0.0;
    m_projectionMatrix[14] = -(2 * m_far * m_near) / (m_far - m_near);
    m_projectionMatrix[15] = 0.0;
}

void Camera::calculateTranslationMatrix()
{
    m_translationMatrix[0] = 1.0f;
    m_translationMatrix[1] = 0.0f;
    m_translationMatrix[2] = 0.0f;
    m_translationMatrix[3] = 0.0f;

    m_translationMatrix[4] = 0.0f;
    m_translationMatrix[5] = 1.0f;
    m_translationMatrix[6] = 0.0f;
    m_translationMatrix[7] = 0.0f;

    m_translationMatrix[8] = 0.0f;
    m_translationMatrix[9] = 0.0f;
    m_translationMatrix[10] = 1.0f;
    m_translationMatrix[11] = 0.0f;

    m_translationMatrix[12] = m_position.x;
    m_translationMatrix[13] = m_position.y;
    m_translationMatrix[14] = m_position.z;
    m_translationMatrix[15] = 1.0f;
}

void Camera::calculateViewProjectionMatrix()
{

    m_viewProjectionMatrix[0]  = m_translationMatrix[0] * m_projectionMatrix[0];
    m_viewProjectionMatrix[1]  = 0.0f;
    m_viewProjectionMatrix[2]  = 0.0f;
    m_viewProjectionMatrix[3]  = 0.0f;

    m_viewProjectionMatrix[4]  = 0.0f;
    m_viewProjectionMatrix[5]  = m_translationMatrix[5] * m_projectionMatrix[5];
    m_viewProjectionMatrix[6]  = 0.0f;
    m_viewProjectionMatrix[7]  = 0.0f;

    m_viewProjectionMatrix[8]  = 0.0f;
    m_viewProjectionMatrix[9]  = 0.0f;
    m_viewProjectionMatrix[10] = m_translationMatrix[10] * m_projectionMatrix[10];
    m_viewProjectionMatrix[11] = m_translationMatrix[10] * m_projectionMatrix[11];

    m_viewProjectionMatrix[12] = m_translationMatrix[12] * m_projectionMatrix[0];
    m_viewProjectionMatrix[13] = m_translationMatrix[13] * m_projectionMatrix[5];
    m_viewProjectionMatrix[14] = m_translationMatrix[14] * m_projectionMatrix[10]
                 + m_translationMatrix[15] * m_projectionMatrix[14];
    m_viewProjectionMatrix[15] = m_translationMatrix[14] * m_projectionMatrix[11];
}

float* Camera::getViewProjectionMatrix()
{
    return m_viewProjectionMatrix;
}

void Camera::update(int currentTimeInMs, int lastFrameTime)
{
    (void)currentTimeInMs; // prevent warning
    (void)lastFrameTime; // prevent warning
}
