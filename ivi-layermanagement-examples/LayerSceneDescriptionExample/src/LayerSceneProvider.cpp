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

#include "LayerSceneProvider.h"
#include "ICommandExecutor.h"
#include "CommitCommand.h"
#include "LayerCreateCommand.h"
#include "LayerSetDestinationRectangleCommand.h"
#include "LayerSetSourceRectangleCommand.h"
#include "LayerSetVisibilityCommand.h"
#include "LayerSetOpacityCommand.h"
#include "ScreenSetRenderOrderCommand.h"
#include "LayerScene.h"
#include "SurfaceCreateCommand.h"
#include "SurfaceSetVisibilityCommand.h"
#include "SurfaceSetOpacityCommand.h"
#include <unistd.h>

ExampleSceneProvider::LayerSceneProvider(ICommandExecutor* executor)
: ISceneProvider(executor)
{
}

typedef struct t_layerScene
{
    unsigned int layer;
    bool visibility;
    float opacity;
} layerScene;

typedef struct t_surfaceScene
{
    unsigned int surface;
    bool visibility;
    float opacity;
} surfaceScene;


static layerScene gInitialLayerScene[] =
{
    { LAYER_EXAMPLE_VIDEO_APPLICATIONS, true, 1.0 },
    { LAYER_EXAMPLE_GLES_APPLICATIONS, true, 1.0 },
    { LAYER_EXAMPLE_X_APPLICATIONS, true, 1.0 }
};

static surfaceScene gInitialSurfaceScene[] =
{
    { SURFACE_EXAMPLE_EGLX11_APPLICATION,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_1,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_2,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_3,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_4,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_5,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_6,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_7,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_8,false,1.0 },
    { SURFACE_EXAMPLE_GDTESTENV_APPLICATION_9,false,1.0 },
    { SURFACE_EXAMPLE_GLXX11_APPLICATION,false,1.0 },
    { SURFACE_EXAMPLE_EGLRAW_APPLICATION,false,1.0 },
    { SURFACE_EXAMPLE_VIDEO_APPLICATION,false,1.0 }
};


bool ExampleSceneProvider::delegateScene()
{
    bool result = true;
    pid_t layermanagerPid = getpid();
    int i = 0;
    int numberOfLayers = sizeof(gInitialLayerScene) / sizeof (layerScene);
    int numberOfSurfaces = sizeof(gInitialSurfaceScene) / sizeof (surfaceScene);
    unsigned int *renderOrder = new unsigned int [numberOfLayers];
    unsigned int* screenResolution = m_executor->getScreenResolution(0);
    if ( numberOfLayers > 0 )
    {
        /* setup inital layer scenery */
        for (i = 0;i<numberOfLayers;i++)
        {
            result &= m_executor->execute(new LayerCreateCommand(layermanagerPid, screenResolution[0], screenResolution[1], &(gInitialLayerScene[i].layer)));
            result &= m_executor->execute(new LayerSetSourceRectangleCommand(layermanagerPid, gInitialLayerScene[i].layer, 0, 0, screenResolution[0], screenResolution[1]));
            result &= m_executor->execute(new LayerSetDestinationRectangleCommand(layermanagerPid, gInitialLayerScene[i].layer, 0, 0, screenResolution[0], screenResolution[1]));
            result &= m_executor->execute(new LayerSetOpacityCommand(layermanagerPid, gInitialLayerScene[i].layer, gInitialLayerScene[i].opacity) );
            result &= m_executor->execute(new LayerSetVisibilityCommand(layermanagerPid, gInitialLayerScene[i].layer, gInitialLayerScene[i].visibility) );
            result &= m_executor->execute(new CommitCommand(layermanagerPid));
            renderOrder[i]=gInitialLayerScene[i].layer;
        }
        /* Finally set the first executed renderorder */
        result &= m_executor->execute(new ScreenSetRenderOrderCommand(layermanagerPid, 0, renderOrder, numberOfLayers));
        result &= m_executor->execute(new CommitCommand(layermanagerPid));
    }

    if ( numberOfSurfaces > 0 )
    {
        /* setup inital surface scenery */
        for (i = 0;i<numberOfSurfaces;i++)
        {
            result &= m_executor->execute(new SurfaceCreateCommand(layermanagerPid, &(gInitialSurfaceScene[i].surface)));
            result &= m_executor->execute(new SurfaceSetOpacityCommand(layermanagerPid, gInitialSurfaceScene[i].surface, gInitialSurfaceScene[i].opacity));
            result &= m_executor->execute(new SurfaceSetVisibilityCommand(layermanagerPid, gInitialSurfaceScene[i].surface, gInitialSurfaceScene[i].visibility));
            result &= m_executor->execute(new CommitCommand(layermanagerPid));
        }
        /* Finally set the first executed renderorder */
    }
    return result;
}

extern "C" ISceneProvider* createLayerScene(ICommandExecutor* executor)
{
    return new ExampleSceneProvider(executor);
}

extern "C" void destroyLayerScene(ExampleSceneProvider* p)
{
    delete p;
}
