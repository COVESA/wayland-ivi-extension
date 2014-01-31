############################################################################
#
# Copyright 2010-2012 BMW Car IT GmbH
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

find_package(PkgConfig)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(WAYLAND_EGL wayland-egl REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

include_directories(
    ${GLESv2_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmControl/include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmClient/include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include"
    "include"
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
        src/WaylandServerinfoProtocol.cpp
        src/IlmMatrix.cpp
)

add_executable(EGLWLMockNavigation
    ${SRC_FILES}
    ${HEADER_FILES}
)

add_dependencies(EGLWLMockNavigation
    ilmCommon
    ilmClient
    ilmControl
    wayland-client
)

set(LIBS
	${LIBS}
	${GLESv2_LIBRARIES}
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_EGL_LIBRARIES}
	${EGL_LIBRARIES}
	ilmClient
	ilmControl
)

target_link_libraries(EGLWLMockNavigation ${LIBS} -ldl)

install (TARGETS EGLWLMockNavigation DESTINATION bin)
