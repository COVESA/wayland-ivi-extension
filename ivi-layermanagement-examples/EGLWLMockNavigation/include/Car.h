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
#ifndef _CAR_H
#define _CAR_H

#include "IRenderable.h"
#include "vec.h"

class ShaderBase;

class Car : public IRenderable
{
public:
    Car(vec3f position, vec3f size, vec4f color, ShaderBase* shader);
    virtual ~Car() {}

    virtual void render();

private:
    vec3f m_position;
    vec3f m_size;
    vec4f m_color;

    vec3u m_index;
    vec3f m_vertex[3];

    ShaderBase* m_pShader;
};

#endif /* _CAR_H */
