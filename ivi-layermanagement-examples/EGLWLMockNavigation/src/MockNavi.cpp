/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 * Copyright (C) 2018 Advanced Driver Information Technology Joint Venture GmbH
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
#include "Sky.h"
#include "configuration.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define CITY_GRID_SIZE 1.0f

MockNavi::MockNavi(float fps, float animationSpeed, SurfaceConfiguration* config)
: OpenGLES2App(fps, animationSpeed, config)
, m_camera(vec3f(-1.5 * CITY_GRID_SIZE, -0.1, 0.0), vec3f(0.0, 0.0, 0.0), config->surfaceWidth, config->surfaceHeight)
, m_houseCount(15)
, nosky(config->nosky)
{
    generateCity();
}

MockNavi::~MockNavi()
{
    delete pShader;
    delete pShaderTexture;
    delete pShaderGradient;
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
    float* projection = m_camera.getViewProjectionMatrix();
    pShader = new ShaderLighting(projection);
    pShaderTexture = new ShaderTexture(projection);
    pShaderGradient = new ShaderGradient();
    TextureLoader* carTexture = new TextureLoader;
    bool carTextureLoaded = carTexture->loadBMP((texturePath + std::string("/car.bmp")).c_str());
    TextureLoader* streetTexture = new TextureLoader;
    bool streetTextureLoaded = streetTexture->loadBMP((texturePath + std::string("/street.bmp")).c_str());
    list<TextureLoader*> houseTextures;
    for(int i = 0; i < m_houseCount; i++){
        TextureLoader* houseTexture = new TextureLoader;
        std::string houseTexturePath = texturePath;
        houseTexturePath.append("/skyscrapers/facade0");
        houseTexturePath.append(std::to_string(i));
        houseTexturePath.append(".bmp");
        bool houseTextureLoaded = houseTexture->loadBMP(houseTexturePath.c_str());
        if(not houseTextureLoaded){
            break;
        }
        houseTextures.push_back(houseTexture);
    }

    // generate sky
    if (not nosky) {
        vec4f skyColor(0.0, 0.0, 1.0, 1.0);
        vec3f skyPosition = vec3f(-1.0, -1.0, 1.0);
        vec3f skySize = vec3f(2.0, 2.0, 0.0);
        Sky* sky = new Sky(skyPosition, skySize, skyColor, pShaderGradient);
        m_renderList.push_back(sky);
        m_updateList.push_back(sky);
    }

    // generate base plate
	vec4f groundColor(0.8, 0.8, 0.6, 1.0);
    vec3f position = vec3f(0.0, -0.001, 0.0);
    vec3f size = vec3f(CITY_GRID_SIZE * 3, 0.0, -CITY_GRID_SIZE * 2.0 * m_houseCount);
    Ground* ground = new Ground(position, size, groundColor, pShader);
    m_renderList.push_back(ground);

    // generate street z direction
    vec4f streetColor(0.0, 0.0, 0.0, 1.0);
    vec3f streetPosition = vec3f(0.6 * CITY_GRID_SIZE, 0.001, 0.0);
    vec3f streetSize = vec3f(CITY_GRID_SIZE * 0.6, 0.0, -CITY_GRID_SIZE * 2.0 * m_houseCount * 200.0);
    Street* obj = nullptr;
        if(streetTextureLoaded){
            obj = new Street(streetPosition, streetSize, streetColor, pShaderTexture, streetTexture);
        }else{
            obj = new Street(streetPosition, streetSize, streetColor, pShader);
    }
    m_renderList.push_back(obj);
    m_updateList.push_back(obj);

    // generate streets x direction
    for (int z = 1; z < m_houseCount; ++z)
    {
        vec4f streetColor(0.0, 0.0, 0.0, 1.0);
        vec3f streetPosition = vec3f(0.0, 0.0, 0.6 - z * CITY_GRID_SIZE);
        vec3f streetSize = vec3f(CITY_GRID_SIZE * 3, 0.0, CITY_GRID_SIZE * 0.6);
        Street* obj = nullptr;
        if(streetTextureLoaded){
            obj = new Street(streetPosition, streetSize, streetColor, pShaderTexture, streetTexture);
        }else{
            obj = new Street(streetPosition, streetSize, streetColor, pShader);
        }
        m_renderList.push_back(obj);
        m_updateList.push_back(obj);
    }

    // generate car
    vec3f carSize(0.2f, 0.2f, 0.3f);
    vec4f carColor(0.7, 0.3, 0.3, 1.0);
    Car* car = nullptr;
    if(carTextureLoaded){
        vec3f carPosition(1.39 * CITY_GRID_SIZE, 0.002, -0.3);
        car = new Car(carPosition, carSize, carColor, pShaderTexture, carTexture);
    } else {
        vec3f carPosition(1.4 * CITY_GRID_SIZE, 0.002, -0.3);
        car = new Car(carPosition, carSize, carColor, pShader);
    }
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

            TextureLoader* houseTexture = houseTextures.front();

            House* obj = nullptr;

            if(houseTexture == nullptr){
                obj = new House(housePosition, houseSize, houseColor, pShader);
            } else {
                houseTextures.pop_front();
                obj = new House(housePosition, houseSize, houseColor, pShaderTexture, houseTexture);
                houseTextures.push_back(houseTexture);
            }

            m_renderList.push_back(obj);
            m_updateList.push_back(obj);
        }
    }
}
