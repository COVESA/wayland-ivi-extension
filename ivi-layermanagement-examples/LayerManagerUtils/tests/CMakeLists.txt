############################################################################
#
# Copyright 2010, 2011 BMW Car IT GmbH
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

cmake_minimum_required (VERSION 2.6)

if (WITH_TESTS)

    enable_testing()

    project(LayerManagerUtils_Test)
    project_type(TEST)

    set(LIBS
        ${LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
        LayerManagerUtils
        dl
        gtest
    )

    include_directories(${INCLUDE_DIRS})

    add_executable(${PROJECT_NAME}
        BitmapTest.cpp
        LogTest.cpp
    )

    target_link_libraries(${PROJECT_NAME} ${LIBS})

    install(TARGETS     ${PROJECT_NAME}
            DESTINATION bin
    )

    add_test(LayerManagerUtils ${PROJECT_NAME})

endif(WITH_TESTS)
