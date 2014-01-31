############################################################################
#
# Copyright 2010-2012 BMW Car IT GmbH
# Copyright (C) 2013 DENSO CORPORATION
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

project(ilmClient)
project_type(CORE)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)


include_directories(
    "include"
    "${CMAKE_SOURCE_DIR}/config"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmClient/include"
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)

add_library(${PROJECT_NAME} SHARED
    src/ilm_client.c
    src/ilm_client_wayland_platform.c
    ../ilmCommon/src/ivi-application-protocol.c
)

add_dependencies(${PROJECT_NAME}
    ilmCommon
    ${WAYLAND_CLIENT_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ilmCommon
    rt
    dl
    ${CMAKE_THREAD_LIBS_INIT}
    ${WAYLAND_CLIENT_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib
)

install (
    FILES       ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmClient/include/ilm_client.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_VERSION} SOVERSION ${ILM_VERSION})
