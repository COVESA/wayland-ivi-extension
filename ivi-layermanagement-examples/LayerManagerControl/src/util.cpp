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

#include "ilm_client.h"
#include "LMControl.h"

#include <cmath>
using std::min;
using std::max;
using std::pow;

t_ilm_bool inside(tuple4 A, tuple4 B)
{
    if (A.x >= B.x && A.y >= B.y && A.w <= B.w && A.z <= B.z)
        return ILM_TRUE;
    return ILM_FALSE;
}

t_ilm_bool between(int b1, int a, int b2)
{
    return a >= b1 && a <= b2;
}

t_ilm_bool intersect(tuple4 A, tuple4 B)
{
    if (between(B.x, A.x, B.z) || between(B.x, A.z, B.z))
    {
        if (between(A.y, B.y, A.w) || between(A.y, B.w, A.w))
            return ILM_TRUE;
    }

    if (between(B.y, A.y, B.w) || between(B.y, A.w, B.w))
    {
        if (between(A.x, B.x, A.z) || between(A.x, B.z, A.z))
            return ILM_TRUE;
    }
    return inside(A, B) || inside(B, A);
}

string rtrim(string s)
{
    s = s.substr(s.find_first_not_of(" \r\n\t\b"));
    return s;
}

string replaceAll(string s, string a, string b)
{
    std::size_t index = -b.size();

    while ((index = s.find(a, index + b.size())) != string::npos)
    {
        s.replace(index, a.size(), b);
    }

    return s;
}

string replaceAll(string s, map<string, string> replacements)
{
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        for (map<string, string>::iterator it = replacements.begin(); it != replacements.end(); ++it)
        {
            if (s.size() - i >= it->first.size() && s.substr(i, it->first.size()) == it->first)
            {
                s.replace(i, it->first.size(), it->second);
                i += it->second.size();
            }
        }
    }

    return s;
}

string replaceAll(string s, char a, char b)
{
    std::size_t index = -1;
    while ((index = s.find(a, index + 1)) != string::npos)
    {
        s.replace(index, 1, string(1, b));
    }

    return s;
}

set<string> split(string s, char d)
{
    set<string> result;
    std::size_t start = 0;

    while (start < s.size() && start != string::npos)
    {
        start = s.find_first_not_of(d, start);
        std::size_t end = s.find(d, start);
        result.insert(s.substr(start, end - start));

        start = end;
    }

    return result;
}
