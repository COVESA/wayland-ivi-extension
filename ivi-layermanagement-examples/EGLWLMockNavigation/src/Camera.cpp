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
    IlmMatrixIdentity(m_identityMatrix);
    IlmMatrixTranslation(m_translationMatrix, m_position.x, m_position.y, m_position.z);
    IlmMatrixRotateX(m_rotationMatrix, 45.0);
    IlmMatrixProjection(m_projectionMatrix,
                        45.0,                          // field of view
                        0.1f,                          // near
                        1000.0f,                       // far
                        viewportWidth/viewportHeight); // aspect ratio
    m_viewProjectionMatrix = m_identityMatrix;
}

Camera::~Camera()
{
}

IlmMatrix* Camera::getViewProjectionMatrix()
{
    return &m_viewProjectionMatrix;
}

void Camera::update(int currentTimeInMs, int lastFrameTime)
{
    (void)currentTimeInMs; // prevent warning
    (void)lastFrameTime; // prevent warning

    IlmMatrixTranslation(m_translationMatrix, m_position.x, m_position.y, m_position.z);

    m_viewProjectionMatrix = m_identityMatrix;
    //IlmMatrixMultiply(m_viewProjectionMatrix, m_viewProjectionMatrix, m_rotationMatrix);
    IlmMatrixMultiply(m_viewProjectionMatrix, m_viewProjectionMatrix, m_translationMatrix);
    IlmMatrixMultiply(m_viewProjectionMatrix, m_viewProjectionMatrix, m_projectionMatrix);
}
