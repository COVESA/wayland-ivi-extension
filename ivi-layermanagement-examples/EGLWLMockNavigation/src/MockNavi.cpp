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
#include "MockNavi.h"

#include "House.h"
#include "Street.h"
#include "Ground.h"
#include "Car.h"
#include "ShaderLighting.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define CITY_GRID_SIZE 1.0f

MockNavi::MockNavi(float fps, float animationSpeed, SurfaceConfiguration* config)
: OpenGLES2App(fps, animationSpeed, config)
, m_camera(vec3f(-1.5 * CITY_GRID_SIZE, -0.1, 0.0), vec3f(0.0, 0.0, 0.0), config->surfaceWidth, config->surfaceHeight)
, m_houseCount(15)
{
    generateCity();
}

void MockNavi::update(int currentTimeInMs, int lastFrameTime)
{
	m_camera.update(currentTimeInMs, lastFrameTime);

	list<IUpdateable*>::const_iterator iter = m_updateList.begin();
    list<IUpdateable*>::const_iterator iterEnd = m_updateList.end();

    for (; iter != iterEnd; ++iter)
    {
        (*iter)->update(currentTimeInMs, lastFrameTime);
    }
}

void MockNavi::render()
{
    list<IRenderable*>::const_iterator iter = m_renderList.begin();
    list<IRenderable*>::const_iterator iterEnd = m_renderList.end();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (; iter != iterEnd; ++iter)
    {
        (*iter)->render();
    }
}

void MockNavi::generateCity()
{
	IlmMatrix* projection = m_camera.getViewProjectionMatrix();
    ShaderLighting* pShader = new ShaderLighting(projection);

    // generate base plate
	vec4f groundColor(0.8, 0.8, 0.6, 1.0);
    vec3f position = vec3f(0.0, -0.001, 0.0);
    vec3f size = vec3f(CITY_GRID_SIZE * 3, 0.0, -CITY_GRID_SIZE * 2.0 * m_houseCount);
    Ground* ground = new Ground(position, size, groundColor, pShader);
    m_renderList.push_back(ground);

    // generate street z direction
    vec4f streetColor(0.0, 0.0, 0.0, 1.0);
    vec3f streetPosition = vec3f(0.6 * CITY_GRID_SIZE, 0.0, 0.0);
    vec3f streetSize = vec3f(CITY_GRID_SIZE * 0.6, 0.0, -CITY_GRID_SIZE * 2.0 * m_houseCount);
    Street* obj = new Street(streetPosition, streetSize, streetColor, pShader);
    m_renderList.push_back(obj);

    // generate streets x direction
    for (int z = 1; z < m_houseCount; ++z)
    {
        vec4f streetColor(0.0, 0.0, 0.0, 1.0);
        vec3f streetPosition = vec3f(0.0, 0.0, 0.6 - z * CITY_GRID_SIZE);
        vec3f streetSize = vec3f(CITY_GRID_SIZE * 3, 0.0, CITY_GRID_SIZE * 0.6);
        Street* obj = new Street(streetPosition, streetSize, streetColor, pShader);
        m_renderList.push_back(obj);
        m_updateList.push_back(obj);
    }

    // generate car
    vec3f carPosition(1.4 * CITY_GRID_SIZE, 0.001, -0.3);
    vec3f carSize(0.2f, 0.2f, 0.3f);
    vec4f carColor(0.7, 0.3, 0.3, 1.0);
    Car* car = new Car(carPosition, carSize, carColor, pShader);
    m_renderList.push_back(car);

    // generate houses
	vec4f houseColor(0.6, 0.6, 0.8, 1.0);
    for (int x = 0; x < 2; ++x)
    {
        for (int z = 0; z < m_houseCount; ++z)
        {
            float posx = x * 2.0 * CITY_GRID_SIZE;
            float posy = 0.0;
            float posz = -z * 2.0 * CITY_GRID_SIZE;

            vec3f housePosition(posx, posy, posz);
            vec3f houseSize(1.0, 1.0, 1.0);

            House* obj = new House(housePosition, houseSize, houseColor, pShader);

            m_renderList.push_back(obj);
            m_updateList.push_back(obj);
        }
    }
}
