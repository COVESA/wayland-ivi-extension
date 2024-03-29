############################################################################
#
# Copyright 2010-2014 BMW Car IT GmbH
# Copyright (C) 2013 DENSO CORPORATION
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

cmake_minimum_required (VERSION 2.6...3.22)

project(ilmCommon)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

include_directories(
    include
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    src/ilm_common.c
    src/ilm_common_wayland_platform.c
)

target_link_libraries(${PROJECT_NAME}
    ${WAYLAND_CLIENT_LIBRARIES}
    ilmControl
)

add_dependencies(${PROJECT_NAME}
    ilmControl
)

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install (
    FILES       ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_common.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_types.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_platform.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_API_VERSION} SOVERSION ${ILM_API_VERSION})

#=============================================================================================
# generate pkg-config file for ilmCommon API
#=============================================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ilmCommon.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ilmCommon.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ilmCommon.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
