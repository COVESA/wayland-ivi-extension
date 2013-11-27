############################################################################
#
# Copyright 2010, 2011 BMW Car IT GmbH
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

FIND_PATH(CAIRO_INCLUDE_DIR cairo.h
/usr/include/cairo
)

FIND_LIBRARY(CAIRO_LIBRARIES
NAMES cairo
PATHS /usr/lib
)

SET( CAIRO_FOUND "NO" )
IF(CAIRO_LIBRARIES)
    SET( CAIRO_FOUND "YES" )
    message(STATUS "Found cairo libs: ${CAIRO_LIBRARIES}")
    message(STATUS "Found cairo includes: ${CAIRO_INCLUDE_DIR}")
ENDIF(CAIRO_LIBRARIES)

MARK_AS_ADVANCED(
  CAIRO_INCLUDE_DIR
  CAIRO_LIBRARIES
)
