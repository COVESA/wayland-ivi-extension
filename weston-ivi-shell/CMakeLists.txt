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
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)
pkg_check_modules(CAIRO cairo REQUIRED)
pkg_check_modules(WESTON weston REQUIRED)
pkg_check_modules(PIXMAN pixman-1 REQUIRED)

GET_TARGET_PROPERTY(IVI_EXTENSION_INCLUDE_DIRS ivi-extension-protocol INCLUDE_DIRECTORIES)

include_directories(
    include
    ${IVI_EXTENSION_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
)


add_library(${PROJECT_NAME} MODULE
    src/ivi-controller.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

add_dependencies(${PROJECT_NAME}
    ivi-extension-protocol
    ${WAYLAND_SERVER_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${WESTON_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ivi-extension-protocol
    ${WAYLAND_SERVER_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${WESTON_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib/weston
)
