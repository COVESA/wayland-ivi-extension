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

FIND_PACKAGE(PkgConfig)
IF (PKG_CONFIG_FOUND)
# added aditional pkg config check to find dbus dependend includes
    pkg_check_modules(DBUS_PKG_PATHS dbus-1)
ENDIF (PKG_CONFIG_FOUND)


FIND_PATH(DBUS_INCLUDE_DIR dbus/dbus.h
${DBUS_PKG_PATHS_INCLUDE_DIRS}
/usr/include/dbus-1.0
)

FIND_PATH(DBUS_ARCH_INCLUDE_DIR dbus/dbus-arch-deps.h
${DBUS_PKG_PATHS_INCLUDE_DIRS}
/usr/lib/dbus-1.0/include
)

FIND_LIBRARY(DBUS_LIBRARY
NAMES dbus-1
PATHS /lib
)

SET( DBUS_FOUND "NO" )
IF(DBUS_LIBRARY)
    SET( DBUS_FOUND "YES" )
    message(STATUS "Found DBUS libs: ${DBUS_LIBRARY}")
    message(STATUS "Found DBUS includes: ${DBUS_INCLUDE_DIR}")
    message(STATUS "Found DBUS arch dependent includes: ${DBUS_ARCH_INCLUDE_DIR}")
ENDIF(DBUS_LIBRARY)

MARK_AS_ADVANCED(
  DBUS_INCLUDE_DIR
  DBUS_ARCH_INCLUDE_DIR
  DBUS_LIBRARY
)
