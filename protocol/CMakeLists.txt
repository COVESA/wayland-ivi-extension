############################################################################
#
# Copyright 2014 BMW Car IT GmbH
#
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

project (ivi-application)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client>=1.13.0 REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server>=1.13.0 REQUIRED)

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
    ${CMAKE_CURRENT_BINARY_DIR}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_SERVER_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
)

target_link_libraries (
  ${PROJECT_NAME}
  ${WAYLAND_CLIENT_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      COMPILE_FLAGS "-fPIC")

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    DESTINATION include/ilm
)

install(
    FILES ${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml
    DESTINATION share/wayland-protocols/stable/ivi-application
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_API_VERSION} SOVERSION ${ILM_API_VERSION})

#=============================================================================================
# generate pkg-config file for ivi-application API
#=============================================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ivi-application.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ivi-application.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ivi-application.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
