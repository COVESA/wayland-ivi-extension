############################################################################
#
# Copyright 2010-2012 BMW Car IT GmbH
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

cmake_minimum_required (VERSION 2.6)

project(ilmCommon)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT ivi-application-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header <${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml> ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ivi-application-client-protocol.h
)

add_custom_command(
    OUTPUT ivi-application-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code <${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml> ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/src/ivi-application-protocol.c
)

add_custom_command(
    OUTPUT ivi-controller-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header <${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml> ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ivi-controller-client-protocol.h
)

add_custom_command(
    OUTPUT ivi-controller-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code <${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml> ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/src/ivi-controller-protocol.c
)

add_custom_target(
    generate_wayland_protocol_codes_for_client
    DEPENDS ivi-application-client-protocol.h ivi-application-protocol.c ivi-controller-client-protocol.h ivi-controller-protocol.c
)

include_directories(
    "include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include"
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)


add_library(${PROJECT_NAME} SHARED
    src/ilm_common.c
    src/ilm_common_wayland_platform.c
    src/ivi-application-protocol.c
    src/ivi-controller-protocol.c
)

add_dependencies(${PROJECT_NAME}
    generate_wayland_protocol_codes_for_client
    ${WAYLAND_CLIENT_LIBRARIES}
)

set(LIBS
    ${LIBS}
    rt
    dl
    ${CMAKE_THREAD_LIBS_INIT}
    ${WAYLAND_CLIENT_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS} -ldl)

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib
)

install (
    FILES       ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_common.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_types.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_configuration.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_tools.h
                ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_platform.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_VERSION} SOVERSION ${ILM_VERSION})
