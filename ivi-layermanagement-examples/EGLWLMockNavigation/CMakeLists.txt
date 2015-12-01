############################################################################
#
# Copyright 2010-2014 BMW Car IT GmbH
# Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#		http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

cmake_minimum_required (VERSION 2.6)

project(EGLWLMockNavigation)

find_package(PkgConfig)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(WAYLAND_EGL wayland-egl REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

GET_TARGET_PROPERTY(ILM_CLIENT_INCLUDE_DIRS  ilmClient  INCLUDE_DIRECTORIES)

include_directories(
    include
    ${ILM_CLIENT_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
)

link_directories(
    ${GLESv2_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)


set (HEADER_FILES
    include/Camera.h
    include/Car.h
    include/Argument.h
    include/OpenGLES2App.h
    include/Ground.h
    include/House.h
    include/IRenderable.h
    include/IUpdateable.h
    include/MockNavi.h
    include/Street.h
    include/ShaderBase.h
    include/ShaderLighting.h
)

set (SRC_FILES
    src/Camera.cpp
    src/Car.cpp
    src/Argument.cpp
    src/OpenGLES2App.cpp
    src/Ground.cpp
    src/House.cpp
    src/MockNavi.cpp
    src/Street.cpp
    src/ShaderBase.cpp
    src/ShaderLighting.cpp
    src/main.cpp
    src/IlmMatrix.cpp
)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
)


set(LIBS
    ${LIBS}
    ${GLESv2_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${EGL_LIBRARIES}
    ilmClient
)

add_dependencies(${PROJECT_NAME}
    ${LIBS}
)

add_definitions(${EGL_CFLAGS})

target_link_libraries(${PROJECT_NAME}
    ${LIBS}
)

install(
    TARGETS     ${PROJECT_NAME}
    DESTINATION bin
)
