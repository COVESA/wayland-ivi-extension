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

cmake_minimum_required (VERSION 2.6)

project(ilmControl)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

GET_TARGET_PROPERTY(ILM_COMMON_INCLUDE_DIRS ilmCommon INCLUDE_DIRECTORIES)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-wm-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
)

add_custom_command(
    OUTPUT  ivi-wm-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
)

add_custom_command(
    OUTPUT  ivi-input-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

add_custom_command(
    OUTPUT  ivi-input-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

include_directories(
    include
    ${ILM_COMMON_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    src/ilm_control_wayland_platform.c
    src/bitmap.c
    src/writepng.c
    ivi-wm-client-protocol.h
    ivi-wm-protocol.c
    ivi-input-client-protocol.h
    ivi-input-protocol.c
)

add_dependencies(${PROJECT_NAME}
    ${WAYLAND_CLIENT_LIBRARIES}
)

set(LIBS
    ${LIBS}
    rt
    dl
    png
    ${CMAKE_THREAD_LIBS_INIT}
    ${WAYLAND_CLIENT_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install (
    FILES       ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmControl/include/ilm_control.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_API_VERSION} SOVERSION ${ILM_API_VERSION})


#=============================================================================================
# generate documentation for ilmControl API
#=============================================================================================

add_custom_target(ilm-control-doc
                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's/___DOC_NAME___/ilmControl API/'
                          | sed 's/___DOC_VERSION___/${ILM_API_VERSION}/'
                          | sed 's!___INPUT_FILE___!${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_common.h ${CMAKE_CURRENT_SOURCE_DIR}/include/ilm_control.h!'
                          | sed 's/___OUTPUT_DIR___/ilm-control-doc/'
                          | doxygen -
                  COMMAND make --silent -C ilm-control-doc/latex
                  COMMAND cp ilm-control-doc/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ilm-control-api-${ILM_API_VERSION}.pdf
                  COMMENT "Generating ilm-control-api-${ILM_API_VERSION}.pdf"
)

#=============================================================================================
# generate pkg-config file for ilmControl API
#=============================================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ilmControl.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ilmControl.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ilmControl.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
