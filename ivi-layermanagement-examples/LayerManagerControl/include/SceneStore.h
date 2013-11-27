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

#ifndef __SCENESTORE_H__
#define __SCENESTORE_H__


//=========================================================================
// helper macros fileformat
//=========================================================================
#include <StringMapTree.h>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

using std::string;
using std::stringstream;
using std::ostream;

#include <list>
using std::list;

#include <vector>
using std::vector;




template<typename T> string getPrimitiveType(T var)
{
    return "";
}

template<typename T> string getPrimitiveType(T* var)
{
    (void) var;//suppress warning: unsued variable
    T var2 = 0;
    return getPrimitiveType(var2) + "*";
}

template<> string getPrimitiveType(bool var)
{
    (void) var;//suppress warning: unsued variable
    return "bool";
}

template<> string getPrimitiveType(char var)
{
    (void) var;//suppress warning: unsued variable
    return "char";
}

template<> string getPrimitiveType(signed char var)
{
    (void) var;//suppress warning: unsued variable
    return "signed char";
}

template<> string getPrimitiveType(unsigned char var)
{
    (void) var;//suppress warning: unsued variable
    return "unsigned char";
}

template<> string getPrimitiveType(wchar_t var)
{
    (void) var;//suppress warning: unsued variable
    return "wchar_t";
}

template<> string getPrimitiveType(short int var)
{
    (void) var;//suppress warning: unsued variable
    return "short int";
}

template<> string getPrimitiveType(unsigned short int var)
{
    (void) var;//suppress warning: unsued variable
    return "unsigned short int";
}

template<> string getPrimitiveType(long int var)
{
    (void) var;//suppress warning: unsued variable
    return "long int";
}

template<> string getPrimitiveType(unsigned long int var)
{
    (void) var;//suppress warning: unsued variable
    return "unsigned long int";
}

template<> string getPrimitiveType(int var)
{
    (void) var;//suppress warning: unsued variable
    return "int";
}

template<> string getPrimitiveType(unsigned int var)
{
    (void) var;//suppress warning: unsued variable
    return "unsigned int";
}

template<> string getPrimitiveType(float var)
{
    (void) var;//suppress warning: unsued variable
    return "float";
}

template<> string getPrimitiveType(double var)
{
    (void) var;//suppress warning: unsued variable
    return "double";
}

template<> string getPrimitiveType(long double var)
{
    (void) var;//suppress warning: unsued variable
    return "long double";
}

template<> string getPrimitiveType(string var)
{
    (void) var;//suppress warning: unsued variable
    return "string";
}


#if defined(__GXX_EXPERIMENTAL_CXX0X) || __cplusplus >= 201103L
template<> string getPrimitiveType(char16_t var)
{
    (void) var;//suppress warning: unsued variable
    return "char16_t";
}

template<> string getPrimitiveType(char32_t var)
{
    (void) var;//suppress warning: unsued variable
    return "char32_t";
}

template<> string getPrimitiveType(long long int var)
{
    (void) var;//suppress warning: unsued variable
    return "long long int";
}

template<> string getPrimitiveType(unsigned long long int var)
{
    (void) var;//suppress warning: unsued variable
    return "unsigned long long int";
}
#endif


struct WrapperHelper
{
public:
    const string mType;
    WrapperHelper(string t) :
            mType(t)
    {
    }

    virtual ~WrapperHelper()
    {
    }

    virtual void fromString(string s)
    {
        (void) s;//suppress warning: unsued variable
    }

    virtual string asString()
    {
        return "";
    }

    virtual void toStringMapTree(StringMapTree* parent)
    {
        (void) parent;//suppress warning: unsued variable
    }

    virtual void toGrammarMapTree(StringMapTree* tree)
    {
        (void) tree;//suppress warning: unsued variable
    }

    virtual WrapperHelper* tryClone(string type, StringMapTree* tree)
    {
        (void) type;//suppress warning: unsued variable
        (void) tree;//suppress warning: unsued variable
        return NULL;
    }

    virtual void addToComplexWrapper(WrapperHelper* wrapper)
    {
        (void) wrapper;//suppress warning: unsued variable
    }

    virtual string getWrapperPrimitiveType()
    {
        return "";
    }
};

template<typename T>
struct ComplexWrapper : public WrapperHelper
{
public:
    list<T> components;

    ComplexWrapper(string t) :
            WrapperHelper(t)
    {
    }

    virtual void toStringMapTree(StringMapTree* parent)
    {
        for (typename list<T>::iterator it = components.begin(); it != components.end(); ++it)
        {
            StringMapTree* node = new StringMapTree;
            (*it)->toStringMapTree(node);
            parent->mChildren.push_back(node);
        }
    }
};

template<typename T>
struct BasicWrapper : public WrapperHelper
{
public:
    T value;

    BasicWrapper(string t) :
            WrapperHelper(t)
    {
    }

    virtual void fromString(string s)
    {
        stringstream ss;
        ss.str(s);
        ss >> std::skipws >> value;
    }

    virtual string asString()
    {
        stringstream ss;
        ss << value;
        return    ss.str();
    }

    virtual string getWrapperPrimitiveType()
    {
        return getPrimitiveType(value);
    }
};

template<>
class BasicWrapper<string>: public WrapperHelper
{
public:
    string value;

    BasicWrapper(string t) :
            WrapperHelper(t)
    {
    }

    virtual void fromString(string s)
    {
        value = s;
    }

    virtual string asString()
    {
        return    value;
    }

    virtual string getWrapperPrimitiveType()
    {
        return getPrimitiveType(value);
    }
};

template<>
class BasicWrapper<char*>: public WrapperHelper
{
public:
    const char* value;

    BasicWrapper(char* t) :
            WrapperHelper(t)
    {
    }

    virtual void fromString(string s)
    {
        value = s.c_str();
    }

    virtual string asString()
    {
        return    value;
    }

    virtual string getWrapperPrimitiveType()
    {
        return getPrimitiveType(value);
    }
};

template<typename T>
struct DummyWrapper : public WrapperHelper
{
public:
    T value;

    DummyWrapper(string t) :
            WrapperHelper(t)
    {
    }

    virtual WrapperHelper* tryClone(string type, StringMapTree* tree)
    {
        if (type == mType)
        {
            DummyWrapper<T>* newWrapper = new DummyWrapper<T>(type);
            newWrapper->value.fromStringMapTree(tree);
            return newWrapper;
        }

        return NULL;
    }

    virtual void toGrammarMapTree(StringMapTree* tree)
    {
        value.toGrammarMapTree(tree);
    }

    virtual void addToComplexWrapper(WrapperHelper* wrapper)
    {
        ComplexWrapper<T*>* complexWrapper = static_cast<ComplexWrapper<T*>* >(wrapper);
        complexWrapper->components.push_back(&value);
    }
};

map<int, string> _globalTypeIndexdToType;

#define OBJECT(class_name) \
class class_name;\
ostream& operator<<(ostream& out, class_name& obj )\
{\
    (void) obj;\
    return out;\
}\
istream& operator>>(istream& in, class_name& obj)\
{\
    (void) obj;\
    return in;\
}\
class class_name { \
public:\
    map<string, WrapperHelper*> properties;\
    map<string, string> typesMap;\
    map<string, WrapperHelper*> components;\
    map<string, WrapperHelper*> dummyComponentClones;\
public:\
    const static int classNameIndex = __COUNTER__;\
    static int getClassNameIndex()\
    {\
        return classNameIndex;\
    }\
    \
    string mClassName;\
    template<typename T> bool get(string key, T* p)\
    {\
        if(properties.find(key) != properties.end())\
        {\
            BasicWrapper<T>* obj = (static_cast<BasicWrapper<T>*> (properties[key]));\
            if(obj) *p = obj->value;\
            return obj != NULL;\
        }\
        return false;\
    }\
    template<typename T> bool get(int* count, T*** p)\
    {\
        string type = _globalTypeIndexdToType[T::getClassNameIndex()];\
        if(components.find(type) != components.end())\
        {\
            ComplexWrapper<T*>* obj = static_cast<ComplexWrapper<T*>* >(components[type]);\
            if(obj){\
                vector<T*>* temp = new vector<T*>(obj->components.begin(), obj->components.end());\
                *p = temp->data();\
                *count = obj->components.size();\
            }\
            return obj != NULL;\
        }\
        return false;\
    }\
    template<typename T> bool get(list<T*>* p)\
    {\
        string type = _globalTypeIndexdToType[T::getClassNameIndex()];\
        if(components.find(type) != components.end())\
        {\
            ComplexWrapper<T*>* obj = static_cast<ComplexWrapper<T*>* >(components[type]);\
            if(obj){\
                *p = obj->components;\
            }\
            return obj != NULL;\
        }\
        return false;\
    }\
    template<typename T> bool set(string key, const T& p)\
    {\
        BasicWrapper<T>* obj = (static_cast<BasicWrapper<T>*> (properties[key]));\
        if(obj) obj->value = p;\
        return obj != NULL;\
    }\
    template<typename T> bool set(string key, T* p)\
    {\
        BasicWrapper<T*>* obj = (static_cast<BasicWrapper<T*>*> (properties[key]));\
        if(obj) obj->value = p;\
        return obj != NULL;\
    }\
    string getType(string key)\
    {\
        return typesMap[key];\
    }\
    template<typename T> bool add(T* obj)\
    {\
        if(components.find(obj->mClassName) != components.end())\
        {\
            (static_cast<ComplexWrapper<T*>* > (components[obj->mClassName])->components).push_back(obj);\
            return true;\
        }\
        return false;\
    }\
    template<typename T> bool unwrapAndAdd(DummyWrapper<T>* obj)\
    {\
        if(components.find(obj->mClassName) != components.end())\
        {\
            (static_cast<ComplexWrapper<T*>* > (components[obj->mClassName])->components).push_back(obj);\
            return true;\
        }\
        return false;\
    }\
    template<typename T> bool remove(T* obj)\
    {\
        if(components.find(obj->mClassName) != components.end())\
        {\
            (static_cast<ComplexWrapper<T*>* > (components[obj->mClassName])->components).remove(obj);\
            return true;\
        }\
        return false;\
    }\
    ~class_name()\
    {\
        for(map<string, WrapperHelper*>::iterator it = properties.begin(); it != properties.end();++it)\
            delete (*it).second;\
        for(map<string, WrapperHelper*>::iterator it = components.begin(); it != components.end();++it)\
            delete (*it).second;\
    }\
    class_name():mClassName(#class_name)\
    {\
        _globalTypeIndexdToType[getClassNameIndex()] = #class_name;

#define PROPERTY(type, name) \
    properties[#name] = new BasicWrapper<type>(#type);\
    typesMap[#name] = #type;


#define CONTAINS(type) \
        components[#type] = new ComplexWrapper<type*>(#type);\
        dummyComponentClones[#type]= (new DummyWrapper<type>(#type));


#define OBJECTEND \
    }\
    void toGrammarMapTree(StringMapTree* node)\
    {\
        node->mNodeLabel = mClassName;\
        for(map<string, WrapperHelper*>::iterator it = properties.begin(); it != properties.end();++it)\
        {\
            node->mNodeValues[(*it).first] = make_pair((*it).second->mType, it->second->getWrapperPrimitiveType());\
        }\
        for(map<string, WrapperHelper*>::iterator it = dummyComponentClones.begin(); it != dummyComponentClones.end();++it)\
        {\
            StringMapTree* child = new StringMapTree;\
            node->mChildren.push_back(child);\
            it->second->toGrammarMapTree(child);\
        }\
    }\
    void toStringMapTree(StringMapTree* node)\
    {\
        node->mNodeLabel = mClassName;\
        for(map<string, WrapperHelper*>::iterator it = properties.begin(); it != properties.end();++it)\
        {\
            node->mNodeValues[(*it).first] = make_pair((*it).second->mType, (*it).second->asString());\
        }\
        for(map<string, WrapperHelper*>::iterator it = components.begin(); it != components.end();++it)\
        {\
            it->second->toStringMapTree(node);\
        }\
    }\
    void fromStringMapTree(StringMapTree* node)\
    {\
        mClassName = node->mNodeLabel;\
        for(map<string,pair<string,string> >::iterator it = node->mNodeValues.begin(); it != node->mNodeValues.end();++it)\
        {\
            properties[it->first]->fromString(it->second.second);\
        }\
        for(list<StringMapTree*>::iterator it = node->mChildren.begin(); it != node->mChildren.end(); ++it )\
        {\
            string type = (*it)->mNodeLabel;\
            WrapperHelper* wrapper = dummyComponentClones[type]->tryClone(type, (*it));\
            WrapperHelper* complexWrapper = components[type];\
            wrapper->addToComplexWrapper(complexWrapper);\
        }\
    }\
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**************************************************************************************************************************//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**************************************************************************************************************************//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OBJECT(IlmSurface)
    PROPERTY(t_ilm_surface, id)
    PROPERTY(t_ilm_float, opacity)
    PROPERTY(t_ilm_uint, sourceX)
    PROPERTY(t_ilm_uint, sourceY)
    PROPERTY(t_ilm_uint, sourceWidth)
    PROPERTY(t_ilm_uint, sourceHeight)
    PROPERTY(t_ilm_uint, origSourceWidth)
    PROPERTY(t_ilm_uint, origSourceHeight)
    PROPERTY(t_ilm_uint, destX)
    PROPERTY(t_ilm_uint, destY)
    PROPERTY(t_ilm_uint, destWidth)
    PROPERTY(t_ilm_uint, destHeight)
    PROPERTY(int, orientation)
    PROPERTY(t_ilm_bool, visibility)
    PROPERTY(t_ilm_uint, frameCounter)
    PROPERTY(t_ilm_uint, drawCounter)
    PROPERTY(t_ilm_uint, updateCounter)
    PROPERTY(t_ilm_uint, pixelformat)
    PROPERTY(t_ilm_uint, nativeSurface)
    PROPERTY(ilmInputDevice, inputDevicesAcceptance)
    /***/
OBJECTEND

OBJECT(IlmLayer)
    PROPERTY(t_ilm_layer, id)
    PROPERTY(t_ilm_float, opacity)
    PROPERTY(t_ilm_uint, sourceX)
    PROPERTY(t_ilm_uint, sourceY)
    PROPERTY(t_ilm_uint, sourceWidth)
    PROPERTY(t_ilm_uint, sourceHeight)
    PROPERTY(t_ilm_uint, origSourceWidth)
    PROPERTY(t_ilm_uint, origSourceHeight)
    PROPERTY(t_ilm_uint, destX)
    PROPERTY(t_ilm_uint, destY)
    PROPERTY(t_ilm_uint, destWidth)
    PROPERTY(t_ilm_uint, destHeight)
    PROPERTY(int, orientation)
    PROPERTY(t_ilm_bool, visibility)
    PROPERTY(t_ilm_uint, type)
    /***/
    CONTAINS(IlmSurface)
OBJECTEND

OBJECT(IlmDisplay)
    PROPERTY(t_ilm_display, id)
    PROPERTY(t_ilm_uint, width)
    PROPERTY(t_ilm_uint, height)
    /***/
    CONTAINS(IlmLayer)
OBJECTEND

OBJECT(IlmScene)
    /***/
    CONTAINS(IlmSurface)
    CONTAINS(IlmLayer)
    CONTAINS(IlmDisplay)
OBJECTEND

#endif /* __SCENESTORE_H__ */
