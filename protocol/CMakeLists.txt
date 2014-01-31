############################################################################
#
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

project(ivi-extension-protocol)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-application-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

add_custom_command(
    OUTPUT  ivi-application-server-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-server-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

add_custom_command(
    OUTPUT  ivi-application-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
)

add_custom_command(
    OUTPUT  ivi-controller-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
)

add_custom_command(
    OUTPUT  ivi-controller-server-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-server-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
)

add_custom_command(
    OUTPUT  ivi-controller-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)

add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-protocol.c
)
