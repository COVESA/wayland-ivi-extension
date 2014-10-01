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

#ifndef _ILMMATRIX_H_
#define _ILMMATRIX_H_

#define MAT00     0
#define MAT01     1
#define MAT02     2
#define MAT03     3
#define MAT10     4
#define MAT11     5
#define MAT12     6
#define MAT13     7
#define MAT20     8
#define MAT21     9
#define MAT22     10
#define MAT23     11
#define MAT30     12
#define MAT31     13
#define MAT32     14
#define MAT33     15

typedef struct
{
    float x;
    float y;
} IlmVector2f;

typedef struct
{
    float x;
    float y;
    float z;
} IlmVector3f;

typedef struct
{
    float x;
    float y;
    float z;
    float w;
} IlmVector4f;

class IlmMatrix
{
public:
    float* operator [] (const int row)
    {
        return &f[row<<2];
    }

    float f[16];
};

void IlmMatrixIdentity(IlmMatrix &mOut);

void IlmMatrixMultiply(IlmMatrix &mOut, const IlmMatrix &mA, const IlmMatrix &mB);

void IlmMatrixTranslation(IlmMatrix &mOut, const float X, const float Y, const float Z);

void IlmMatrixScaling(IlmMatrix &mOut, const float X, const float Y, const float Z);

void IlmMatrixRotateX(IlmMatrix &mOut, const float angle);

void IlmMatrixRotateY(IlmMatrix &mOut, const float angle);

void IlmMatrixRotateZ(IlmMatrix &mOut, const float angle);

void IlmMatrixProjection(IlmMatrix &mOut, const float fov, const float near, const float far, const float aspectRatio);


#endif /* _ILMMATRIX_H_*/
