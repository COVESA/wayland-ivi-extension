############################################################################
#
# Copyright 2010, 2011 BMW Car IT GmbH
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

# check configuration

message(STATUS "Checking configuration, if valid.")
if (WITH_TESTS AND WITH_STATIC_LIBRARIES)
    message(FATAL_ERROR "unit tests (WITH_TESTS=ON) are not supported with static linking (WITH_STATIC_LIBRARIES=ON)")
endif (WITH_TESTS AND WITH_STATIC_LIBRARIES)


# generate cmake configuration header file, includes information on the
# project configuration, compiler settings, ...

message(STATUS "Generating config.h header file with build system configuration.")

foreach(FLAG ${BUILD_FLAG_LIST})
    SET(EXPORTED_BUILD_FLAGS "${EXPORTED_BUILD_FLAGS}${FLAG}\n")
endforeach(FLAG ${BUILD_FLAG_LIST})

LIST(SORT BUILD_FLAG_ARRAY)
foreach(FLAG ${BUILD_FLAG_ARRAY})
    SET(EXPORTED_BUILD_FLAG_ARRAY "${EXPORTED_BUILD_FLAG_ARRAY}    { DEBUG_FLAG, \"${FLAG} = ${${FLAG}}\" },\n")
endforeach(FLAG ${BUILD_FLAG_ARRAY})

foreach(PLUGIN ${STATICALLY_LINKED_PLUGINS})
    SET(STATIC_PLUGIN_REGISTRATION "${STATIC_PLUGIN_REGISTRATION} \\\n        REGISTER_PLUGIN(${PLUGIN})")
endforeach(PLUGIN ${STATICALLY_LINKED_PLUGINS})

configure_file(
    ${CMAKE_SOURCE_DIR}/config/res/config.h.cmake
    ${CMAKE_SOURCE_DIR}/config/config.h.1stpass
)

configure_file(
    ${CMAKE_SOURCE_DIR}/config/config.h.1stpass
    ${CMAKE_SOURCE_DIR}/config/config.h
)
