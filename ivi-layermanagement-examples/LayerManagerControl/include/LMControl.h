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

#ifndef __LMCONTROL_H__
#define __LMCONTROL_H__

#include <map>
using std::map;

#include <vector>
using std::vector;

#include <set>
using std::set;

#include <string>
using std::string;

#include "ilm_common.h"

/*
 * Datastructure that contains all information about a scene
 */
struct t_scene_data
{
    map<t_ilm_display, vector<t_ilm_layer> > screenLayers;
    map<t_ilm_layer, vector<t_ilm_surface> > layerSurfaces;

    map<t_ilm_surface, ilmSurfaceProperties> surfaceProperties;
    map<t_ilm_layer, ilmLayerProperties> layerProperties;

    map<t_ilm_layer, t_ilm_display> layerScreen;
    map<t_ilm_surface, t_ilm_layer> surfaceLayer;

    vector<t_ilm_surface> surfaces;
    vector<t_ilm_surface> layers;
    vector<t_ilm_display> screens;

    t_ilm_layer extraLayer;
    t_ilm_uint screenWidth;
    t_ilm_uint screenHeight;
};

/*
 * Vector of four integers <x y z w>
 */
struct tuple4
{
public:
    int x;
    int y;
    int z;
    int w;

    tuple4(int _x, int _y, int _z, int _w) :
        x(_x), y(_y), z(_z), w(_w)
    {
    }

    tuple4() :
        x(0), y(0), z(0), w(0)
    {
    }

    tuple4(const tuple4& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }

    void scale(float s)
    {
        x = static_cast<int>(x * s);
        y = static_cast<int>(y * s);
        z = static_cast<int>(z * s);
        w = static_cast<int>(w * s);
    }

    const tuple4& operator=(const tuple4& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
};



//=============================================================================
//common.cpp
//=============================================================================
/*
 * Captures all information about the rendered scene into an object of type t_scene_data
 */
void captureSceneData(t_scene_data* pScene);

/*
 * Calculates the final coordinates of a surface on the screen in the scene
 */
tuple4 getSurfaceScreenCoordinates(t_scene_data* pScene, t_ilm_surface surface);

/*
 * Gets render order of surfaces in a scene. A surface is execluded from the render order
 * if it does not belong to a layer or if it belongs to a layer that does not belong to a screen
 *
 * The surface at index 0 is the deepest surface in the scene, and surface at size()-1 is
 * the topmost surface
 */
vector<t_ilm_surface> getSceneRenderOrder(t_scene_data* pScene);

//=============================================================================
//util.cpp
//=============================================================================

/*
 * Returns true if the rectangle A is inside (or typical with) rectangle B
 * (each tuple represents a rectangle in 2-D coordinates as <x1 y1 x2 y2>)
 */
t_ilm_bool inside(tuple4 A, tuple4 B);

/*
 * Returns true if a is in the interval between b1 and b2 (inclusive)
 */
t_ilm_bool between(int b1, int a, int b2);


/*
 * Returns true if the rectangle A intersects (or touches) rectangle B
 * (each tuple represents a rectangle in 2-D coordinates as <x1 y1 x2 y2>)
 */
t_ilm_bool intersect(tuple4 A, tuple4 B);

/*
 * Trim white space characters from the beginning of the string
 */
string rtrim(string s);

/*
 * Replace all occurances of a in s by b
 */
string replaceAll(string s, string a, string b);
string replaceAll(string s, char a, char b);

/*
 * For every pair (a,b) in the map: replace all occurances of a in the string by b
 */
string replaceAll(string s, map<string, string> replacements);

/*
 * Split the string using the giving delimiter
 */
set<string> split(string s, char d);


//=============================================================================
//print.cpp
//=============================================================================

/*
 * Functions for printing arrays, vector and maps
 */
void printArray(const char* text, unsigned int* array, int count);

template<typename T>
void printArray(const char* text, T* array, int count);

template<typename T>
void printVector(const char* text, vector<T> v);

template<typename K, typename V>
void printMap(const char* text, std::map<K, V> m);

/*
 * Prints information about the specified screen
 */
void printScreenProperties(unsigned int screenid, const char* prefix = "");

/*
 * Prints information about the specified layer
 */
void printLayerProperties(unsigned int layerid, const char* prefix = "");

/*
 * Prints information about the specified surface
 */
void printSurfaceProperties(unsigned int surfaceid, const char* prefix = "");

/*
 * Prints information about rendered scene
 * (All screens, all rendered layers, all rendered surfaces)
 */
void printScene();


//=============================================================================
//control.cpp
//=============================================================================
void testNotificationLayer(t_ilm_layer layerid);
void watchLayer(unsigned int* layerids, unsigned int layeridCount);
void watchSurface(unsigned int* surfaceids, unsigned int surfaceidCount);


//=============================================================================
//analyze.cpp
//=============================================================================

/*
 * Runs and prints the results for a set of analysis procedures to check if there are any potential problems that
 * might lead to the surface being not rendered or not visible
 */
t_ilm_bool analyzeSurface(t_ilm_surface targetSurfaceId);

/*
 * Returns a scattered version of the scene
 */
t_scene_data getScatteredScene(t_scene_data* pInitialScene);

//=============================================================================
//sceneio.cpp
//=============================================================================

/*
 * Saves a representation of the current rendered scene to a file
 */
void exportSceneToFile(string filename);

/*
 * Saves an xtext representation of the grammar of the scene
 */
void exportXtext(string fileName, string grammar, string url);

#endif
