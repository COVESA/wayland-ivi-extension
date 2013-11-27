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
#ifndef _HOUSE_H
#define _HOUSE_H

#include "IRenderable.h"
#include "IUpdateable.h"
#include "IlmMatrix.h"
#include "vec.h"

class ShaderBase;

class House : public IRenderable, public IUpdateable
{
public:
    House(vec3f position, vec3f size, vec4f houseColor, ShaderBase* pShader);
    virtual ~House();

    virtual void render();
    virtual void update(int currentTimeInMs, int lastFrameTime);

private:
    void attachVertexBuffer();
    void detachVertexBuffer();

private:
    vec3f m_position;
    vec3f m_size;
    vec4f m_color;

    vec3f m_normals[6];
    vec3u m_index[12];
    vec3f m_vertex[8];

    ShaderBase* m_pShader;
};

#endif /* _HOUSE_H */
