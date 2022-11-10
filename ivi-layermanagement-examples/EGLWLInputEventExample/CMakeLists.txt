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
pkg_check_modules(WAYLAND_CURSOR wayland-cursor REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-application-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

add_custom_command(
    OUTPUT  ivi-application-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

include_directories(
    ${GLESv2_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${WAYLAND_CURSOR_INCLUDE_DIRS}
    ${FFI_INCLUDE_DIRS}
    "include"
    ${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(
    ${GLESv2_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_CURSOR_LIBRARY_DIRS}
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
    ivi-application-protocol.c
    ${HEADER_FILES}
    ivi-application-client-protocol.h
)

add_dependencies(EGLWLInputEventExample
    wayland-client
    wayland-cursor
    wayland-egl
    ${LIBS}
)

set(LIBS
    ${LIBS}
    ${GLESv2_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${FFI_LIBRARIES}
    ${EGL_LIBRARIES}
)

add_definitions(${EGL_CFLAGS})

target_link_libraries(EGLWLInputEventExample ${LIBS})

install (TARGETS EGLWLInputEventExample DESTINATION bin)
