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

FIND_PATH(WESTON_INCLUDE_DIR compositor.h
/usr/include/weston
/include/weston
)

FIND_LIBRARY(WESTON_LIBRARIES
NAMES weston-layout
PATHS /lib/weston
)

SET( WESTON_FOUND "NO" )
IF(WESTON_INCLUDE_DIR AND WESTON_LIBRARIES)
    SET( WESTON_FOUND "YES" )
    message(STATUS "Found weston includes: ${WESTON_INCLUDE_DIR}")
    message(STATUS "Found weston libs: ${WESTON_LIBRARIES}")
ENDIF(WESTON_INCLUDE_DIR AND WESTON_LIBRARIES)

MARK_AS_ADVANCED(
  WESTON_INCLUDE_DIR
  WESTON_LIBRARIES
)
