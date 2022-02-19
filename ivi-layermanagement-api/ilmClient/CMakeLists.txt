############################################################################
#
# Copyright 2010-2014 BMW Car IT GmbH
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

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)

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

GET_TARGET_PROPERTY(ILM_COMMON_INCLUDE_DIRS ilmCommon INCLUDE_DIRECTORIES)

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
    src/ilm_client.c
    src/ilm_client_wayland_platform.c
    ivi-application-protocol.c
    ivi-application-client-protocol.h
)

set(LIBS
    ${LIBS}
    ${WAYLAND_CLIENT_LIBRARIES}
)

add_dependencies(${PROJECT_NAME}
    ${LIBS}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBS}
)

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install (
    FILES       ${CMAKE_CURRENT_SOURCE_DIR}/include/ilm_client.h
    DESTINATION include/ilm
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${ILM_API_VERSION} SOVERSION ${ILM_API_VERSION})


#=============================================================================================
# generate documentation for ilmClient API
#=============================================================================================

add_custom_target(ilm-client-doc
                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's/___DOC_NAME___/ilmClient API/'
                          | sed 's/___DOC_VERSION___/${ILM_API_VERSION}/'
                          | sed 's!___INPUT_FILE___!${CMAKE_CURRENT_SOURCE_DIR}/include/ilm_client.h ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include/ilm_common.h!'
                          | sed 's/___OUTPUT_DIR___/ilm-client-doc/'
                          | doxygen -
                  COMMAND make --silent -C ilm-client-doc/latex
                  COMMAND cp ilm-client-doc/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ilm-client-api-${ILM_API_VERSION}.pdf
                  COMMENT "Generating ilm-client-api-${ILM_API_VERSION}.pdf"
)

