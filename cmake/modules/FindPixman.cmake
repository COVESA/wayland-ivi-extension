############################################################################
#
# Copyright (C) 2013 DENSO CORPORATION
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

FIND_PATH(PIXMAN_INCLUDE_DIR pixman.h
/usr/include/pixman-1
/include/pixman-1
)

FIND_LIBRARY(PIXMAN_LIBRARIES
NAMES pixman-1
PATHS /usr/lib
)

SET( PIXMAN_FOUND "NO" )
IF(PIXMAN_LIBRARIES)
    SET( PIXMAN_FOUND "YES" )
    message(STATUS "Found pixman libs: ${PIXMAN_LIBRARIES}")
    message(STATUS "Found pixman includes: ${PIXMAN_INCLUDE_DIR}")
ENDIF(PIXMAN_LIBRARIES)

MARK_AS_ADVANCED(
  PIXMAN_INCLUDE_DIR
  PIXMAN_LIBRARIES
)
