############################################################################
#
# Copyright 2010-2014 BMW Car IT GmbH
# Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
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
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

project(wayland-ivi-extension)

include(GNUInstallDirs)

SET(IVI_EXTENSION_VERSION 2.2.0)
SET(ILM_API_VERSION 2.2.0)

SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter" )
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter" )

add_subdirectory(protocol)

add_subdirectory(weston-ivi-shell)

add_subdirectory(ivi-layermanagement-api/ilmCommon)
add_subdirectory(ivi-layermanagement-api/ilmClient)
add_subdirectory(ivi-layermanagement-api/ilmControl)
add_subdirectory(ivi-layermanagement-api/test)
add_subdirectory(ivi-layermanagement-examples)
add_subdirectory(ivi-layermanagement-api/ilmInput)
add_subdirectory(ivi-input-modules/ivi-input-controller)
add_subdirectory(ivi-id-agent-modules/ivi-id-agent)
add_subdirectory(unittest)


#=============================================================================================
# generate all documents
#=============================================================================================
add_custom_target(doc
                  COMMAND echo "====================================="
                  COMMAND echo "generated documents"
                  COMMAND echo "    `pwd`/ivi-application-api-${IVI_EXTENSION_VERSION}.pdf"
                  COMMAND echo "    `pwd`/ivi-controller-api-${IVI_EXTENSION_VERSION}.pdf"
                  COMMAND echo "    `pwd`/ilm-client-api-${ILM_API_VERSION}.pdf"
                  COMMAND echo "    `pwd`/ilm-control-api-${ILM_API_VERSION}.pdf"
                  COMMAND echo "====================================="
                  DEPENDS ivi-application-doc
                          ivi-controller-doc
                          ilm-client-doc
                          ilm-control-doc
)

