############################################################################
#
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

project(ivi-controller)
project_type(CORE)

find_package(Cairo REQUIRED)
find_package(Wayland REQUIRED)
find_package(Weston REQUIRED)
find_package(Pixman REQUIRED)

add_custom_command(
    OUTPUT ivi-application-server-protocol.h
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/wayland-scanner server-header <${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml> ${CMAKE_SOURCE_DIR}/weston-ivi-shell/include/ivi-application-server-protocol.h
)

add_custom_command(
    OUTPUT ivi-application-protocol.c
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/wayland-scanner code <${CMAKE_SOURCE_DIR}/protocol/ivi-application.xml> ${CMAKE_SOURCE_DIR}/weston-ivi-shell/src/ivi-application-protocol.c
)

add_custom_command(
    OUTPUT ivi-controller-server-protocol.h
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/wayland-scanner server-header <${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml> ${CMAKE_SOURCE_DIR}/weston-ivi-shell/include/ivi-controller-server-protocol.h
)

add_custom_command(
    OUTPUT ivi-controller-protocol.c
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/wayland-scanner code <${CMAKE_SOURCE_DIR}/protocol/ivi-controller.xml> ${CMAKE_SOURCE_DIR}/weston-ivi-shell/src/ivi-controller-protocol.c
)

add_custom_target(
    generate_wayland_protocol_codes_for_server
    DEPENDS ivi-application-server-protocol.h ivi-application-protocol.c ivi-controller-server-protocol.h ivi-controller-protocol.c
)

include_directories(
    "include"
    ${WAYLAND_SERVER_INCLUDE_DIR}
    ${CAIRO_INCLUDE_DIR}
    ${WESTON_INCLUDE_DIR}
    ${PIXMAN_INCLUDE_DIR}
)


add_library(${PROJECT_NAME} MODULE
    src/ivi-controller.c
    src/ivi-controller-protocol.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

add_dependencies(${PROJECT_NAME}
    generate_wayland_protocol_codes_for_server
    ${WAYLAND_SERVER_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${WESTON_LIBRARIES}
)

set(LIBS
    ${LIBS}
    rt
    dl
    ${WAYLAND_SERVER_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${WESTON_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${LIBS} -ldl)

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib/weston
)
