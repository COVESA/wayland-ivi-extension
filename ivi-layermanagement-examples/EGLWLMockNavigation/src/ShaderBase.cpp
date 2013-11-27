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
#include "ShaderBase.h"
#include "IlmMatrix.h"
#include <ilm_client.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <iostream>
using std::cout;

ShaderBase::ShaderBase(string vertexCode, string fragmentCode, IlmMatrix* projectionMatrix)
: m_vertexCode()
, m_fragmentCode(fragmentCode)
, m_projectionMatrix(projectionMatrix)
{
	m_vertexCode = "uniform mediump mat4 u_projectionMatrix;\n";
	m_vertexCode += vertexCode;
	initShader();
}

ShaderBase::~ShaderBase()
{
	destroyShader();
}

bool ShaderBase::initShader()
{
    GLint glResult = GL_FALSE;

    // Create the fragment shader object
    fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Load Fragment Source
    const char* fragmentShaderCode = m_fragmentCode.c_str();
    glShaderSource(fragmentShaderId, 1, &fragmentShaderCode, NULL);

    // Compile the source code of fragment shader
    glCompileShader(fragmentShaderId);

    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &glResult);

    if (glResult == GL_FALSE)
    {
        t_ilm_int infoLength, numberChars;
        glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLength);

        // Allocate Log Space
        char* info = (char*) malloc(sizeof(char) * infoLength);
        glGetShaderInfoLog(fragmentShaderId, infoLength, &numberChars, info);

        // Print the error
        cout << "Failed to compile fragment shader: " << info << "\n";
        free(info);
        return ILM_FALSE;
    }

    // Create the fragment shader object
    vertexShaderId = glCreateShader(GL_VERTEX_SHADER);

    // Load Fragment Source
    const char* vertexShaderCode = m_vertexCode.c_str();
    glShaderSource(vertexShaderId, 1, &vertexShaderCode, NULL);

    // Compile the source code of fragment shader
    glCompileShader(vertexShaderId);

    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &glResult);

    if (glResult == GL_FALSE)
    {
        t_ilm_int infoLength, numberChars;
        glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLength);

        // Allocate Log Space
        char* info = (char*) malloc(sizeof(char) * infoLength);
        glGetShaderInfoLog(vertexShaderId, infoLength, &numberChars, info);

        // Print the error
        cout << "Failed to compile vertex shader: " << info << "\n";
        free(info);
        return ILM_FALSE;
    }

    shaderProgramId = glCreateProgram();

    glAttachShader(shaderProgramId, fragmentShaderId);
    glAttachShader(shaderProgramId, vertexShaderId);

    glBindAttribLocation(shaderProgramId, 0, "a_vertex");

    glLinkProgram(shaderProgramId);

    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &glResult);

    if (glResult == GL_FALSE)
    {
        t_ilm_int infoLength, numberChars;
        glGetShaderiv(shaderProgramId, GL_INFO_LOG_LENGTH, &infoLength);

        // Allocate Log Space
        char* info = (char*) malloc(sizeof(char) * infoLength);
        glGetShaderInfoLog(shaderProgramId, infoLength, &numberChars,
                info);

        // Print the error
        cout << "Failed to link program: " << info << "\n";
        free(info);
        return ILM_FALSE;
    }

#if 1 /* ADIT */
    glValidateProgram(shaderProgramId);

    glGetProgramiv(shaderProgramId, GL_VALIDATE_STATUS, &glResult);

    if (glResult == GL_FALSE)
    {
        t_ilm_int infoLength, numberChars;
        glGetShaderiv(shaderProgramId, GL_INFO_LOG_LENGTH, &infoLength);

        // Allocate Log Space
        char* info = (char*) malloc(sizeof(char) * infoLength);
        glGetShaderInfoLog(shaderProgramId, infoLength, &numberChars,
                info);

        // Print the error
        cout << "Failed to validate program: " << info << "\n";
        free(info);
        return ILM_FALSE;
    }

#endif /* ADIT */

    glUseProgram(shaderProgramId);

    m_uniformProjectionMatrix = glGetUniformLocation(shaderProgramId, "u_projectionMatrix");

    cout << "Shader setup complete.\n";

    return ILM_TRUE;
}

bool ShaderBase::destroyShader()
{
    t_ilm_bool result = ILM_TRUE;
    glDeleteProgram(shaderProgramId);
    glDeleteShader(fragmentShaderId);
    glDeleteShader(vertexShaderId);
    return result;
}

void ShaderBase::use(vec3f* position, vec4f* color)
{
	(void)position; // prevent warning
	(void)color; // prevent warning

	glUseProgram(shaderProgramId);
	glUniformMatrix4fv(m_uniformProjectionMatrix, 1, GL_FALSE, m_projectionMatrix->f);
}
