/***************************************************************************
 *
 * Copyright 2012 BMW Car IT GmbH
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
#include "ExpressionInterpreter.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
    ExpressionInterpreter interpreter;

    // create full string of arguments
    string userCommand;

    if (argc == 1)
    {
        userCommand = "help";
    }
    else
    {
        for (int i = 1; i < argc; ++i)
        {
            userCommand += argv[i];
            userCommand += ' ';
        }
        userCommand = userCommand.substr(0, userCommand.size() - 1);
    }

    // start interpreter
    if (CommandSuccess != interpreter.interpretCommand(userCommand))
    {
        cerr << "Interpreter error: " << interpreter.getLastError() << endl;
    }

    return 0;
}
