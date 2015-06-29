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

    PROJECT(ivi-input-api-test)

    INCLUDE_DIRECTORIES(
        ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include
        ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmClient/include
        ${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmControl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ilmInput/include
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${gtest_INCLUDE_DIRS}
    )

    LINK_DIRECTORIES(
        ${WAYLAND_CLIENT_LIBRARY_DIRS}
    )

    SET(LIBS
        ilmCommon
        ilmClient
        ilmControl
        ilmInput
        ${gtest_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARY_DIRS}
    )

    SET(SRC_FILES
        TestBase.cpp
        ilm_input_test.cpp
        ilm_input_null_pointer_test.cpp
    )

    ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES})

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})

    ADD_DEPENDENCIES(${PROJECT_NAME} ${LIBS})

    IF(INSTALL_ILM_API_TESTS)
        INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
    ENDIF()

    # use CTest
    ENABLE_TESTING()
    ADD_TEST(ilmCommon  ${PROJECT_NAME})
    ADD_TEST(ilmClient  ${PROJECT_NAME})
    ADD_TEST(ilmControl ${PROJECT_NAME})
    ADD_TEST(ilmInput ${PROJECT_NAME})

ENDIF()
