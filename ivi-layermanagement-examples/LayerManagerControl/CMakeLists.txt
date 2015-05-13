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

project (LayerManagerControl)

include_directories(
    include
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmClient/include"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmControl/include"
    "${CMAKE_SOURCE_DIR}/ivi-input-api/ilmInput/include"
    ${WAYLAND_CLIENT_INCLUDE_DIR}
)

link_directories(
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
)

SET(LIBS
    ilmClient
    ilmControl
)

SET(SRC_FILES
    src/main.cpp
    src/commands.cpp
    src/analyze.cpp
    src/common.cpp
    src/control.cpp
    src/demo.cpp
    src/Expression.cpp
    src/ExpressionInterpreter.cpp
    src/print.cpp
    src/scatter.cpp
    src/sceneio.cpp
    src/util.cpp
)

if (WITH_ILM_INPUT)
    SET(SRC_FILES
        ${SRC_FILES}
        src/input_commands.cpp
    )

    SET(LIBS ${LIBS} ilmInput)
ENDIF()

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_dependencies(${PROJECT_NAME} ${LIBS})

target_link_libraries(${PROJECT_NAME} ${LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)
