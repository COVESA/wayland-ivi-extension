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
#ifndef _CAMERA_H
#define _CAMERA_H

#include "IUpdateable.h"
#include "IlmMatrix.h"
#include "vec.h"

class Camera : public IUpdateable
{
public:
    Camera(vec3f position, vec3f target, float viewportWidth, float viewportHeight);
    virtual ~Camera();

    IlmMatrix* getViewProjectionMatrix();
    void update(int currentTimeInMs, int lastFrameTime);

private:
    vec3f m_position;
    vec3f m_target;
    IlmMatrix m_viewProjectionMatrix;
    IlmMatrix m_translationMatrix;
    IlmMatrix m_rotationMatrix;
    IlmMatrix m_projectionMatrix;
    IlmMatrix m_identityMatrix;
};

#endif /* _CAMERA_H */
