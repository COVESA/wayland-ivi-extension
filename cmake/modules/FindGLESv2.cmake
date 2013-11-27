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

FIND_PATH(GLESv2_INCLUDE_DIR GLES2/gl2.h
/usr/include
)

FIND_LIBRARY(GLESv2_LIBRARIES
NAMES GLESv2
PATHS
)

SET( GLESv2_FOUND "NO" )
IF(GLESv2_LIBRARIES)
    SET( GLESv2_FOUND "YES" )
    message(STATUS "Found GLESv2 libs: ${GLESv2_LIBRARIES}")
    message(STATUS "Found GLESv2 includes: ${GLESv2_INCLUDE_DIR}")
ENDIF(GLESv2_LIBRARIES)

MARK_AS_ADVANCED(
  GLESv2_INCLUDE_DIR
  GLESv2_LIBRARIES
)
