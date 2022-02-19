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
#ifndef SHADERTEXTURE_H_
#define SHADERTEXTURE_H_

#include "ShaderBase.h"

class ShaderTexture: public ShaderBase {
public:
    ShaderTexture(float* projectionMatrix);
    virtual ~ShaderTexture();

    virtual void use(vec3f* position, GLuint textureID);

    GLint getAttributeTexCoord();
    void setTexCoords(vec2f * m_texCoords);

private:
    int m_uniformModelMatrix;
    int m_uniformColor;
    GLint m_uniformSampler;
    GLint attributeTexCoord;
};

#endif /* SHADERTEXTURE_H_ */
