############################################################################
#
# Copyright 2012 BMW Car IT GmbH
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

project (simple-weston-client)

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT wayland-client REQUIRED)
pkg_check_modules(WAYLAND_CURSOR wayland-cursor REQUIRED)

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
    ${WAYLAND_CLIENT_INCLUDE_DIR}
    ${WAYLAND_CURSOR_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_CURSOR_LIBRARY_DIRS}
)

SET(LIBS
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
)

SET(SRC_FILES
    src/simple-weston-client.c
    ivi-application-protocol.c
    ivi-application-client-protocol.h
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_dependencies(${PROJECT_NAME} ${LIBS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
