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


#include <gtest/gtest.h>
#include <stdio.h>

#include "Log.h"

#define WIDTH 1
#define HEIGHT 1

std::string moduleName = "TheModuleName";
std::string logFileName = "/tmp/LayerManagerService.log";
int i=0;

bool checkLogFileForMessage(std::string message)
{
    std::string line;
    std::ifstream logFile(logFileName.c_str());

    if (logFile.is_open())
    {
        while ( logFile.good() )
        {
            getline (logFile,line);
            int position = line.find(message);
            if (position >0)
                return true;
        }
        logFile.close();
    }
    // did not find the message in any line of the logfile
    return false;
}


TEST(LogTest, writeAWarning) {
    Log::getInstance()->fileLogLevel = LOG_MAX_LEVEL;
    LOG_WARNING(moduleName, "writeAWarning");

    ASSERT_TRUE(checkLogFileForMessage("writeAWarning"));
}

TEST(LogTest, writeAnError) {
    Log::getInstance()->fileLogLevel = LOG_INFO;
    LOG_ERROR(moduleName, "writeAnError");

    ASSERT_TRUE(checkLogFileForMessage("writeAnError"));
}

TEST(LogTest, writeAnInformation) {
    Log::getInstance()->fileLogLevel = LOG_INFO;
    LOG_INFO(moduleName, "writeAnInformation");

    ASSERT_TRUE(checkLogFileForMessage("writeAnInformation"));
}


TEST(LogTest, logSomethingWithLowerLogLevel) {

    // when disabled, nothing should ever be logged
    Log::getInstance()->fileLogLevel = LOG_DISABLED;
    LOG_ERROR(moduleName, "err1");
    ASSERT_FALSE(checkLogFileForMessage("err1"));

    LOG_DEBUG(moduleName, "debug1");
    ASSERT_FALSE(checkLogFileForMessage("debug1"));

    LOG_WARNING(moduleName, "warn1");
    ASSERT_FALSE(checkLogFileForMessage("warn1"));

    LOG_INFO(moduleName, "info1");
    ASSERT_FALSE(checkLogFileForMessage("info1"));

    // when error is set, only higher levels may be logged
    Log::getInstance()->fileLogLevel = LOG_ERROR;
    LOG_ERROR(moduleName, "err2");
    ASSERT_TRUE(checkLogFileForMessage("err2"));

    LOG_DEBUG(moduleName, "debug2");
    ASSERT_FALSE(checkLogFileForMessage("debug2"));

    LOG_WARNING(moduleName, "warn2");
    ASSERT_FALSE(checkLogFileForMessage("warn2"));

    LOG_INFO(moduleName, "info2");
    ASSERT_FALSE(checkLogFileForMessage("info2"));

    // when debug is set, only higher levels may be logged
    Log::getInstance()->fileLogLevel = LOG_DEBUG;
    LOG_ERROR(moduleName, "err3");
    ASSERT_TRUE(checkLogFileForMessage("err3"));

    LOG_DEBUG(moduleName, "debug3");
    ASSERT_TRUE(checkLogFileForMessage("debug3"));

    LOG_WARNING(moduleName, "warn3");
    ASSERT_TRUE(checkLogFileForMessage("warn3"));

    LOG_INFO(moduleName, "info3");
    ASSERT_TRUE(checkLogFileForMessage("info3"));

    // when info is set, only higher levels may be logged
    Log::getInstance()->fileLogLevel = LOG_INFO;
    LOG_ERROR(moduleName, "err4");
    ASSERT_TRUE(checkLogFileForMessage("err4"));

    LOG_DEBUG(moduleName, "debug4");
    ASSERT_FALSE(checkLogFileForMessage("debug4"));

    LOG_WARNING(moduleName, "warn4");
    ASSERT_FALSE(checkLogFileForMessage("warn4"));

    LOG_INFO(moduleName, "info4");
    ASSERT_TRUE(checkLogFileForMessage("info4"));

}
