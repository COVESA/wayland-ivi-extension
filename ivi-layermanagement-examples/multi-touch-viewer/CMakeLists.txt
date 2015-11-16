############################################################################
#
# \copyright (c) 2012, 2013 Advanced Driver Information Technology.
# This code is developed by Advanced Driver Information Technology.
# Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

cmake_minimum_required (VERSION 2.6)

project (generate-protocol-src)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-application-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

################################################################

project (multi-touch-viewer)

find_package(PkgConfig)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(WAYLAND_EGL wayland-egl REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

GET_TARGET_PROPERTY(IVI_EXTENSION_PROTOCOL_DIRS ivi-extension-protocol INCLUDE_DIRECTORIES)

include_directories(
    include
    ${IVI_EXTENSION_PROTOCOL_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)

link_directories(
    ${GLESv2_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

SET(LIBS
    ${LIBS}
    ${GLESv2_LIBRARIES}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${EGL_LIBRARIES}
    m
)

SET(SRC_FILES
    src/multi-touch-viewer.c
    src/util.c
    src/window.c
)

set(HEADER_FILES
    include/multi-touch-viewer.h
    include/util.h
    include/window.h
)

add_executable(${PROJECT_NAME}
               ${SRC_FILES}
               ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
)

add_dependencies(${PROJECT_NAME} ${LIBS})

add_definitions(${EGL_CFLAGS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
