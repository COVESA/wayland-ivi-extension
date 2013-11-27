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

project(LayerManagerUtils)
project_type(CORE)

find_package (Threads)

add_library(${PROJECT_NAME} STATIC
    src/Bitmap.cpp
    src/IlmMatrix.cpp
    src/Log.cpp
    src/LogMessageBuffer.cpp
    src/ThreadBase.cpp
    src/IpcModuleLoader.cpp
)

set(INCLUDE_DIRS
    "include"
    "${CMAKE_SOURCE_DIR}/config"
    "${CMAKE_SOURCE_DIR}/ivi-layermanagement-api/ilmCommon/include"
)

set(LIBS
    ${LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    dl
)

if (WITH_DLT)

    find_package(AutomotiveDlt REQUIRED)

    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        ${DLT_INCLUDE_DIR}
    )

    set(LIBS
        ${LIBS}
        ${DLT_LIBRARY}
    )

    target_link_libraries(${PROJECT_NAME} ${LIBS})

endif (WITH_DLT)

include_directories(${INCLUDE_DIRS})

install(FILES
            include/Log.h
            include/LogMessageBuffer.h
            include/IlmMatrix.h
            include/Bitmap.h
            include/IpcModuleLoader.h
        DESTINATION
            include/layermanager
)

install(TARGETS
            ${PROJECT_NAME}
        DESTINATION
            lib/layermanager/static
)

add_subdirectory(tests)
