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
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

cmake_minimum_required (VERSION 2.6)

project(EGLWLInputEventExample)

find_package(PkgConfig)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(WAYLAND_EGL wayland-egl REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

GET_TARGET_PROPERTY(ILM_CLIENT_INCLUDE_DIRS  ilmClient  INCLUDE_DIRECTORIES)

include_directories(
    ${ILM_CLIENT_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIR}
    ${EGL_INCLUDE_DIR}
    ${WAYLAND_CLIENT_INCLUDE_DIR}
    ${FFI_INCLUDE_DIR}
    "include"
)

link_directories(
    ${GLESv2_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

set (HEADER_FILES
    include/transform.h
    include/WLContext.h
    include/WLEGLSurface.h
    include/WLEyes.h
    include/WLEyesRenderer.h
    include/WLSurface.h
)

set (SRC_FILES
    src/main.cpp
    src/WLContext.cpp
    src/WLEGLSurface.cpp
    src/WLEyes.cpp
    src/WLEyesRenderer.cpp
    src/WLSurface.cpp
)

add_executable(EGLWLInputEventExample
    ${SRC_FILES}
    ${HEADER_FILES}
)

add_dependencies(EGLWLInputEventExample
    wayland-client
    wayland-egl
    ilmClient
    ${LIBS}
)

set(LIBS
    ${LIBS}
    ${GLESv2_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${FFI_LIBRARIES}
    ${EGL_LIBRARIES}
    ilmClient
)

add_definitions(${EGL_CFLAGS})

target_link_libraries(EGLWLInputEventExample ${LIBS})

install (TARGETS EGLWLInputEventExample DESTINATION bin)
