/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
#ifndef _VEC_H
#define _VEC_H

#include <GLES2/gl2.h>

template <class T>
class vec2
{
public:
    vec2()
    {
    }

    vec2(T _x, T _y)
    : x(_x)
    , y(_y)
    {
    }

    struct
    {
        T x;
        T y;
    };
};

template <class T>
class vec3
{
public:
    vec3()
    {
    }

    vec3(T _x, T _y, T _z)
    : x(_x)
    , y(_y)
    , z(_z)
    {
    }

    union
    {
        struct
        {
            T x;
            T y;
            T z;
        };
        struct
        {
            T r;
            T g;
            T b;
        };
    };
};

template <class T>
class vec4
{
public:
    vec4()
    {
    }

    vec4(T _x, T _y, T _z, T _w)
    : x(_x)
    , y(_y)
    , z(_z)
    , w(_w)
    {
    }

    union
    {
        struct
        {
            T x;
            T y;
            T z;
            T w;
        };
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
    };
};

typedef vec2<GLfloat> vec2f;
typedef vec2<GLint>   vec2i;
typedef vec2<GLushort>  vec2u;

typedef vec3<GLfloat> vec3f;
typedef vec3<GLint>   vec3i;
typedef vec3<GLushort>  vec3u;

typedef vec4<GLfloat> vec4f;
typedef vec4<GLint>   vec4i;
typedef vec4<GLushort>  vec4u;

#endif /* _VEC_H */
