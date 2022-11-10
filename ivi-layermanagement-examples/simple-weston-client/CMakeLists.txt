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
pkg_check_modules(LIBWESTON_PROTOCOLS libweston-6-protocols QUIET)

if(${LIBWESTON_PROTOCOLS_FOUND})
    #check for DLT
    pkg_check_modules(DLT automotive-dlt QUIET)

    #import the pkgdatadir from libweston-protocols pkgconfig file
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=pkgdatadir libweston-6-protocols
                    OUTPUT_VARIABLE WestonProtocols_PKGDATADIR)
    string(REGEX REPLACE "[\r\n]" "" WestonProtocols_PKGDATADIR "${WestonProtocols_PKGDATADIR}")
    SET(LIBWESTON_PROTOCOLS_PKGDATADIR ${WestonProtocols_PKGDATADIR})
endif(${LIBWESTON_PROTOCOLS_FOUND})

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

if(${LIBWESTON_PROTOCOLS_FOUND})
    add_custom_command(
        OUTPUT  weston-debug-client-protocol.h
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
                < ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-client-protocol.h
        DEPENDS ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
    )

    add_custom_command(
        OUTPUT  weston-debug-server-protocol.h
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} server-header
                < ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-server-protocol.h
        DEPENDS ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
    )

    add_custom_command(
        OUTPUT  weston-debug-protocol.c
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
                < ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-protocol.c
        DEPENDS ${LIBWESTON_PROTOCOLS_PKGDATADIR}/weston-debug.xml
    )
endif(${LIBWESTON_PROTOCOLS_FOUND})

include_directories(
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${WAYLAND_CURSOR_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${DLT_INCLUDE_DIRS}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_CURSOR_LIBRARY_DIRS}
    ${DLT_LIBRARY_DIRS}
)

SET(LIBS
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
    ${DLT_LIBRARIES}
)

SET(SRC_FILES
    src/simple-weston-client.c
    ivi-application-protocol.c
    ivi-application-client-protocol.h
)

if(${LIBWESTON_PROTOCOLS_FOUND})
    SET(WESTON_DEBUG_SRC_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-protocol.c
        ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-client-protocol.h
        ${CMAKE_CURRENT_BINARY_DIR}/weston-debug-server-protocol.h
)
endif(${LIBWESTON_PROTOCOLS_FOUND})

add_executable(${PROJECT_NAME} ${SRC_FILES} ${WESTON_DEBUG_SRC_FILES})

add_dependencies(${PROJECT_NAME} ${LIBS})

add_definitions(${DLT_CFLAGS})

if(${DLT_FOUND})
    add_definitions(-DDLT)
endif(${DLT_FOUND})

if(${LIBWESTON_PROTOCOLS_FOUND})
    add_definitions(-DLIBWESTON_DEBUG_PROTOCOL)
    target_link_libraries(${PROJECT_NAME} pthread)
endif(${LIBWESTON_PROTOCOLS_FOUND})

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
