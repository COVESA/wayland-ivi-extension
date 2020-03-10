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

PROJECT(ivi-layermanagement-api-test)

IF(BUILD_ILM_API_TESTS)

    SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/modules)
    FIND_PACKAGE(gtest)

IF(NOT gtest_FOUND)
	MESSAGE(STATUS "gtest not found. Tests are not built, exiting.")
ELSE()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

    find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

    add_custom_command(
        OUTPUT  ivi-application-client-protocol.h
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
                < ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/ivi-application.xml
                > ${CMAKE_CURRENT_BINARY_DIR}/ivi-application-client-protocol.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/ivi-application.xml
    )

    INCLUDE_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmCommon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmControl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmInput/include
        ${CMAKE_CURRENT_BINARY_DIR}/../../protocol
        ${CMAKE_CURRENT_BINARY_DIR}
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${gtest_INCLUDE_DIRS}
    )

    LINK_DIRECTORIES(
        ${WAYLAND_CLIENT_LIBRARY_DIRS}
    )

    SET(LIBS
        ilmCommon
        ilmControl
        ilmInput
        ivi-application
        ${gtest_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARIES}
    )

    SET(SRC_FILES
        TestBase.cpp
        ilm_control_test.cpp
        ilm_control_notification_test.cpp
        ilm_input_test.cpp
        ilm_input_null_pointer_test.cpp
	ivi-application-client-protocol.h
    )

    SET(GCC_SANITIZER_COMPILE_FLAGS "-fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector-all")
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_SANITIZER_COMPILE_FLAGS}" )
    SET( CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static-libasan -static-libubsan" )

    ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})

    ADD_DEPENDENCIES(${PROJECT_NAME} ${LIBS})

    INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

    # use CTest
    ENABLE_TESTING()
    ADD_TEST(ilmCommon  ${PROJECT_NAME})
    ADD_TEST(ilmControl ${PROJECT_NAME})
    ADD_TEST(ilmInput ${PROJECT_NAME})

ENDIF()
ENDIF() 
