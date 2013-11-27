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

#ifndef _LOGMESSAGEBUFFER_H_
#define _LOGMESSAGEBUFFER_H_

#include <sstream>
#include <pthread.h>
typedef std::ios_base& (*ios_base_manip)(std::ios_base&);

class LogMessageBuffer
{
public:
    /**
     *  Creates a new instance.
     */
    LogMessageBuffer();
    /**
     *  Destructor.
     */
    ~LogMessageBuffer();

    LogMessageBuffer& operator<<(const std::basic_string<char>& msg);

    LogMessageBuffer& operator<<(const char* msg);

    LogMessageBuffer& operator<<(char* msg);

    LogMessageBuffer& operator<<(const char msg);

    std::ostream& operator<<(ios_base_manip manip);

    std::ostream& operator<<(bool val);

    std::ostream& operator<<(short val);

    std::ostream& operator<<(int val);

    std::ostream& operator<<(unsigned int val);

    std::ostream& operator<<(long val);

    std::ostream& operator<<(unsigned long val);

    std::ostream& operator<<(float val);

    std::ostream& operator<<(double val);

    std::ostream& operator<<(long double val);

    std::ostream& operator<<(void* val);

    /**
     *  Cast to ostream.
     */
    operator std::basic_ostream<char>&();

    const std::basic_string<char>& str(std::basic_ostream<char>& os);

    const std::basic_string<char>& str() const;

    bool hasStream() const;

private:
    /**
     * No default copy constructor.
     */
    LogMessageBuffer(const LogMessageBuffer&);

    /**
     *  No assignment operator.
     */
    LogMessageBuffer& operator=(const LogMessageBuffer&);

    /**
     * Encapsulated std::string.
     */
    std::basic_string<char> buf;

    /**
     *  Encapsulated stream, created on demand.
     */
    std::basic_ostringstream<char>* stream;
};

template<class V>
std::basic_ostream<char>& operator<<(LogMessageBuffer& os, const V& val)
{
    return ((std::basic_ostream<char>&) os) << val;
}

#endif /* _LOGMESSAGEBUFFER_H_ */
