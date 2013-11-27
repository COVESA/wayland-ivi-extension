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

#include "Log.h"
#include <iomanip>
#include "config.h"
#ifdef WITH_DLT
#include <dlt/dlt.h>
#endif

Log* Log::m_instance = NULL;
Log::DiagnosticCallbackMap* Log::m_diagnosticCallbackMap = NULL;
LOG_MODES Log::fileLogLevel = LOG_DISABLED;
LOG_MODES Log::consoleLogLevel = LOG_INFO;
#ifdef WITH_DLT
LOG_MODES Log::dltLogLevel = LOG_DEBUG;
#else
LOG_MODES Log::dltLogLevel = LOG_DISABLED;
#endif


Log::Log()
{
    // TODO Auto-generated constructor stub
    m_fileStream = new std::ofstream("/tmp/LayerManagerService.log");
    pthread_mutex_init(&m_LogBufferMutex, NULL);
    Log::m_diagnosticCallbackMap = new Log::DiagnosticCallbackMap;
#ifdef WITH_DLT
    m_logContext = new DltContext;
    DLT_REGISTER_APP("LMSA", "LayerManagerService");
    DLT_REGISTER_CONTEXT(*((DltContext*)m_logContext), "LMSC", "LayerManagerService");
    DLT_SET_APPLICATION_LL_TS_LIMIT(DLT_LOG_VERBOSE, DLT_TRACE_STATUS_DEFAULT);
#else
    m_logContext = NULL;
#endif
}

Log* Log::getInstance()
{
    if (m_instance == NULL)
    {
        m_instance = new Log();
    }
    return m_instance;
}

void Log::closeInstance()
{
    delete m_instance;
    m_instance = NULL;
}

Log::~Log()
{
    // TODO Auto-generated destructor stub
    m_fileStream->close();
    pthread_mutex_destroy(&m_LogBufferMutex);
    Log::m_instance = NULL;
#ifdef WITH_DLT
    DLT_UNREGISTER_CONTEXT(*((DltContext*)m_logContext));
    delete((DltContext*)m_logContext);
    DLT_UNREGISTER_APP();
#endif
    delete m_diagnosticCallbackMap;
    m_diagnosticCallbackMap = NULL;
    m_logContext = NULL;
}

void Log::warning(LogContext logContext, const std::string& moduleName, const std::basic_string<char>& output)
{
    log(logContext, LOG_WARNING, moduleName, output);
}

void Log::info(LogContext logContext, const std::string& moduleName, const std::basic_string<char>& output)
{
    log(logContext, LOG_INFO, moduleName, output);
}

void Log::error(LogContext logContext, const std::string& moduleName, const std::basic_string<char>& output)
{
    log(logContext, LOG_ERROR, moduleName, output);
}

void Log::debug(LogContext logContext, const std::string& moduleName, const std::basic_string<char>& output)
{
    log(logContext, LOG_DEBUG, moduleName, output);
}

void Log::log(LogContext logContext, LOG_MODES logMode, const std::string& moduleName, const std::basic_string<char>& output)
{
    (void)logContext;

    std::string logString[LOG_MAX_LEVEL] = {"", "ERROR", "INFO", "WARNING", "DEBUG"};
    std::string logOutLevelString = logString[LOG_INFO];
    pthread_mutex_lock(&m_LogBufferMutex);
    if (logMode < LOG_MAX_LEVEL)
    {
        logOutLevelString = logString[logMode];
    }
    if (consoleLogLevel >= logMode)
    {
        LogToConsole(logOutLevelString, moduleName, output);
    }
    if (fileLogLevel >= logMode)
    {
        LogToFile(logOutLevelString, moduleName, output);
    }
#ifdef WITH_DLT
    if (dltLogLevel >= logMode)
    {
        LogToDltDaemon(logContext, logMode, moduleName, output);
    }
#endif
    pthread_mutex_unlock(&m_LogBufferMutex);
}

void Log::LogToFile(std::string logMode, const std::string& moduleName, const std::basic_string<char>& output)
{
    static unsigned int maxLengthModuleName = 0;
    static unsigned int maxLengthLogModeName = 0;

    if (moduleName.length() > maxLengthModuleName)
    {
        maxLengthModuleName = moduleName.length();
    }

    if (logMode.length() > maxLengthLogModeName)
    {
        maxLengthLogModeName = logMode.length();
    }

    *m_fileStream << std::setw(maxLengthModuleName)  << std::left << moduleName << " | "
                << std::setw(maxLengthLogModeName) << std::left << logMode << " | "
                << output << std::endl;
}

void Log::LogToConsole(std::string logMode, const std::string& moduleName, const std::basic_string<char>& output)
{
    static unsigned int maxLengthModuleName = 0;
    static unsigned int maxLengthLogModeName = 0;

    if (moduleName.length() > maxLengthModuleName)
    {
        maxLengthModuleName = moduleName.length();
    }

    if (logMode.length() > maxLengthLogModeName)
    {
        maxLengthLogModeName = logMode.length();
    }

    std::cout << std::setw(maxLengthModuleName)  << std::left << moduleName << " | "
                << std::setw(maxLengthLogModeName) << std::left << logMode << " | "
                << output << std::endl;
}

LogContext Log::getLogContext()
{
    return m_logContext;
}

#ifdef WITH_DLT
int dlt_injection_callback(unsigned int module_id, void *data, unsigned int length)
{
    LOG_DEBUG("LOG", "Injection for service " << module_id << " called");
    Log::diagnosticCallbackData *cbData = (*Log::getDiagnosticCallbackMap())[module_id];
    if (cbData)
    {
        cbData->diagFunc(module_id, data, length, cbData->userdata);
    }
    return 0;
}
#endif

void Log::registerDiagnosticInjectionCallback(unsigned int module_id, diagnosticInjectionCallback diagFunc, void* userdata)
{
    Log::diagnosticCallbackData *cbData = new Log::diagnosticCallbackData;
    cbData->module_id = module_id;
    cbData->userdata = userdata;
    cbData->diagFunc = diagFunc;
    (*m_diagnosticCallbackMap)[module_id] = cbData;
#ifdef WITH_DLT
    DLT_REGISTER_INJECTION_CALLBACK(*(DltContext*)m_logContext, module_id, dlt_injection_callback);
#endif
}

void Log::unregisterDiagnosticInjectionCallback(unsigned int module_id)
{
    m_diagnosticCallbackMap->erase(module_id);
}

#ifdef WITH_DLT

// DLT macros will fail using -pedantic with
// warning: ISO C++ forbids braced-groups within expressions
#pragma GCC diagnostic ignored "-pedantic"

void Log::LogToDltDaemon(LogContext logContext, LOG_MODES logMode, const std::string& moduleName, const std::basic_string<char>& output)
{
    std::stringstream oss;
    std::string dltString;
    static unsigned int maxLengthModuleName = 0;

    if (moduleName.length() > maxLengthModuleName)
    {
        maxLengthModuleName = moduleName.length();
    }
    oss << std::setw(maxLengthModuleName)  << std::left << moduleName << " | "
        << output << std::endl;
    dltString = oss.str();

    switch (logMode)
    {
    case LOG_INFO:
        DLT_LOG(*((DltContext*)logContext), DLT_LOG_INFO, DLT_STRING(dltString.c_str()));
        break;

    case LOG_ERROR:
        DLT_LOG(*((DltContext*)logContext), DLT_LOG_ERROR, DLT_STRING(dltString.c_str()));
        break;

    case LOG_DEBUG:
        DLT_LOG(*((DltContext*)logContext), DLT_LOG_DEBUG, DLT_STRING(dltString.c_str()));
        break;

    case LOG_WARNING:
        DLT_LOG(*((DltContext*)logContext), DLT_LOG_WARN, DLT_STRING(dltString.c_str()));
        break;

    default:
        DLT_LOG(*((DltContext*)logContext), DLT_LOG_INFO, DLT_STRING(dltString.c_str()));
    }
}
#endif
