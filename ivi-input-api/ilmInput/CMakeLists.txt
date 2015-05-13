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

include_directories(
    include
    ${ILM_CONTROL_INCLUDE_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME} SHARED
    src/ilm_input.c
)

add_dependencies(${PROJECT_NAME}
    ilmControl
    ${WAYLAND_CLIENT_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ilmControl
    ivi-extension-protocol
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
    FILES       ${CMAKE_SOURCE_DIR}/ivi-input-api/ilmInput/include/ilm_input.h
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
