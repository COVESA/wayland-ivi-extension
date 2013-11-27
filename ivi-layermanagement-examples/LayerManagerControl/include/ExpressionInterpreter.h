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
#ifndef __EXPRESSIONINTERPRETER_H__
#define __EXPRESSIONINTERPRETER_H__

#include "Expression.h"
#include <string>
using namespace std;

enum CommandResult
{
    CommandSuccess,
    CommandIncomplete,
    CommandInvalid,
    CommandExecutionFailed
};

class ExpressionInterpreter
{
public:
    ExpressionInterpreter();
    CommandResult interpretCommand(string userInput);
    string getLastError();
    static void printExpressionTree();
    static void printExpressionList();

    static bool addExpression(callback funcPtr, string command);

private:
    static Expression* mpRoot;
    string mErrorText;
};

#endif // __EXPRESSIONINTERPRETER_H__
