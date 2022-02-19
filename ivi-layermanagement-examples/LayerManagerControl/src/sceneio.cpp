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
#include "LMControl.h"
#include "Expression.h"
#include "ExpressionInterpreter.h"
#include "SceneStore.h"
#include <cstdio>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <pthread.h>
#include <stdarg.h>

using namespace std;


namespace {
void captureSceneDataHelper(t_ilm_surface surfaceId, t_scene_data* pSceneData, IlmSurface* pSurface)
{
    ilmSurfaceProperties& props = pSceneData->surfaceProperties[surfaceId];

    pSurface->set("id", surfaceId);
    pSurface->set("destHeight", props.destHeight);
    pSurface->set("destWidth", props.destWidth);
    pSurface->set("destX", props.destX);
    pSurface->set("destY", props.destY);
    pSurface->set("frameCounter", props.frameCounter);
    pSurface->set("opacity", props.opacity);
    pSurface->set("origSourceHeight", props.origSourceHeight);
    pSurface->set("origSourceWidth", props.origSourceWidth);
    pSurface->set("sourceHeight", props.sourceHeight);
    pSurface->set("sourceWidth", props.sourceWidth);
    pSurface->set("sourceX", props.sourceX);
    pSurface->set("sourceY", props.sourceY);
    pSurface->set("visibility", props.visibility);
}

void captureSceneDataHelper(t_ilm_layer layerId, t_scene_data* pSceneData, IlmLayer* pLayer)
{
    ilmLayerProperties& props = pSceneData->layerProperties[layerId];
    pLayer->set("id", layerId);
    pLayer->set("destHeight", props.destHeight);
    pLayer->set("destWidth", props.destWidth);
    pLayer->set("destX", props.destX);
    pLayer->set("destY", props.destY);
    pLayer->set("opacity", props.opacity);
    pLayer->set("sourceHeight", props.sourceHeight);
    pLayer->set("sourceWidth", props.sourceWidth);
    pLayer->set("sourceX", props.sourceX);
    pLayer->set("sourceY", props.sourceY);
    pLayer->set("visibility", props.visibility);

    if (pSceneData->layerSurfaces.find(layerId) != pSceneData->layerSurfaces.end())
    {
        for (vector<t_ilm_surface>::iterator it = pSceneData->layerSurfaces[layerId].begin();
                it != pSceneData->layerSurfaces[layerId].end(); ++it)
        {
            IlmSurface* pIlmsurface = new IlmSurface;
            captureSceneDataHelper(*it, pSceneData, pIlmsurface);
            pLayer->add(pIlmsurface);
        }
    }
}

void captureSceneData(IlmScene* scene)
{
    t_scene_data sceneStruct;
    captureSceneData(&sceneStruct);

    for (vector<t_ilm_display>::iterator it = sceneStruct.screens.begin();
            it != sceneStruct.screens.end(); ++it)
    {
        t_ilm_display displayId = *it;
        IlmDisplay* pIlmdisplay = new IlmDisplay;
        pIlmdisplay->set("id", displayId);
        pIlmdisplay->set("width", sceneStruct.screenWidth);
        pIlmdisplay->set("height", sceneStruct.screenHeight);

        if (sceneStruct.screenLayers.find(displayId) != sceneStruct.screenLayers.end())
        {
            for (vector<t_ilm_layer>::iterator it = sceneStruct.screenLayers[displayId].begin();
                    it != sceneStruct.screenLayers[displayId].end(); ++it)
            {
                IlmLayer* pIlmlayer = new IlmLayer;
                captureSceneDataHelper(*it, &sceneStruct, pIlmlayer);

                pIlmdisplay->add(pIlmlayer);
            }
        }

        scene->add(pIlmdisplay);
    }

    for (vector<t_ilm_layer>::iterator it = sceneStruct.layers.begin();
            it != sceneStruct.layers.end(); ++it)
    {
        if (sceneStruct.layerScreen.find(*it) == sceneStruct.layerScreen.end())
        {
            IlmLayer* pIlmlayer = new IlmLayer;
            captureSceneDataHelper(*it, &sceneStruct, pIlmlayer);

            scene->add(pIlmlayer);
        }
    }

    for (vector<t_ilm_surface>::iterator it = sceneStruct.surfaces.begin();
            it != sceneStruct.surfaces.end(); ++it)
    {
        if (sceneStruct.surfaceLayer.find(*it) == sceneStruct.surfaceLayer.end())
        {
            IlmSurface* pIlmsurface = new IlmSurface;
            captureSceneDataHelper(*it, &sceneStruct, pIlmsurface);

            scene->add(pIlmsurface);
        }
    }
}

string encodeEscapesequences(string s)
{
    map<string, string> code;

    code["\\"] = "\\[\\]";
    code["\n"] = "\\[n]";
    code["\t"] = "\\[t]";
    code["\v"] = "\\[v]";
    code["\b"] = "\\[b]";
    code["\f"] = "\\[f]";
    code["\r"] = "\\[r]";

    return replaceAll(s, code);
}

void exportSceneToTXTHelper(ostream& stream, StringMapTree* tree, string prefix = "")
{
    stream << prefix << encodeEscapesequences(tree->mNodeLabel) + ":{\n";
    for (map<string, pair<string, string> >::iterator it = tree->mNodeValues.begin();
            it != tree->mNodeValues.end(); ++it)
    {
        stream << prefix + "\t[" + encodeEscapesequences(it->first) + ":"
            + encodeEscapesequences(it->second.first) + "]=["
            + encodeEscapesequences(it->second.second) + "]\n";
    }

    for (list<StringMapTree*>::iterator it = tree->mChildren.begin(); it != tree->mChildren.end(); ++it)
    {
        exportSceneToTXTHelper(stream, *it, prefix + "\t");
        stream << "\n";
    }

    stream << prefix + "}";
}

string decodeEscapesequences(string s)
{
    map<string, string> code;
    code["\\[\\]"] = "\\";
    code["\\[n]"] = "\n";
    code["\\[t]"] = "\t";
    code["\\[v]"] = "\v";
    code["\\[b]"] = "\b";
    code["\\[f]"] = "\f";
    code["\\[r]"] = "\r";
    return replaceAll(s, code);
}

void importSceneFromTXTHelper(istream& stream, StringMapTree* node)
{
    string in;
    //Type
    getline(stream, in);
    int typeSize = in.find(":") - in.find_first_not_of('\t');
    int typeStart = in.find_first_not_of('\t');
    node->mNodeLabel = in.substr(typeStart, typeSize);
    while (true)
    {
        long streamPosition = stream.tellg();
        getline(stream, in);
        in = rtrim(in);

        //end of object
        if (in.substr(0, 1) == "}")
            return;

        //start of object property
        if (in.substr(0, 1) == "[")
        {
            int startIndex = in.find('[') + 1;
            int endIndex = in.find(":");
            string propertyName = in.substr(startIndex, endIndex - startIndex);
            propertyName = decodeEscapesequences(propertyName);

            startIndex = endIndex + 1;
            endIndex = in.find("]");
            string propertyType = in.substr(startIndex, endIndex - startIndex);
            propertyType = decodeEscapesequences(propertyType);

            startIndex = in.find('[', endIndex) + 1;
            endIndex = in.find_last_of(']');
            string propertyValue = in.substr(startIndex, endIndex - startIndex);
            propertyValue = decodeEscapesequences(propertyValue);

            node->mNodeValues[propertyName] = make_pair(propertyType, propertyValue);
        }
        else
        {
            stream.seekg(streamPosition);
            StringMapTree* child = new StringMapTree;
            node->mChildren.push_back(child);
            importSceneFromTXTHelper(stream, child);
        }
    }
}

string makeValidXMLCharacters(string s)
{
    map<string, string> code;
    code["<"] = "&lt;";
    code[">"] = "&gt;";
    code["&"] = "&amp;";
    code["\'"] = "&apos;";
    code["\""] = "&quot;";
    return replaceAll(s, code);
}

void exportSceneToXMLHelper(ostream& stream, StringMapTree* tree, string prefix = "")
{
    stream << prefix << "<" << tree->mNodeLabel << ">\n";

    for (map<string, pair<string, string> >::iterator it = tree->mNodeValues.begin();
            it != tree->mNodeValues.end(); ++it)
    {
        stream << prefix + "\t<Property name=\"" << it->first << "\" type=\"" << it->second.first << "\">\n";
        stream << prefix << "\t\t" << makeValidXMLCharacters(it->second.second) + "\n";
        stream << prefix << "\t</Property>\n";
    }

    for (list<StringMapTree*>::iterator it = tree->mChildren.begin();
            it != tree->mChildren.end(); ++it)
    {
        exportSceneToXMLHelper(stream, *it, prefix + "\t");
        stream << "\n";
    }

    stream << prefix << "</" << tree->mNodeLabel << ">\n";
}
} //end of anonymous namespace

void exportSceneToFile(string filename)
{
    IlmScene ilmscene;
    IlmScene* pScene = &ilmscene;
    captureSceneData(&ilmscene);
    stringstream buffer;
    StringMapTree sceneTree;
    pScene->toStringMapTree(&sceneTree);

    //check extension
    if (filename.find_last_of(".") != string::npos)
    {
        string extension = filename.substr(filename.find_last_of("."));
        cout << extension << endl;

        if (extension == ".xml")
        {
            buffer << "<\?xml version=\"1.0\"\?>\n";
            exportSceneToXMLHelper(buffer, &sceneTree);
            cout << "DONE WRITING XML" << endl;
        }
        else if (extension == ".txt")
        {
            exportSceneToTXTHelper(buffer, &sceneTree);
            cout << "DONE WRITING TXT" << endl;
        }
    }
    else
    {
        //defult:
        exportSceneToTXTHelper(buffer, &sceneTree);
        cout << "DONE WRITING TXT" << endl;
    }

    fstream stream(filename.c_str(), ios::out);

    cout << buffer.str() << endl;
    stream << buffer.str();
    stream.flush();
    stream.close();
}

void exportXtext(string fileName, string grammar, string url)
{
    string name = grammar.substr(grammar.find_last_of('.') + 1);
    //make sure first character is lower case
    std::transform(name.begin(), ++(name.begin()), name.begin(), ::tolower);

    IlmScene scene;
    StringMapTree grammarTree;
    scene.toGrammarMapTree(&grammarTree);

    //writing to file
    stringstream buffer;
    buffer << "grammar " << grammar << " with org.eclipse.xtext.common.Terminals" << endl;
    buffer << "generate " << name << " \"" << url << "\"" << endl;

    list<string> doneTypes;
    list<StringMapTree*> waitingNodes;
    waitingNodes.push_back(&grammarTree);
    while (!waitingNodes.empty())
    {
        //pop first element of the waiting types
        StringMapTree* typeNode = *(waitingNodes.begin());
        waitingNodes.pop_front();
        string type = typeNode->mNodeLabel;

        //if the type was not printed before
        if (find(doneTypes.begin(), doneTypes.end(), type) == doneTypes.end())
        {
            doneTypes.push_back(type);

            buffer << type << ":" << endl;
            buffer << "\t\'" << type << ":\'" << endl;

            for (map<string, pair<string, string> >::iterator it = typeNode->mNodeValues.begin();
                    it != typeNode->mNodeValues.end(); ++it)
            {
                buffer << "\t\t\'" << it->first << ":\' " << it->first << "=" <<
                        (it->second.second.size() > 0 ? it->second.second : it->second.first) << endl;
            }

            for (list<StringMapTree*>::iterator it = typeNode->mChildren.begin();
                    it != typeNode->mChildren.end(); ++it)
            {
                waitingNodes.push_back(*it);
                string childName = (*it)->mNodeLabel;
                //make lower case
                std::transform(childName.begin(), childName.end(), childName.begin(), ::tolower);
                childName += "s";

                buffer << "\t\t" << childName << "+=" << (*it)->mNodeLabel << "*" << endl;
            }

            buffer << ";" << endl;
        }
    }

    cout << "Xtext:[" << buffer.str() << "]" << endl;

    fstream fileout(fileName.c_str(), ios::out);
    fileout << buffer.str();
    fileout.flush();
    fileout.close();
}
