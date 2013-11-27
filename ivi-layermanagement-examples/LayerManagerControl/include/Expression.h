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
#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__

#include <string>
#include <list>
using namespace std;

class Expression;

typedef void(*callback)(Expression*);
typedef list<Expression*> ExpressionList;

class Expression
{
public:
    Expression(string name, Expression* parent);
    string getName();

    Expression* getPreviousExpression();
    void addNextExpression(Expression* word);
    Expression* getNextExpression(string text);
    ExpressionList getNextExpressions();
    ExpressionList getClosure(bool bypass);
    ExpressionList getNextExpressionClosure(string text);
    ExpressionList getClosureExecutables(bool canBypass);

    void setFunc(callback funcPtr);
    void execute();
    bool isExecutable();

    bool isVar();
    void setVarValue(string value);

    string getString(string name);
    unsigned int getUint(string name);
    void getUintArray(string name, unsigned int** array, unsigned int* scount);
    int getInt(string name);
    double getDouble(string name);
    bool getBool(string name);
    bool contains(string name);

    void printTree(int level = 0);
    void printList(string list = "");

private:
    string mName;
    ExpressionList mNextWords;
    Expression* mPreviousWord;
    callback mFuncPtr;
    string mVarValue;
    string mMatchText;
};

#endif // __EXPRESSION_H__
