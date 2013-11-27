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
#include "ShaderLighting.h"
#include "IlmMatrix.h"

const char* vertexShaderCode =
		    "attribute mediump vec4 a_vertex;                                 \
		     uniform mediump mat4 u_modelMatrix;                              \
		     varying mediump vec4 v_normal;                                   \
             void main(void)                                                  \
             {                                                                \
                 gl_Position = u_projectionMatrix * u_modelMatrix * a_vertex; \
		         v_normal = normalize(a_vertex);                              \
             }";

const char* fragmentShaderCode =
		    "uniform mediump vec4 u_color;   \
		     varying mediump vec4 v_normal;  \
		     mediump vec4 lightPosition;     \
             void main (void)                \
		     {                               \
		         lightPosition = normalize(vec4(-3.0, -5.0, 10.0, 1.0));   \
		         gl_FragColor = max(dot(v_normal, lightPosition), 0.0) * 0.5 * u_color + 0.8 * u_color;   \
		         gl_FragColor.a = 1.0;   \
		     }";

ShaderLighting::ShaderLighting(IlmMatrix* projectionMatrix)
: ShaderBase(vertexShaderCode, fragmentShaderCode, projectionMatrix)
{
    glUseProgram(shaderProgramId);
    m_uniformModelMatrix = glGetUniformLocation(shaderProgramId, "u_modelMatrix");
    m_uniformColor = glGetUniformLocation(shaderProgramId, "u_color");
}

ShaderLighting::~ShaderLighting()
{
}

void ShaderLighting::use(vec3f* position, vec4f* color)
{
	ShaderBase::use(position, color);

    IlmMatrix translation;
	IlmMatrixTranslation(translation, position->x, position->y, position->z);

	glUseProgram(shaderProgramId);
	glUniformMatrix4fv(m_uniformModelMatrix, 1, GL_FALSE, translation.f);
    glUniform4fv(m_uniformColor, 1, &color->r);

}
