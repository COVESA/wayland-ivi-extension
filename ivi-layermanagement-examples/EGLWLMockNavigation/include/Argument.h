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
#ifndef _ARGUMENT_H
#define _ARGUMENT_H

#include <string>
using std::string;

class BoolArgument {
public:
    BoolArgument(string name, bool default_value, int argc, const char * const argv[]);
    bool get();

private:
    bool value;
};

class FloatArgument {
public:
    FloatArgument(string name, float default_value, int argc, const char * const argv[]);
    float get();

private:
    float value;
};

class IntArgument {
public:
    IntArgument(string name, int default_value, int argc, const char * const argv[]);
    int get();

private:
    int value;
};

class UnsignedIntArgument {
public:
    UnsignedIntArgument(string name, unsigned int default_value, int argc, const char * const argv[]);
    unsigned int get();

private:
    unsigned int value;
};

#endif /* _ARGUMENT_H */
