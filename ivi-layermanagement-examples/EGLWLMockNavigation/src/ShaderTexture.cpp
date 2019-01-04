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
#include "ShaderTexture.h"

const static char* vertexShaderCode =
        "attribute mediump vec4 a_vertex;                                \
         uniform mediump mat4 u_modelMatrix;                             \
         attribute mediump vec2 a_texCoord;                              \
         varying mediump vec2 v_texCoord;                                \
         void main(void)                                                 \
         {                                                               \
            gl_Position = u_projectionMatrix * u_modelMatrix * a_vertex; \
            v_texCoord = a_texCoord;                                     \
         }";

const static char* fragmentShaderCode =
        "varying mediump vec2 v_texCoord;                    \
         uniform sampler2D s_texture;                        \
         void main (void)                                    \
         {                                                   \
            gl_FragColor = texture2D(s_texture, v_texCoord); \
         }";

ShaderTexture::ShaderTexture(float* projectionMatrix)
: ShaderBase(vertexShaderCode, fragmentShaderCode, projectionMatrix)
{
    glUseProgram(shaderProgramId);
    m_uniformModelMatrix = glGetUniformLocation(shaderProgramId, "u_modelMatrix");
    m_uniformSampler = glGetUniformLocation(shaderProgramId, "s_texture");
    attributeTexCoord = glGetAttribLocation(shaderProgramId, "a_texCoord");
}

ShaderTexture::~ShaderTexture()
{
}

void ShaderTexture::use(vec3f* position, GLuint textureID)
{
    ShaderBase::use(position, nullptr);

    float translation[16];
    translation[0] = 1.0f;
    translation[1] = 0.0f;
    translation[2] = 0.0f;
    translation[3] = 0.0f;

    translation[4] = 0.0f;
    translation[5] = 1.0f;
    translation[6] = 0.0f;
    translation[7] = 0.0f;

    translation[8] = 0.0f;
    translation[9] = 0.0f;
    translation[10] = 1.0f;
    translation[11] = 0.0f;

    translation[12] = position->x;
    translation[13] = position->y;
    translation[14] = position->z;
    translation[15] = 1.0f;

    glUseProgram(shaderProgramId);
    glUniformMatrix4fv(m_uniformModelMatrix, 1, GL_FALSE, translation);

    // Code for using textures
    // select active texture unit
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Set the sampler texture unit to 0: GL_TEXTURE0
    glUniform1i(m_uniformSampler, 0);
}

GLint ShaderTexture::getAttributeTexCoord() {
    return attributeTexCoord;
}

void ShaderTexture::setTexCoords(vec2f * m_texCoords){
    glEnableVertexAttribArray(attributeTexCoord);
    glVertexAttribPointer(attributeTexCoord, 2, GL_FLOAT, GL_FALSE, 0, m_texCoords);
}
