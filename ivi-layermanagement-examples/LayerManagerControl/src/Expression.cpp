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
#include "Expression.h"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>  // memcpy
#include <algorithm>

Expression::Expression(string name, Expression* parent)
: mName(name)
, mPreviousWord(parent)
, mFuncPtr(NULL)
, mMatchText("")
{
}

void Expression::setVarValue(string value)
{
    mVarValue = value;
}

bool Expression::isVar()
{
    return mName[0] == '<' || (mName[0] == '[' && mName[1] == '<');
}

string Expression::getString(string name)
{
    //remove brackets if needed
    string noBrackets = mName;
    noBrackets = noBrackets[0] == '[' ? noBrackets.substr(1, noBrackets.size() - 1) : noBrackets;
    noBrackets = noBrackets[noBrackets.size() - 1] == ']' ? noBrackets.substr(0, noBrackets.size() - 1) : noBrackets;

    noBrackets = noBrackets[0] == '<' ? noBrackets.substr(1, noBrackets.size() - 1) : noBrackets;
    noBrackets = noBrackets[noBrackets.size() - 1] == '>' ? noBrackets.substr(0, noBrackets.size() - 1) : noBrackets;

    //remove default value (if needed)
    string exprName = noBrackets.substr(0, noBrackets.find("="));

    if (exprName == name)
    {
        if (mMatchText != "")
        {
            //if there was a match return the value
            return mVarValue;
        }
        else if (noBrackets.find("=") != string::npos)
        {
            //return default value
            return noBrackets.substr(noBrackets.find("=") + 1);
        }
    }
    else if (mPreviousWord)
    {
        return mPreviousWord->getString(name);
    }

    return "";
}

unsigned int Expression::getUint(string name)
{
    string stringVal = getString(name);

    unsigned int value = 0;
    sscanf(stringVal.c_str(), "%u", &value);

    if (!value)
    {
        sscanf(stringVal.c_str(), "0x%x", &value);
    }
    return value;
}

void Expression::getUintArray(string name, unsigned int** array, unsigned int* count)
{
    stringstream ss;
    ss << getString(name);

    unsigned int buffer[256]; // more than enough for all cases
    *count = 0;

    string stringVal;
    while (getline(ss, stringVal, ','))
    {
        sscanf(stringVal.c_str(), "%u", &buffer[*count]);

        if (!buffer[*count])
        {
            sscanf(stringVal.c_str(), "0x%x", &buffer[*count]);
        }
        ++(*count);
    }

    *array = new unsigned int[*count];
    memcpy(*array, buffer, sizeof(unsigned int) * (*count));
}

int Expression::getInt(string name)
{
    string stringVal = getString(name);

    int value = 0;
    sscanf(stringVal.c_str(), "%d", &value);

    if (!value)
    {
        sscanf(stringVal.c_str(), "0x%x", (unsigned int*) &value);
    }
    return value;
}

double Expression::getDouble(string name)
{
    string stringVal = getString(name);

    double value = 0;
    sscanf(stringVal.c_str(), "%lf", &value);
    return value;
}

bool Expression::getBool(string name)
{
    string stringVal = getString(name);
    int value = 0;
    return sscanf(stringVal.c_str(), "%d", &value) && value;
}

bool Expression::contains(string name)
{
    return mMatchText == name || (mPreviousWord && mPreviousWord->contains(name));
}

string Expression::getName()
{
    return mName;
}

bool ExpressionCompare(Expression* a, Expression* b)
{
    return a->getName() < b->getName();
}

void Expression::addNextExpression(Expression* word)
{
    mNextWords.push_back(word);
    mNextWords.sort(ExpressionCompare);
}

ExpressionList Expression::getClosure(bool bypass)
{
    ExpressionList closure;

    if (bypass)
    {
        //if expression is end of the optional expression
        bool bypassChildren = mName[mName.length() - 1] != ']';
        //get closure of children
        ExpressionList::const_iterator iter = mNextWords.begin();
        ExpressionList::const_iterator end = mNextWords.end();
        for (; iter != end; ++iter)
        {
            ExpressionList childClosure = (*iter)->getClosure(bypassChildren);
            closure.splice(closure.end(), childClosure);
        }
    }
    else
    {
        closure.push_back(this);
        //if start of optional expression
        if (mName[0] == '[')
        {
            //get closure of elements after the end of the expression
            ExpressionList restClosure = getClosure(true);
            closure.splice(closure.end(), restClosure);
        }
    }

    return closure;
}

ExpressionList Expression::getNextExpressionClosure(string text)
{
    ExpressionList nextClosure;

    ExpressionList::const_iterator iter = mNextWords.begin();
    ExpressionList::const_iterator end = mNextWords.end();
    for (; iter != end; ++iter)
    {
        Expression* childExpr = *iter;
        ExpressionList childClosure = childExpr->getClosure(false);

        ExpressionList::const_iterator iter = childClosure.begin();
        ExpressionList::const_iterator end = childClosure.end();
        for (; iter != end; ++iter)
        {
            Expression* expr = *iter;

            if (expr->isVar())
            {
                nextClosure.push_back(expr);

                expr->setVarValue(text);

                string exprName = expr->mName;

                //remove brakcets
                exprName = exprName[0] == '[' ? exprName.substr(1, exprName.size() - 1) : exprName;
                exprName = exprName[exprName.size() - 1] == ']' ? exprName.substr(0, exprName.size() - 1) : exprName;

                exprName = exprName[0] == '<' ? exprName.substr(1, exprName.size()-1) : exprName;
                exprName = exprName[exprName.size() - 1] == '>' ? exprName.substr(0, exprName.size() - 1) : exprName;

                //remove default value (if needed)
                exprName = exprName.substr(0, exprName.find("="));

                expr->mMatchText = exprName;
            }
            else
            {
                //remove brackets if needed
                string exprName = expr->mName;
                exprName = exprName[0] == '[' ? exprName.substr(1, exprName.size() - 1) : exprName;
                exprName = exprName[exprName.size() - 1] == ']' ? exprName.substr(0, exprName.size() - 1) : exprName;

                //check all alternatives (in case there are alternatives)
                while (exprName.length() > 0)
                {
                    //get next part
                    string temp = exprName.substr(0, exprName.find("|", 1));
                    exprName = exprName.substr(temp.length());

                    //it there is a '|' at beginning remove it
                    temp = temp[0] == '|' ? temp.substr(1) : temp;
                    if (temp == text)
                    {
                        //add to result !
                        nextClosure.push_back(expr);
                        expr->mMatchText = text;
                        break; //from inner loop !
                    }
                }
            }
        }
    }

    return nextClosure;
}

ExpressionList Expression::getNextExpressions()
{
    return this->mNextWords;
}

Expression* Expression::getNextExpression(string text)
{
    Expression* varMatch = NULL;
    Expression* nameMatch = NULL;

    ExpressionList::const_iterator iter = mNextWords.begin();
    ExpressionList::const_iterator end = mNextWords.end();
    for (; iter != end; ++iter)
    {
        Expression* expr = *iter;
        string exprName = expr->getName();

        if (exprName == text)
        {
            nameMatch = expr;
        }

        if (expr->isVar())
        {
            varMatch = expr;
            varMatch->setVarValue(text);
        }
    }

    return nameMatch ? nameMatch : (varMatch ? varMatch : NULL);
}

ExpressionList Expression::getClosureExecutables(bool canBypass)
{
    ExpressionList candidateExecutables;

    if (canBypass)
    {
        string expName = this->mName;
        if (mName[mName.length() - 1] == ']')
        {
            //as if this child was the "last consumed" expression by the user string input !
            ExpressionList childExecutables = getClosureExecutables(false);
            candidateExecutables.splice(candidateExecutables.end(), childExecutables);
        }
        else
        {
            ExpressionList::const_iterator iter = mNextWords.begin();
            ExpressionList::const_iterator end = mNextWords.end();
            for (; iter != end; ++iter)
            {
                string childName = (*iter)->mName;

                ExpressionList childClosure = (*iter)->getClosureExecutables(true);
                candidateExecutables.splice(candidateExecutables.end(), childClosure);
            }
        }
    }
    else
    {
        //add myself to candidate executables
        candidateExecutables.push_back(this);

        //get candidate executables from children
        ExpressionList::const_iterator iter = mNextWords.begin();
        ExpressionList::const_iterator end = mNextWords.end();
        for (; iter != end; ++iter)
        {
            //if child is start of optional expression: get executable closure from the ends of this child
            string childName = (*iter)->mName;
            if (childName[0] == '[')
            {
                ExpressionList childClosure = (*iter)->getClosureExecutables(true);
                candidateExecutables.splice(candidateExecutables.end(), childClosure);
            }
        }
    }

    //return only the executable expressions
    ExpressionList executables;

    ExpressionList::const_iterator iter = candidateExecutables.begin();
    ExpressionList::const_iterator end = candidateExecutables.end();
    for (; iter != end; ++iter)
    {
        Expression* expr = *iter;

        //check if it already exists in the list
        bool duplicate = std::find(executables.begin(), executables.end(), expr) != executables.end();

        if ((! duplicate) && expr->isExecutable())
        {
            executables.push_back(expr);
        }
    }

    return executables;
}

void Expression::printTree(int level)
{
    for (int i = 0; i < level; ++i)
    {
        cout << ((i + 1 != level) ? "|  " : "|--");
    }

    stringstream name;
    name << mName;

    if (isExecutable())
    {
        name << "*";
    }

    cout << name.str() << endl;

    ExpressionList::const_iterator iter = mNextWords.begin();
    ExpressionList::const_iterator end = mNextWords.end();
    for (; iter != end; ++iter)
    {
        (*iter)->printTree(level + 1);
    }
}

void Expression::printList(string list)
{
    if (mName != "[root]")
    {
        list += mName;
        list += " ";
        if (isExecutable())
        {
            cout << list << "\n";
        }
    }

    ExpressionList::const_iterator iter = mNextWords.begin();
    ExpressionList::const_iterator end = mNextWords.end();
    for (; iter != end; ++iter)
    {
        (*iter)->printList(list);
    }
}

bool Expression::isExecutable()
{
    return mFuncPtr;
}

void Expression::execute()
{
    (*mFuncPtr)(this);
}

void Expression::setFunc(callback funcPtr)
{
    mFuncPtr = funcPtr;
}

Expression* Expression::getPreviousExpression()
{
    return mPreviousWord;
}
