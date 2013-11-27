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
#include "Argument.h"
#include <iostream>
#include <stdlib.h>

BoolArgument::BoolArgument(string name, bool default_value, int argc, const char * const argv[])
: value(default_value)
{
    for (int arg = 1; arg < argc; ++arg) {
        const char* lpArgNow = argv[arg];

        if ((lpArgNow[0] == '-') && (name == &lpArgNow[1])) {
            value = !value;
        }
    }
}

bool BoolArgument::get()
{
    return value;
}

FloatArgument::FloatArgument(string name, float default_value, int argc, const char * const argv[])
: value(default_value)
{
    for (int arg = 1; arg < argc; ++arg)
    {
        char dummy[] = "";
        const char* lpArgNow = argv[arg];
        const char* lpArgNext = dummy;

        if (arg != argc - 1)
        {
            lpArgNext = argv[arg + 1];
        }

        if ((lpArgNow[0] == '-') && (name == &lpArgNow[1]))
        {
            value = atof(lpArgNext);
        }
    }
}

float FloatArgument::get()
{
    return value;
}


IntArgument::IntArgument(string name, int default_value, int argc, const char * const argv[])
: value(default_value)
{
    for (int arg = 1; arg < argc; ++arg)
    {
        char dummy[] = "";
        const char* lpArgNow = argv[arg];
        const char* lpArgNext = dummy;

        if (arg != argc - 1)
        {
            lpArgNext = argv[arg + 1];
        }

        if ((lpArgNow[0] == '-') && (name == &lpArgNow[1]))
        {
            value = atoi(lpArgNext);
        }
    }
}

int IntArgument::get()
{
    return value;
}


UnsignedIntArgument::UnsignedIntArgument(string name, unsigned int default_value, int argc, const char * const argv[])
: value(default_value)
{
    for (int arg = 1; arg < argc; ++arg)
    {
        char dummy[] = "";
        const char* lpArgNow = argv[arg];
        const char* lpArgNext = dummy;

        if (arg != argc - 1)
        {
            lpArgNext = argv[arg + 1];
        }

        if ((lpArgNow[0] == '-') && (name == &lpArgNow[1]))
        {
            value = atoll(lpArgNext);
        }
    }
}

unsigned int UnsignedIntArgument::get()
{
    return value;
}
