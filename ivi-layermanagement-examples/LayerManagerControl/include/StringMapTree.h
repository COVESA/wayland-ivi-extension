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


#ifndef __STRINGMAPTREE_H__
#define __STRINGMAPTREE_H__

#include <map>
#include <list>
#include <string>

using std::map;
using std::list;
using std::string;

class StringMapTree
{
public:
    string mNodeLabel;
    map<string, pair<string, string> > mNodeValues;//key: property name, value= pair(type, property value)
    list<StringMapTree*> mChildren;// pair(type, child component node)

    string toString(string prefix = "")
    {
        string result;
        result += prefix + mNodeLabel + ":{\n";
        for(map<string, pair<string, string> >::iterator it = mNodeValues.begin(); it != mNodeValues.end(); ++it)
        {
            result += prefix + "\t[" + it->first + ":" + it->second.first + "]=[" + it->second.second + "]\n";
        }
        for(list<StringMapTree*>::iterator it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            result += (*it)->toString(prefix + "\t") +"\n";
        }
        result += prefix + "}";

        return result;
    }

    virtual ~StringMapTree()
    {
        for(list<StringMapTree*>::iterator it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            delete *it;
        }
    }
};

#endif /* __STRINGMAPTREE_H__ */
