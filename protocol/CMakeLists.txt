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

project(ivi-extension-protocol)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT wayland-client>=1.9.0 REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server>=1.9.0 REQUIRED)

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

add_custom_command(
    OUTPUT  ivi-input-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

add_custom_command(
    OUTPUT  ivi-input-server-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-server-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

add_custom_command(
    OUTPUT  ivi-input-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

add_custom_command(
    OUTPUT  ivi-share-client-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-client-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
)

add_custom_command(
    OUTPUT  ivi-share-server-protocol.h
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-server-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
)

add_custom_command(
    OUTPUT  ivi-share-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-share.xml
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

add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-server-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ivi-share-protocol.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      COMPILE_FLAGS "-fPIC")


#=============================================================================================
# generate documentation for ivi-application API
#=============================================================================================
SET(IVI_APP_XML      ${CMAKE_CURRENT_SOURCE_DIR}/ivi-application.xml)
SET(IVI_APP_CLIENT_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client.h)
SET(IVI_APP_SERVER_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-server.h)

add_custom_target(ivi-application-doc
                  COMMAND wayland-scanner client-header < ${IVI_APP_XML} > ${IVI_APP_CLIENT_H}
                  COMMAND wayland-scanner server-header < ${IVI_APP_XML} > ${IVI_APP_SERVER_H}

                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's!___DOC_NAME___!IVI Application API!'
                          | sed 's!___DOC_VERSION___!${IVI_EXTENSION_VERSION}!'
                          | sed 's!___INPUT_FILE___!${IVI_APP_CLIENT_H} ${IVI_APP_SERVER_H}!'
                          | sed 's!___OUTPUT_DIR___!ivi-application-tmp!'
                          | doxygen -

                  COMMAND make --silent -C ivi-application-tmp/latex

                  COMMAND cp ivi-application-tmp/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ivi-application-api-${IVI_EXTENSION_VERSION}.pdf

                  DEPENDS ${IVI_APP_XML}

                  COMMENT "Generating ivi-application-api-${IVI_EXTENSION_VERSION}.pdf"
)

#=============================================================================================
# generate documentation ivi-controller API
#=============================================================================================
SET(IVI_CTL_XML      ${CMAKE_CURRENT_SOURCE_DIR}/ivi-controller.xml)
SET(IVI_CTL_CLIENT_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-client.h)
SET(IVI_CTL_SERVER_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-controller-server.h)

add_custom_target(ivi-controller-doc
                  COMMAND wayland-scanner client-header < ${IVI_CTL_XML} > ${IVI_CTL_CLIENT_H}
                  COMMAND wayland-scanner server-header < ${IVI_CTL_XML} > ${IVI_CTL_SERVER_H}

                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's!___DOC_NAME___!IVI Controller API!'
                          | sed 's!___DOC_VERSION___!${IVI_EXTENSION_VERSION}!'
                          | sed 's!___INPUT_FILE___!${IVI_CTL_CLIENT_H} ${IVI_CTL_SERVER_H}!'
                          | sed 's!___OUTPUT_DIR___!ivi-controller-tmp!'
                          | doxygen -

                  COMMAND make --silent -C ivi-controller-tmp/latex

                  COMMAND cp ivi-controller-tmp/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ivi-controller-api-${IVI_EXTENSION_VERSION}.pdf

                  DEPENDS ${IVI_CTL_XML}

                  COMMENT "Generating ivi-controller-api-${IVI_EXTENSION_VERSION}.pdf"
)

#=============================================================================================
# generate documentation ivi-input API
#=============================================================================================
SET(IVI_CTL_INP_XML      ${CMAKE_CURRENT_SOURCE_DIR}/ivi-input.xml)
SET(IVI_CTL_INP_CLIENT_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-client.h)
SET(IVI_CTL_INP_SERVER_H ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-server.h)

add_custom_target(ivi-input-doc
                  COMMAND wayland-scanner client-header < ${IVI_CTL_INP_XML} > ${IVI_CTL_INP_CLIENT_H}
                  COMMAND wayland-scanner server-header < ${IVI_CTL_INP_XML} > ${IVI_CTL_INP_SERVER_H}

                  COMMAND cat ${CMAKE_SOURCE_DIR}/doc/Doxyfile.template
                          | sed 's!___DOC_NAME___!IVI Controller API!'
                          | sed 's!___DOC_VERSION___!${IVI_EXTENSION_VERSION}!'
                          | sed 's!___INPUT_FILE___!${IVI_CTL_INP_CLIENT_H} ${IVI_CTL_INP_SERVER_H}!'
                          | sed 's!___OUTPUT_DIR___!ivi-input-tmp!'
                          | doxygen -

                  COMMAND make --silent -C ivi-input-tmp/latex

                  COMMAND cp ivi-input-tmp/latex/refman.pdf
                             ${CMAKE_BINARY_DIR}/ivi-input-api-${IVI_EXTENSION_VERSION}.pdf

                  DEPENDS ${IVI_CTL_INP_XML}

                  COMMENT "Generating ivi-input-api-${IVI_EXTENSION_VERSION}.pdf"
)
