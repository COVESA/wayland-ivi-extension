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

GET_TARGET_PROPERTY(IVI_EXTENSION_PROTOCOL_DIRS ivi-extension-protocol INCLUDE_DIRECTORIES)

include_directories(
    ${IVI_EXTENSION_PROTOCOL_DIRS}
    ${WAYLAND_CLIENT_INCLUDE_DIR}
    ${WAYLAND_CURSOR_INCLUDE_DIR}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_CURSOR_LIBRARY_DIRS}
)

SET(LIBS
    ivi-application
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
)

SET(SRC_FILES
    src/simple-weston-client.c
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_dependencies(${PROJECT_NAME} ${LIBS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
