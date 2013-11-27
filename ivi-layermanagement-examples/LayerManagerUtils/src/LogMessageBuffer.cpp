/***************************************************************************
*   Copyright (C) 2011 BMW Car IT GmbH.
*   Author: Michael Schuldt (michael.schuldt@bmw-carit.de)
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*
* This file incorporates work covered by the following copyright and
* permission notice:
* Apache log4cxx
* Copyright 2004-2007 The Apache Software Foundation
*
* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "LogMessageBuffer.h"

LogMessageBuffer::LogMessageBuffer()
: stream(0)
{
}

LogMessageBuffer::~LogMessageBuffer()
{
    if (stream)
    {
        delete stream;
    }
}

LogMessageBuffer& LogMessageBuffer::operator<<(const std::basic_string<char>& msg)
{
    if (stream)
    {
        *stream << msg;
    }
    else
    {
        buf.append(msg);
    }
    return *this;
}

LogMessageBuffer& LogMessageBuffer::operator<<(const char* msg)
{
    const char* actualMsg = msg;
    if (!actualMsg)
    {
        actualMsg = "null";
    }

    if (stream)
    {
        *stream << actualMsg;
    }
    else
    {
        buf.append(actualMsg);
    }
    return *this;
}

LogMessageBuffer& LogMessageBuffer::operator<<(char* msg)
{
    return operator<<((const char*) msg);
}

LogMessageBuffer& LogMessageBuffer::operator<<(const char msg)
{
    if (stream)
    {
        buf.assign(1, msg);
        *stream << buf;
    }
    else
    {
        buf.append(1, msg);
    }
    return *this;
}

LogMessageBuffer::operator std::basic_ostream<char>&()
{
    if (!stream)
    {
        stream = new std::basic_ostringstream<char>();

        if (!buf.empty())
        {
            *stream << buf;
        }
    }
    return *stream;
}

const std::basic_string<char>& LogMessageBuffer::str(std::basic_ostream<char>&)
{
    buf = stream->str();
    return buf;
}

const std::basic_string<char>& LogMessageBuffer::str() const
{
    return buf;
}

bool LogMessageBuffer::hasStream() const
{
    return (stream != 0);
}

std::ostream& LogMessageBuffer::operator<<(ios_base_manip manip)
{
    std::ostream& s = *this;
    (*manip)(s);
    return s;
}

std::ostream& LogMessageBuffer::operator<<(bool val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(short val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(int val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(unsigned int val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(long val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(unsigned long val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(float val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(double val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(long double val)
{
    return ((std::ostream&) *this).operator<<(val);
}

std::ostream& LogMessageBuffer::operator<<(void* val)
{
    return ((std::ostream&) *this).operator<<(val);
}
