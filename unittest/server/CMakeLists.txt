############################################################################
#
# Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
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
cmake_policy(SET CMP0046 OLD)

PROJECT(ilm_server_unittest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_SERVER wayland-server REQUIRED)
pkg_check_modules(WESTON weston>=5.0.0 REQUIRED)

GET_TARGET_PROPERTY(WESTON_IVI ivi-controller INCLUDE_DIRECTORIES)
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

add_custom_command(
    OUTPUT  ivi-input-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-input-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-input.xml
)

add_custom_command(
    OUTPUT  ivi-wm-protocol.c
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
            < ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
            > ${CMAKE_CURRENT_BINARY_DIR}/ivi-wm-protocol.c
    DEPENDS ${CMAKE_SOURCE_DIR}/protocol/ivi-wm.xml
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../weston-ivi-shell/src/
    ${CMAKE_CURRENT_BINARY_DIR}/../../ivi-input-modules/ivi-input-controller/
    ${CMAKE_CURRENT_BINARY_DIR}/../../weston-ivi-shell/
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ivi-layermanagement-api/ilmCommon/include/
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${WESTON_INCLUDE_DIRS}
    ${gtest_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${WAYLAND_SERVER_LIBRARY_DIRS}
)

SET(LIBS
    ${WAYLAND_SERVER_LIBRARIES}
    ${gtest_LIBRARIES}
)

SET(SRC_COMMON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server_api_fake.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_layout_interface_fake.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/common_fake_api.c
)

SET(SRC_ID_AGENT_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_id_agent_base_class.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_id_agent_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ivi-id-agent-modules/ivi-id-agent/src/ivi-id-agent.c
)

SET(SRC_CONTROLLER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_controller_base_class.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_controller_uinttests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../weston-ivi-shell/src/ivi-controller.c
    ivi-wm-protocol.c
)

SET(SRC_INPUT_CONTROLLER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_input_controller_base_class.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ivi_input_controller_uinttests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ivi-input-modules/ivi-input-controller/src/ivi-input-controller.c
    ivi-input-protocol.c
)

SET(GCC_SANITIZER_COMPILE_FLAGS "-fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector-all -fpermissive")
IF(BUILD_CODE_COVERAGE)
    SET(COVERAGE_COMPILER_FLAGS "-g --coverage")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_SANITIZER_COMPILE_FLAGS} -pthread ${COVERAGE_COMPILER_FLAGS}")
    SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static-libasan -static-libubsan ${COVERAGE_COMPILER_FLAGS}")
ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_SANITIZER_COMPILE_FLAGS} -pthread")
    SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -static-libasan -static-libubsan")
ENDIF()

ADD_EXECUTABLE(unittest-ivi-controller ${SRC_CONTROLLER_FILES} ${SRC_COMMON_FILES})
TARGET_LINK_LIBRARIES(unittest-ivi-controller ${LIBS})
ADD_DEPENDENCIES(unittest-ivi-controller ${LIBS} ivi-controller)
INSTALL(TARGETS unittest-ivi-controller DESTINATION bin)

ADD_EXECUTABLE(unittest-ivi-input-controller ${SRC_INPUT_CONTROLLER_FILES} ${SRC_COMMON_FILES})
TARGET_LINK_LIBRARIES(unittest-ivi-input-controller ${LIBS})
ADD_DEPENDENCIES(unittest-ivi-input-controller ${LIBS} ivi-input-controller)
INSTALL(TARGETS unittest-ivi-input-controller DESTINATION bin)

ADD_EXECUTABLE(unittest-ivi-id-agent ${SRC_ID_AGENT_FILES} ${SRC_COMMON_FILES})
TARGET_LINK_LIBRARIES(unittest-ivi-id-agent ${LIBS})
ADD_DEPENDENCIES(unittest-ivi-id-agent ${LIBS} ivi-id-agent)
INSTALL(TARGETS unittest-ivi-id-agent DESTINATION bin)

ADD_TEST(Unittest-ivi-controller unittest-ivi-controller)
ADD_TEST(Unittest-ivi-input-controller unittest-ivi-input-controller)
ADD_TEST(Unittest-ivi-id-agent unittest-ivi-id-agent)
