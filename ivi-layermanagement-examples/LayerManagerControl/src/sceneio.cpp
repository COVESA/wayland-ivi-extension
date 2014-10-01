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
#include "ilm_control.h"
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
    pSurface->set("drawCounter", props.drawCounter);
    pSurface->set("frameCounter", props.frameCounter);
    pSurface->set("inputDevicesAcceptance", props.inputDevicesAcceptance);
    pSurface->set("nativeSurface", props.nativeSurface);
    pSurface->set("opacity", props.opacity);
    pSurface->set("orientation", props.orientation);
    pSurface->set("origSourceHeight", props.origSourceHeight);
    pSurface->set("origSourceWidth", props.origSourceWidth);
    pSurface->set("pixelformat", props.pixelformat);
    pSurface->set("sourceHeight", props.sourceHeight);
    pSurface->set("sourceWidth", props.sourceWidth);
    pSurface->set("sourceX", props.sourceX);
    pSurface->set("sourceY", props.sourceY);
    pSurface->set("updateCounter", props.updateCounter);
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
    pLayer->set("orientation", props.orientation);
    pLayer->set("origSourceHeight", props.origSourceHeight);
    pLayer->set("origSourceWidth", props.origSourceWidth);
    pLayer->set("sourceHeight", props.sourceHeight);
    pLayer->set("sourceWidth", props.sourceWidth);
    pLayer->set("sourceX", props.sourceX);
    pLayer->set("sourceY", props.sourceY);
    pLayer->set("type", props.type);
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
            endIndex = in.find_last_of(']', startIndex) - 1;
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

ilmPixelFormat toPixelFormat(t_ilm_int format)
{
    switch (format)
    {
    case 0:
        return ILM_PIXELFORMAT_R_8;
    case 1:
        return ILM_PIXELFORMAT_RGB_888;
    case 2:
        return ILM_PIXELFORMAT_RGBA_8888;
    case 3:
        return ILM_PIXELFORMAT_RGB_565;
    case 4:
        return ILM_PIXELFORMAT_RGBA_5551;
    case 5:
        return ILM_PIXELFORMAT_RGBA_6661;
    case 6:
        return ILM_PIXELFORMAT_RGBA_4444;
    }

    return ILM_PIXEL_FORMAT_UNKNOWN;
}

ilmSurfaceProperties getSurfaceProperties(IlmSurface* pIlmsurface)
{
    ilmSurfaceProperties props;

    pIlmsurface->get("destHeight", &(props.destHeight));
    pIlmsurface->get("destWidth", &(props.destWidth));
    pIlmsurface->get("destX", &(props.destX));
    pIlmsurface->get("destY", &(props.destY));
    pIlmsurface->get("drawCounter", &(props.drawCounter));
    pIlmsurface->get("frameCounter", &(props.frameCounter));
    pIlmsurface->get("inputDevicesAcceptance", &(props.inputDevicesAcceptance));
    pIlmsurface->get("nativeSurface", &(props.nativeSurface));
    pIlmsurface->get("opacity", &(props.opacity));
    pIlmsurface->get("orientation", &(props.orientation));
    pIlmsurface->get("origSourceHeight", &(props.origSourceHeight));
    pIlmsurface->get("origSourceWidth", &(props.origSourceWidth));
    pIlmsurface->get("pixelformat", &(props.pixelformat));
    pIlmsurface->get("sourceHeight", &(props.sourceHeight));
    pIlmsurface->get("sourceWidth", &(props.sourceWidth));
    pIlmsurface->get("sourceX", &(props.sourceX));
    pIlmsurface->get("sourceY", &(props.sourceY));
    pIlmsurface->get("updateCounter", &(props.updateCounter));
    pIlmsurface->get("visibility", &(props.visibility));

    return props;
}

ilmLayerProperties getLayerProperties(IlmLayer* pIlmlayer)
{
    ilmLayerProperties props;
    pIlmlayer->get("destHeight", &(props.destHeight));
    pIlmlayer->get("destWidth", &(props.destWidth));
    pIlmlayer->get("destX", &(props.destX));
    pIlmlayer->get("destY", &(props.destY));
    pIlmlayer->get("opacity", &(props.opacity));
    pIlmlayer->get("orientation", &(props.orientation));
    pIlmlayer->get("origSourceHeight", &(props.origSourceHeight));
    pIlmlayer->get("origSourceHeight", &(props.origSourceHeight));
    pIlmlayer->get("origSourceWidth", &(props.origSourceWidth));
    pIlmlayer->get("sourceHeight", &(props.sourceHeight));
    pIlmlayer->get("sourceWidth", &(props.sourceWidth));
    pIlmlayer->get("sourceX", &(props.sourceX));
    pIlmlayer->get("sourceY", &(props.sourceY));
    pIlmlayer->get("type", &(props.type));
    pIlmlayer->get("visibility", &(props.visibility));

    return props;
}

void createSceneContentsHelper(IlmSurface* pIlmsurface)
{
    t_ilm_surface surfaceId;
    pIlmsurface->get("id", &surfaceId);
    ilmSurfaceProperties props = getSurfaceProperties(pIlmsurface);

    //if surface does not exist: create it
    t_ilm_int surfaceCount;
    t_ilm_surface* surfaceArray;

    ilmErrorTypes callResult = ilm_getSurfaceIDs(&surfaceCount, &surfaceArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available surface IDs\n";
    }

    if (find(surfaceArray, surfaceArray + surfaceCount, surfaceId)
            == surfaceArray + surfaceCount)
    {
        ilmPixelFormat pixelFormat;
        pixelFormat = toPixelFormat(props.pixelformat);

        ilmErrorTypes callResult = ilm_surfaceCreate(props.nativeSurface, props.origSourceWidth,
                                                        props.origSourceHeight, pixelFormat, &surfaceId);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to create surface\n";
        }
    }
}

void createSceneContentsHelper(IlmLayer* pIlmlayer)
{
    t_ilm_layer layerId;
    pIlmlayer->get("id", &layerId);

    ilmLayerProperties props = getLayerProperties(pIlmlayer);

    //create layer if does not exist
    t_ilm_int layerCount;
    t_ilm_layer* layerArray;

    ilmErrorTypes callResult = ilm_getLayerIDs(&layerCount, &layerArray);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to get available layer IDs\n";
    }

    if (find(layerArray, layerArray + layerCount, layerId) == layerArray + layerCount)
    {
        ilmErrorTypes callResult = ilm_layerCreateWithDimension(&layerId, props.origSourceWidth, props.origSourceHeight);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to create layer width dimensions (" << props.origSourceWidth << " ," << props.origSourceHeight << ")\n";
        }

        ilm_commitChanges();
    }

    list<IlmSurface*> surfaceList;
    pIlmlayer->get(&surfaceList);
    vector<t_ilm_surface> renderOrder;
    for (list<IlmSurface*>::iterator it = surfaceList.begin();
            it != surfaceList.end(); ++it)
    {
        t_ilm_surface surfaceId;
        (*it)->get("id", &surfaceId);
        renderOrder.push_back(surfaceId);

        createSceneContentsHelper(*it);
    }

    callResult = ilm_layerSetRenderOrder(layerId, renderOrder.data(), renderOrder.size());
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to set render order for layer with ID " << layerId << ")\n";
    }

    ilm_commitChanges();
}

void createSceneContentsHelper(IlmDisplay* pIlmdisplay)
{
    t_ilm_display displayId = 0xFFFFFFFF;
    pIlmdisplay->get("id", &displayId);

    list<IlmLayer*> layerList;
    pIlmdisplay->get(&layerList);
    vector<t_ilm_layer> renderOrder;
    for (list<IlmLayer*>::iterator it = layerList.begin(); it != layerList.end(); ++it)
    {
        t_ilm_layer layerId;
        (*it)->get("id", &layerId);
        renderOrder.push_back(layerId);

        createSceneContentsHelper(*it);
        ilm_commitChanges();
    }

    ilmErrorTypes callResult = ilm_displaySetRenderOrder(displayId, renderOrder.data(), renderOrder.size());
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to set render order for display with ID " << displayId << ")\n";
    }

    ilm_commitChanges();
}

void createSceneContents(IlmScene* pIlmscene)
{
    list<IlmSurface*> surfaceList;
    pIlmscene->get(&surfaceList);
    for (list<IlmSurface*>::iterator it = surfaceList.begin(); it != surfaceList.end(); ++it)
    {
        createSceneContentsHelper(*it);
    }

    list<IlmLayer*> layerList;
    pIlmscene->get(&layerList);
    for (list<IlmLayer*>::iterator it = layerList.begin();
            it != layerList.end(); ++it)
    {
        createSceneContentsHelper(*it);
    }

    list<IlmDisplay*> displayList;
    pIlmscene->get(&displayList);
    for (list<IlmDisplay*>::iterator it = displayList.begin(); it != displayList.end(); ++it)
    {
        createSceneContentsHelper(*it);
    }
}

void restoreSceneHelper(IlmSurface* pIlmsurface)
{
    t_ilm_surface surfaceId = 0xFFFFFFFF;
    pIlmsurface->get("id", &surfaceId);
    ilmSurfaceProperties props = getSurfaceProperties(pIlmsurface);

    ilmPixelFormat pixelFormat;
    pixelFormat = toPixelFormat(props.pixelformat);

    ilmErrorTypes callResult = ilm_surfaceSetNativeContent(props.nativeSurface, props.origSourceWidth,
                                                            props.origSourceHeight, pixelFormat, surfaceId);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to set native content for surface with ID " << surfaceId << ")\n";
    }

    ilm_commitChanges();

    ilm_surfaceSetOpacity(surfaceId, props.opacity);
    ilm_commitChanges();
    ilm_surfaceSetOrientation(surfaceId, props.orientation);
    ilm_commitChanges();
    ilm_surfaceSetSourceRectangle(surfaceId, props.sourceX, props.sourceY, props.sourceWidth, props.sourceHeight);
    ilm_commitChanges();
    ilm_surfaceSetDestinationRectangle(surfaceId, props.destX, props.destY, props.destWidth, props.destHeight);
    ilm_commitChanges();
    ilm_surfaceSetVisibility(surfaceId, props.visibility);
    ilm_commitChanges();
}

void restoreSceneHelper(IlmLayer* pIlmlayer)
{
    t_ilm_layer layerId = 0xFFFFFFFF;
    pIlmlayer->get("id", &layerId);

    ilmLayerProperties props = getLayerProperties(pIlmlayer);

    //set layer properties
    ilmErrorTypes callResult = ilm_layerSetDestinationRectangle(layerId, props.destX, props.destY, props.destWidth, props.destHeight);
    if (ILM_SUCCESS != callResult)
    {
        cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
        cout << "Failed to set destination rectangle for layer with ID " << layerId << ")\n";
    }

    ilm_commitChanges();
    ilm_layerSetOpacity(layerId, props.opacity);
    ilm_commitChanges();
    ilm_layerSetOrientation(layerId, props.orientation);
    ilm_commitChanges();
    ilm_layerSetSourceRectangle(layerId, props.sourceX, props.sourceY, props.sourceWidth, props.sourceHeight);
    ilm_commitChanges();
    ilm_layerSetVisibility(layerId, props.visibility);
    ilm_commitChanges();

    list<IlmSurface*> surfaceList;
    pIlmlayer->get(&surfaceList);

    //restore surfaces
    for (list<IlmSurface*>::iterator it = surfaceList.begin(); it != surfaceList.end(); ++it)
    {
        t_ilm_surface surfaceId;
        (*it)->get("id", &surfaceId);

        restoreSceneHelper(*it);
    }
}

void restoreSceneHelper(IlmDisplay* pIlmdisplay)
{
    t_ilm_display displayId;
    pIlmdisplay->get("id", &displayId);

    list<IlmLayer*> layerList;
    pIlmdisplay->get(&layerList);
    for (list<IlmLayer*>::iterator it = layerList.begin(); it != layerList.end(); ++it)
    {
        t_ilm_layer layerId;
        (*it)->get("id", &layerId);

        restoreSceneHelper(*it);
        ilm_commitChanges();
    }
}

void restoreScene(IlmScene* pIlmscene)
{
    t_scene_data currentScene;
    captureSceneData(&currentScene);

    //remove all surfaces from all layers
    for (map<t_ilm_surface, t_ilm_layer>::iterator it = currentScene.surfaceLayer.begin();
            it != currentScene.surfaceLayer.end(); ++it)
    {
        ilmErrorTypes callResult = ilm_layerRemoveSurface(it->second, it->first);
        if (ILM_SUCCESS != callResult)
        {
            cout << "LayerManagerService returned: " << ILM_ERROR_STRING(callResult) << "\n";
            cout << "Failed to remove surface " << it->first << " from layer " << it->second << ")\n";
        }
    }

    ilm_commitChanges();

    //create scene contents
    createSceneContents(pIlmscene);

    //restore scene
    list<IlmSurface*> surfaceList;
    pIlmscene->get(&surfaceList);
    for (list<IlmSurface*>::iterator it = surfaceList.begin(); it != surfaceList.end(); ++it)
    {
        restoreSceneHelper(*it);
    }

    list<IlmLayer*> layerList;
    pIlmscene->get(&layerList);
    for (list<IlmLayer*>::iterator it = layerList.begin(); it != layerList.end(); ++it)
    {
        restoreSceneHelper(*it);
    }

    list<IlmDisplay*> displayList;
    pIlmscene->get(&displayList);
    for (list<IlmDisplay*>::iterator it = displayList.begin(); it != displayList.end(); ++it)
    {
        restoreSceneHelper(*it);
    }
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

void importSceneFromFile(string filename)
{
    IlmScene ilmscene;
    IlmScene* pScene = &ilmscene;

    fstream stream(filename.c_str(), ios::in);

    StringMapTree sceneTree;

    //check extension
    if (filename.find_last_of(".") != string::npos)
    {
        string extension = filename.substr(filename.find_last_of("."));
        cout << extension << endl;

        if (extension == ".xml")
        {
//             importSceneFromXMLHelper(stream, &sceneTree);
            cout << "READING XML IS NOT SUPPORTED YET" << endl;
        }
        else if (extension == ".txt")
        {
            importSceneFromTXTHelper(stream, &sceneTree);
            cout << "DONE READING TXT" << endl;
        }
    }
    else
    {
        //defult behavior: assume txt
        importSceneFromTXTHelper(stream, &sceneTree);
        cout << "DONE READING TXT" << endl;
    }

    stream.close();

    cout << "Scene Tree :[" << sceneTree.toString() << "]" << endl;
    pScene->fromStringMapTree(&sceneTree);
    cout << "Scene successfully created from tree" << endl;

    restoreScene(pScene);
    cout << "Scene restored successfully" << endl;
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
