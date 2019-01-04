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
#include "ShaderGradient.h"

const static char* vertexShaderCode =
		    "attribute mediump vec4 a_vertex; \
		     attribute mediump vec4 a_color;  \
		     varying mediump vec4 v_color;    \
             void main(void)                  \
             {                                \
                 gl_Position = a_vertex;      \
		         v_color = a_color;           \
             }";

const static char* fragmentShaderCode =
		    "varying mediump vec4 v_color;   \
             void main (void)                \
		     {                               \
		         gl_FragColor = v_color;     \
		     }";

ShaderGradient::ShaderGradient()
: ShaderBase(vertexShaderCode, fragmentShaderCode, nullptr)
{
    glUseProgram(shaderProgramId);
    attributeColor = glGetAttribLocation(shaderProgramId, "a_color");
}

ShaderGradient::~ShaderGradient()
{
}

void ShaderGradient::use(vec3f* position, vec4f* color)
{
	glUseProgram(shaderProgramId);
	glEnableVertexAttribArray(attributeColor);
    glVertexAttribPointer(attributeColor, 4, GL_FLOAT, GL_FALSE, 0, &color->r);
}
