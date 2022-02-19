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
#ifndef _SKY_H
#define _SKY_H

#include "IRenderable.h"
#include "IUpdateable.h"
#include "vec.h"
#include "ShaderGradient.h"

class ShaderBase;

class Sky : public IRenderable, public IUpdateable
{
public:
    Sky(vec3f position, vec3f size, vec4f color, ShaderGradient* shader, float sunriseSpeed = 0.02, float sunsetSpeed = 0.01, float daySpeed = 0.02, float nightSpeed = 0.02);
    virtual ~Sky() {}

    virtual void render();
    virtual void update(int currentTimeInMs, int lastFrameTime);

private:
    vec3f m_position;
    vec3f m_size;
    vec4f m_color;
    vec4f colors[4]={{0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 0.0, 1.0}};

    vec4u m_index;
    vec3f m_vertex[4];

    enum skyStates {sunrise, daytime, sunset, nighttime};
    enum skyStates skyState;
    const float sunriseSpeed;
    const float sunsetSpeed;
    const float daySpeed;
    const float nightSpeed;

    ShaderGradient* m_pShader;
};

#endif /* _SKY_H */
