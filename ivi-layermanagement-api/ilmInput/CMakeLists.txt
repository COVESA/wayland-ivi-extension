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

project(ilmInput)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

GET_TARGET_PROPERTY(ILM_CONTROL_INCLUDE_DIRS ilmControl INCLUDE_DIRECTORIES)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

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
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ILM_CONTROL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    src/ilm_input.c
    ivi-input-client-protocol.h
    ivi-input-protocol.c
)

add_dependencies(${PROJECT_NAME}
    ilmControl
    ${WAYLAND_CLIENT_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ilmControl
    rt
    dl
    ${CMAKE_THREAD_LIBS_INIT}
    ${WAYLAND_CLIENT_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install (
    FILES       ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmInput/include/ilm_input.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_API_VERSION} SOVERSION ${ILM_API_VERSION})


#=============================================================================================
# generate documentation for ilmInput API
#=============================================================================================

add_custom_target(ilm-input-doc
                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's/___DOC_NAME___/ilmInput API/'
                          | sed 's/___DOC_VERSION___/${ILM_API_VERSION}/'
                          | sed 's!___INPUT_FILE___!${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_common.h ${CMAKE_CURRENT_SOURCE_DIR}/include/ilm_input.h!'
                          | sed 's/___OUTPUT_DIR___/ilm-input-doc/'
                          | doxygen -
                  COMMAND make --silent -C ilm-input-doc/latex
                  COMMAND cp ilm-input-doc/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ilm-input-api-${ILM_API_VERSION}.pdf
                  COMMENT "Generating ilm-input-api-${ILM_API_VERSION}.pdf"
)

#=============================================================================================
# generate pkg-config file for ilmInput API
#=============================================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ilmInput.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ilmInput.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/ilmInput.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
