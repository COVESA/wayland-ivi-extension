###############################################################################
#
# Copyright (C) 2017 Advanced Driver Information Technology Joint Venture GmbH
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

cmake_minimum_required (VERSION 2.6)

project(ivi-id-agent)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)
pkg_check_modules(WESTON weston>=6.0.0 REQUIRED)
pkg_check_modules(PIXMAN pixman-1 REQUIRED)
pkg_check_modules(LIBWESTON_DESKTOP libweston-desktop-8 REQUIRED)

find_package(Threads REQUIRED)

include_directories(
    src
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_SERVER_LIBRARY_DIRS}
    ${PIXMAN_LIBRARY_DIRS}
)


add_library(${PROJECT_NAME} MODULE
    src/ivi-id-agent.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

add_dependencies(${PROJECT_NAME}
    ${WAYLAND_SERVER_LIBRARIES}
    ${PIXMAN_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ${WAYLAND_SERVER_LIBRARIES}
    ${LIBWESTON_DESKTOP_LIBRARIES}
)

set(CMAKE_C_LDFLAGS "-module -avoid-version")

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib${LIB_SUFFIX}/weston
)
