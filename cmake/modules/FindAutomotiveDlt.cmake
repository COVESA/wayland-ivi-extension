############################################################################
#
# Copyright 2012, BMW AG
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

FIND_PACKAGE(PkgConfig)
IF (PKG_CONFIG_FOUND)
# added aditional pkg config check to find dbus dependend includes
    pkg_check_modules(DLT_PKG_PATHS automotive-dlt)
ENDIF (PKG_CONFIG_FOUND)


FIND_PATH(DLT_INCLUDE_DIR dlt/dlt.h
${DLT_PKG_PATHS_INCLUDE_DIRS}
/usr/include
)


FIND_LIBRARY(DLT_LIBRARY
NAMES dlt
PATHS /lib
)

SET( DLT_FOUND "NO" )
IF(DLT_LIBRARY)
    SET( DLT_FOUND "YES" )
    message(STATUS "Found Dlt libs: ${DLT_LIBRARY}")
    message(STATUS "Found Dlt includes: ${DLT_INCLUDE_DIR}")
ENDIF(DLT_LIBRARY)

MARK_AS_ADVANCED(
  DLT_INCLUDE_DIR
  DLT_LIBRARY
)
