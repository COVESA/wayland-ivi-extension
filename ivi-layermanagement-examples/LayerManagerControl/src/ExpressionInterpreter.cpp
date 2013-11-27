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
#include "Expression.h"
#include "ilm_client.h"
#include <string>
#include <sstream>
#include <algorithm> // transform
#include <ctype.h> // tolower

#include <iostream>

Expression* ExpressionInterpreter::mpRoot = NULL;

ExpressionInterpreter::ExpressionInterpreter()
: mErrorText("No error.")
{
}

bool ExpressionInterpreter::addExpression(callback funcPtr, string command)
{
    bool result = false;

    string text;
    stringstream ss;
    ss << command;

    if (!mpRoot)
    {
        mpRoot = new Expression("[root]", NULL);
    }

    Expression* currentWord = mpRoot;

    while (!ss.eof())
    {
        ss >> text;
        transform(text.begin(), text.end(), text.begin(), ::tolower);

        Expression* nextWord = currentWord->getNextExpression(text);

        if (!nextWord)
        {
            nextWord = new Expression(text, currentWord);
            currentWord->addNextExpression(nextWord);
        }

        currentWord = nextWord;
    }

    currentWord->setFunc(funcPtr);

    return result;
}

CommandResult ExpressionInterpreter::interpretCommand(string userInput)
{
    CommandResult result = CommandSuccess;
    string text;
    stringstream ss;
    ss << userInput;

    ExpressionList currentState;
    currentState.push_back(mpRoot);
    ExpressionList nextState;

    while (result == CommandSuccess && !ss.eof())
    {
        ss >> text;
        transform(text.begin(), text.end(), text.begin(), ::tolower);

        ExpressionList::const_iterator iter = currentState.begin();
        ExpressionList::const_iterator end = currentState.end();
        for (; iter != end; ++iter)
        {
            Expression* expr = *iter;
            ExpressionList exprNextList = expr->getNextExpressionClosure(text);
            nextState.splice(nextState.end(), exprNextList);
        }

        if (nextState.size() > 0)
        {
            currentState = nextState;
            nextState.clear();
        }
        else
        {
            mErrorText = "'" + text + "' not recognized.";
            result = CommandInvalid;
        }
    }

    //remove impossible expressions in the final state before checking for ambiguity
    nextState.clear();
    ExpressionList::const_iterator iter = currentState.begin();
    ExpressionList::const_iterator end = currentState.end();
    for (; iter != end; ++iter)
    {
        Expression* expr = *iter;
        if (expr->isExecutable())
        {
            nextState.push_back(expr);
        }
        else
        {
            ExpressionList children = expr->getNextExpressions();

            bool flag = false;

            ExpressionList::const_iterator iter = children.begin();
            ExpressionList::const_iterator end = children.end();
            for (; iter != end; ++iter)
            {
                if ((*iter)->getName()[0] == '[')
                {
                    flag = true;
                }
            }

            if (flag || children.size() == 0)
            {
                nextState.push_back(expr);
            }
        }
    }

    currentState = nextState;

    if (currentState.size() != 1)
    {
        mErrorText = "'" + text + "' ambiguous or incomplete.";
        result = CommandInvalid;
    }

    //run command if executable and non-ambiguous
    if (result == CommandSuccess)
    {
        Expression* expr = *(currentState.begin());

        ExpressionList executables = expr->getClosureExecutables(false);
        if (executables.size() == 1)
        {
            ilmErrorTypes initResult = ilm_init();
            if (ILM_SUCCESS != initResult)
            {
                mErrorText = ILM_ERROR_STRING(initResult);
                result = CommandExecutionFailed;
            }
            else
            {
                Expression* exec = executables.front();
                exec->execute();
                ilm_destroy();
            }
        }
        else if (executables.size() == 0)
        {
            mErrorText = "command is incomplete.";
            result = CommandIncomplete;
        }
        else
        {
            mErrorText = "command is ambiguous.";
            result = CommandIncomplete;
        }
    }

    return result;
}

void ExpressionInterpreter::printExpressionTree()
{
    mpRoot->printTree();
}

void ExpressionInterpreter::printExpressionList()
{
    mpRoot->printList();
}

string ExpressionInterpreter::getLastError()
{
    string tmp = mErrorText;
    mErrorText = "no error.";
    return tmp;
}
