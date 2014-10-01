/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
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
#ifndef _MOCKNAVI_H
#define _MOCKNAVI_H

#include "OpenGLES2App.h"
#include "IRenderable.h"
#include "IUpdateable.h"
#include "Camera.h"

#include <list>
using std::list;

class MockNaviHouse;

class MockNavi : public OpenGLES2App
{
public:
    MockNavi(float fps, float animationSpeed, SurfaceConfiguration* config);

    virtual void update(int currentTimeInMs, int lastFrameTime);
    virtual void render();

private:
    void generateCity();

private:
    Camera m_camera;
    int m_houseCount;
    list<IRenderable*> m_renderList;
    list<IUpdateable*> m_updateList;
};

#endif /* _MOCKNAVI_H */
