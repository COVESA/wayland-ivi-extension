############################################################################
#
# Copyright (C) 2013 DENSO CORPORATION
# Copyright 2014 BMW Car IT GmbH
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

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server>=1.9.0 REQUIRED)
pkg_check_modules(WESTON weston>=1.9.0 REQUIRED)
pkg_check_modules(PIXMAN pixman-1 REQUIRED)

find_package(Threads REQUIRED)
if (IVI_SHARE)
    pkg_check_modules(GBM gbm REQUIRED)
    pkg_check_modules(LIBDRM libdrm REQUIRED)
    ADD_DEFINITIONS("-DIVI_SHARE_ENABLE")
    SET(BUFFER_SHARING_SRC_FILES
        src/ivi-share.c
        src/ivi-share-gbm.c
    )
endif (IVI_SHARE)

GET_TARGET_PROPERTY(IVI_EXTENSION_INCLUDE_DIRS ivi-extension-protocol INCLUDE_DIRECTORIES)

include_directories(
    src
    ${IVI_EXTENSION_INCLUDE_DIRS}
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_SERVER_LIBRARY_DIRS}
    ${PIXMAN_LIBRARY_DIRS}
)


add_library(${PROJECT_NAME} MODULE
    src/ivi-extension.c src/ivi-controller-impl.c
    src/bitmap.c
    ${BUFFER_SHARING_SRC_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

add_dependencies(${PROJECT_NAME}
    ivi-extension-protocol
    ${WAYLAND_SERVER_LIBRARIES}
    ${PIXMAN_LIBRARIES}
)

set(LIBS
    ${LIBS}
    ivi-extension-protocol
    ${WAYLAND_SERVER_LIBRARIES}
)

if (IVI_SHARE)
     include_directories(
         ${include_directories}
         ${GBM_INCLUDE_DIRS}
         ${LIBDRM_INCLUDE_DIRS}
     )

     link_directories(
         ${link_directories}
         ${GBM_LIBRARY_DIRS}
         ${LIBDRM_LIBRARY_DIRS}
     )

     add_dependencies(${PROJECT_NAME}
         ${GBM_LIBRARIES}
         ${LIBDRM_LIBRARIES}
     )

     set(LIBS
         ${LIBS}
         ${GBM_LIBRARIES}
         ${LIBDRM_LIBRARIES}
     )
endif (IVI_SHARE)

set(CMAKE_C_LDFLAGS "-module -avoid-version")

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (
    TARGETS             ${PROJECT_NAME}
    LIBRARY DESTINATION lib/weston
)
