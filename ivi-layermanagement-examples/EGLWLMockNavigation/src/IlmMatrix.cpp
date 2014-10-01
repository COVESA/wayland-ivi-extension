/***************************************************************************
*
* Copyright 2010,2011 BMW Car IT GmbH
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*               http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
* Oolong Engine for the iPhone / iPod touch
* Copyright (c) 2007-2008 Wolfgang Engel  http://code.google.com/p/oolongengine/
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it freely,
* subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not claim that
*    you wrote the original software. If you use this software in a product, an
*    acknowledgment in the product documentation would be appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be misrepresented
*    as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
****************************************************************************/
#include "IlmMatrix.h"
#include <math.h>

#define ILM_PI 3.14159265

#define degToRad(deg) ((deg) * ILM_PI / 180.0f)
#define radToDeg(rad) ((rad) * 180.0f / ILM_PI)

void IlmMatrixIdentity(IlmMatrix &mOut)
{
    mOut.f[0] = 1.0f;
    mOut.f[1] = 0.0f;
    mOut.f[2] = 0.0f;
    mOut.f[3] = 0.0f;

    mOut.f[4] = 0.0f;
    mOut.f[5] = 1.0f;
    mOut.f[6] = 0.0f;
    mOut.f[7] = 0.0f;

    mOut.f[8] = 0.0f;
    mOut.f[9] = 0.0f;
    mOut.f[10] = 1.0f;
    mOut.f[11] = 0.0f;

    mOut.f[12] = 0.0f;
    mOut.f[13] = 0.0f;
    mOut.f[14] = 0.0f;
    mOut.f[15] = 1.0f;
}

void IlmMatrixMultiply(IlmMatrix &mOut, const IlmMatrix &mA, const IlmMatrix &mB)
{
    IlmMatrix mRet;

    mRet.f[0]  = mA.f[0] * mB.f[0] + mA.f[1] * mB.f[4] + mA.f[2] * mB.f[8] + mA.f[3] * mB.f[12];
    mRet.f[1]  = mA.f[0] * mB.f[1] + mA.f[1] * mB.f[5] + mA.f[2] * mB.f[9] + mA.f[3] * mB.f[13];
    mRet.f[2]  = mA.f[0] * mB.f[2] + mA.f[1] * mB.f[6] + mA.f[2] * mB.f[10] + mA.f[3] * mB.f[14];
    mRet.f[3]  = mA.f[0] * mB.f[3] + mA.f[1] * mB.f[7] + mA.f[2] * mB.f[11] + mA.f[3] * mB.f[15];

    mRet.f[4]  = mA.f[4] * mB.f[0] + mA.f[5] * mB.f[4] + mA.f[6] * mB.f[8] + mA.f[7] * mB.f[12];
    mRet.f[5]  = mA.f[4] * mB.f[1] + mA.f[5] * mB.f[5] + mA.f[6] * mB.f[9] + mA.f[7] * mB.f[13];
    mRet.f[6]  = mA.f[4] * mB.f[2] + mA.f[5] * mB.f[6] + mA.f[6] * mB.f[10] + mA.f[7] * mB.f[14];
    mRet.f[7]  = mA.f[4] * mB.f[3] + mA.f[5] * mB.f[7] + mA.f[6] * mB.f[11] + mA.f[7] * mB.f[15];

    mRet.f[8]  = mA.f[8] * mB.f[0] + mA.f[9] * mB.f[4] + mA.f[10] * mB.f[8] + mA.f[11] * mB.f[12];
    mRet.f[9]  = mA.f[8] * mB.f[1] + mA.f[9] * mB.f[5] + mA.f[10] * mB.f[9] + mA.f[11] * mB.f[13];
    mRet.f[10] = mA.f[8] * mB.f[2] + mA.f[9] * mB.f[6] + mA.f[10] * mB.f[10] + mA.f[11] * mB.f[14];
    mRet.f[11] = mA.f[8] * mB.f[3] + mA.f[9] * mB.f[7] + mA.f[10] * mB.f[11] + mA.f[11] * mB.f[15];

    mRet.f[12] = mA.f[12] * mB.f[0] + mA.f[13] * mB.f[4] + mA.f[14] * mB.f[8] + mA.f[15] * mB.f[12];
    mRet.f[13] = mA.f[12] * mB.f[1] + mA.f[13] * mB.f[5] + mA.f[14] * mB.f[9] + mA.f[15] * mB.f[13];
    mRet.f[14] = mA.f[12] * mB.f[2] + mA.f[13] * mB.f[6] + mA.f[14] * mB.f[10] + mA.f[15] * mB.f[14];
    mRet.f[15] = mA.f[12] * mB.f[3] + mA.f[13] * mB.f[7] + mA.f[14] * mB.f[11] + mA.f[15] * mB.f[15];

    mOut = mRet;
}

void IlmMatrixTranslation(IlmMatrix &mOut, const float X, const float Y, const float Z)
{
    mOut.f[0] = 1.0f;
    mOut.f[1] = 0.0f;
    mOut.f[2] = 0.0f;
    mOut.f[3] = 0.0f;

    mOut.f[4] = 0.0f;
    mOut.f[5] = 1.0f;
    mOut.f[6] = 0.0f;
    mOut.f[7] = 0.0f;

    mOut.f[8] = 0.0f;
    mOut.f[9] = 0.0f;
    mOut.f[10] = 1.0f;
    mOut.f[11] = 0.0f;

    mOut.f[12] = X;
    mOut.f[13] = Y;
    mOut.f[14] = Z;
    mOut.f[15] = 1.0f;
}

void IlmMatrixScaling(IlmMatrix &mOut, const float X, const float Y, const float Z)
{
    mOut.f[0] = X;
    mOut.f[1] = 0.0f;
    mOut.f[2] = 0.0f;
    mOut.f[3] = 0.0f;

    mOut.f[4] = 0.0f;
    mOut.f[5] = Y;
    mOut.f[6] = 0.0f;
    mOut.f[7] = 0.0f;

    mOut.f[8] = 0.0f;
    mOut.f[9] = 0.0f;
    mOut.f[10] = Z;
    mOut.f[11] = 0.0f;

    mOut.f[12] = 0.0f;
    mOut.f[13] = 0.0f;
    mOut.f[14] = 0.0f;
    mOut.f[15] = 1.0f;
}

void IlmMatrixRotateX(IlmMatrix &mOut, const float angle)
{
    // Precompute cos and sin
    float fCosine = (float)cos(degToRad(angle));
    float fSine = (float)sin(degToRad(angle));

    // Create the trigonometric matrix corresponding to X Rotation
    mOut.f[0] = 1.0f;
    mOut.f[1] = 0.0f;
    mOut.f[2] = 0.0f;
    mOut.f[3] = 0.0f;

    mOut.f[4] = 0.0f;
    mOut.f[5] = fCosine;
    mOut.f[6] = -fSine;
    mOut.f[7] = 0.0f;

    mOut.f[8] = 0.0f;
    mOut.f[9] = fSine;
    mOut.f[10] = fCosine;
    mOut.f[11] = 0.0f;

    mOut.f[12] = 0.0f;
    mOut.f[13] = 0.0f;
    mOut.f[14] = 0.0f;
    mOut.f[15] = 1.0f;
}

void IlmMatrixRotateY(IlmMatrix &mOut, const float angle)
{
    // Precompute cos and sin
    float fCosine = (float)cos(degToRad(angle));
    float fSine = (float)sin(degToRad(angle));

    // Create the trigonometric matrix corresponding to Y Rotation
    mOut.f[0] = fCosine;
    mOut.f[1] = 0.0f;
    mOut.f[2] = fSine;
    mOut.f[3] = 0.0f;

    mOut.f[4] = 0.0f;
    mOut.f[5] = 1.0f;
    mOut.f[6] = 0.0f;
    mOut.f[7] = 0.0f;

    mOut.f[8]  = -fSine;
    mOut.f[9]  = 0.0f;
    mOut.f[10] = fCosine;
    mOut.f[11] = 0.0f;

    mOut.f[12] = 0.0f;
    mOut.f[13] = 0.0f;
    mOut.f[14] = 0.0f;
    mOut.f[15] = 1.0f;
}

void IlmMatrixRotateZ(IlmMatrix &mOut, const float angle)
{
    // Precompute cos and sin
    float fCosine = (float)cos(degToRad(angle));
    float fSine = (float)sin(degToRad(angle));

    // Create the trigonometric matrix corresponding to Z Rotation
    mOut.f[0] = fCosine;
    mOut.f[1] = -fSine;
    mOut.f[2] = 0.0f;
    mOut.f[3] = 0.0f;

    mOut.f[4] = fSine;
    mOut.f[5] = fCosine;
    mOut.f[6] = 0.0f;
    mOut.f[7] = 0.0f;

    mOut.f[8]  = 0.0f;
    mOut.f[9]  = 0.0f;
    mOut.f[10] = 1.0f;
    mOut.f[11] = 0.0f;

    mOut.f[12] = 0.0f;
    mOut.f[13] = 0.0f;
    mOut.f[14] = 0.0f;
    mOut.f[15] = 1.0f;
}

void IlmMatrixProjection(IlmMatrix &mOut, const float fov, const float near, const float far, const float aspect)
{
    // Precompute borders for projection
    float range = near * tan(degToRad(fov) / 2.0);
    float left = -range * aspect;
    float right = range * aspect;
    float bottom = -range;
    float top = range;

    // Column 1
    mOut.f[0] = 2 * near / (right - left);
    mOut.f[1] = 0.0;
    mOut.f[2] = 0.0;
    mOut.f[3] = 0.0;

    // Column 2
    mOut.f[4] = 0.0;
    mOut.f[5] = 2 * near / (top - bottom);
    mOut.f[6] = 0.0;
    mOut.f[7] = 0.0;

    // Column 3
    mOut.f[8] = 0.0;
    mOut.f[9] = 0.0;
    mOut.f[10] = -(far + near) / (far - near);
    mOut.f[11] = -1;

    // Column 4
    mOut.f[12] = 0.0;
    mOut.f[13] = 0.0;
    mOut.f[14] = -(2 * far * near) / (far - near);
    mOut.f[15] = 0.0;
}
