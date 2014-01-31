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
#ifndef _LAYERSCENE_H_
#define _LAYERSCENE_H_

typedef enum e_layers
{
    LAYER_NEW = 0,
    LAYER_EXAMPLE_GLES_APPLICATIONS = 1000,
    LAYER_EXAMPLE_X_APPLICATIONS = 2000,
    LAYER_EXAMPLE_VIDEO_APPLICATIONS = 3000
} scenelayers;

typedef enum e_surfaces
{
    SURFACE_NEW = 0,
    SURFACE_EXAMPLE_EGLX11_APPLICATION = 10,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_1 = 11,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_2 = 12,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_3 = 13,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_4 = 14,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_5 = 15,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_6 = 16,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_7 = 17,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_8 = 18,
    SURFACE_EXAMPLE_GDTESTENV_APPLICATION_9 = 19,
    SURFACE_EXAMPLE_GLXX11_APPLICATION = 20,
    SURFACE_EXAMPLE_EGLRAW_APPLICATION = 30,
    SURFACE_EXAMPLE_VIDEO_APPLICATION = 40
} sceneSurfaces;

#endif /* _LAYERSCENE_H_ */
