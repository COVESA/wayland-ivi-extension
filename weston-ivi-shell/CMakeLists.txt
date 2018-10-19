############################################################################
#
# Copyright (C) 2013 DENSO CORPORATION
# Copyright 2014 BMW Car IT GmbH
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

project(ivi-controller)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server>=1.13.0 REQUIRED)
pkg_check_modules(WESTON weston>=5.0.0 REQUIRED)
pkg_check_modules(PIXMAN pixman-1 REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-wm-server-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-server-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
)

add_custom_command(
    OUTPUT  ivi-wm-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
)

INCLUDE (CheckFunctionExists)

CHECK_FUNCTION_EXISTS(posix_fallocate HAVE_POSIX_FALLOCATE)

configure_file(src/config.h.cmake config.h)

include_directories(
    src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_SERVER_LIBRARY_DIRS}
    ${PIXMAN_LIBRARY_DIRS}
)


add_library(${PROJECT_NAME} MODULE
    src/ivi-controller.c
    ivi-wm-protocol.c
    ivi-wm-server-protocol.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

add_dependencies(${PROJECT_NAME}
    ${WAYLAND_SERVER_LIBRARIES}
    ${PIXMAN_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ${WAYLAND_SERVER_LIBRARIES}
)

set(CMAKE_C_LDFLAGS "-module -avoid-version")

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}/weston
)
