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

FIND_PATH(EGL_INCLUDE_DIR EGL/egl.h
/usr/include
)

FIND_LIBRARY(EGL_LIBRARY
NAMES EGL
PATHS
)

SET( EGL_FOUND "NO" )
IF(EGL_LIBRARY)
    SET( EGL_FOUND "YES" )
    message(STATUS "Found EGL libs: ${EGL_LIBRARY}")
    message(STATUS "Found EGL includes: ${EGL_INCLUDE_DIR}")
ENDIF(EGL_LIBRARY)

MARK_AS_ADVANCED(
  EGL_INCLUDE_DIR
  EGL_LIBRARY
)
