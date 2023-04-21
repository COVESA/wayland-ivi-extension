############################################################################
#
# Copyright 2014 BMW Car IT GmbH
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#               http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

FIND_PACKAGE(gtest)

IF(NOT gtest_FOUND)
    MESSAGE(STATUS "gtest not found, disabling unit tests (BUILD_ILM_API_TESTS=OFF)")
    SET(BUILD_ILM_API_TESTS FALSE CACHE BOOL "Build unit tests for IVI LayerManagement API" FORCE)
ENDIF()

IF(BUILD_ILM_API_TESTS)

    PROJECT(ivi-layermanagement-api-test)

    SET(TARGET_API ivi-layermanagement-api-test)
    SET(TARGET_ENV_CHECKING ivi-layermanagement-env-checking-test)

    find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

    add_custom_command(
        OUTPUT  ivi-wm-client-protocol.h
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
                < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-client-protocol.h
        DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
    )

    add_custom_command(
        OUTPUT  ivi-wm-protocol.c
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
                < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-protocol.c
        DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
    )

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

    LINK_DIRECTORIES(
        ${WAYLAND_CLIENT_LIBRARY_DIRS}
    )

    SET(GCC_SANITIZER_COMPILE_FLAGS "-fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector-all")
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_SANITIZER_COMPILE_FLAGS}" )
    SET( CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static-libasan -static-libubsan" )

    SET(TARGET_COMMON_LIBS
        ${gtest_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARIES}
    )
    #api tests
    SET(TARGET_API_LIBS
        ilmCommon
        ilmControl
        ilmInput
        ivi-application
    )
    SET(TARGET_API_SRC_FILES
        TestBase.cpp
        ilm_control_test.cpp
        ilm_control_notification_test.cpp
        ilm_input_test.cpp
        ilm_input_null_pointer_test.cpp
    )
    ADD_EXECUTABLE(${TARGET_API} ${TARGET_API_SRC_FILES})
    TARGET_INCLUDE_DIRECTORIES(${TARGET_API}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmCommon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmControl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmInput/include
        ${CMAKE_CURRENT_BINARY_DIR}/../../protocol
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${gtest_INCLUDE_DIRS}
    )
    TARGET_LINK_LIBRARIES(${TARGET_API} ${TARGET_API_LIBS} ${TARGET_COMMON_LIBS})
    ADD_DEPENDENCIES(${TARGET_API} ${TARGET_API_LIBS})
    INSTALL(TARGETS ${TARGET_API} DESTINATION bin)

    SET(TARGET_ENV_CHECKING_SRC_FILES
        ivi-wm-client-protocol.h
        ivi-wm-protocol.c
        ivi-input-client-protocol.h
        ivi-input-protocol.c
        ilm_check_env.cpp
    )
    ADD_EXECUTABLE(${TARGET_ENV_CHECKING} ${TARGET_ENV_CHECKING_SRC_FILES})
    TARGET_INCLUDE_DIRECTORIES(${TARGET_ENV_CHECKING}
        PUBLIC
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${gtest_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    TARGET_LINK_LIBRARIES(${TARGET_ENV_CHECKING} ${TARGET_COMMON_LIBS})
    INSTALL(TARGETS ${TARGET_ENV_CHECKING} DESTINATION bin)

    # use CTest
    ENABLE_TESTING()
    ADD_TEST(NAME ${TARGET_API} COMMAND ${TARGET_API})
    ADD_TEST(NAME ${TARGET_ENV_CHECKING} COMMAND ${TARGET_ENV_CHECKING})

ENDIF() 
