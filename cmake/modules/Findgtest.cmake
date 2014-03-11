SET(MODULE_NAME gtest)

SET(${MODULE_NAME}_FOUND FALSE)

FIND_PATH(${MODULE_NAME}_INCLUDE_DIRS
    NAMES gtest/gtest.h
    PATHS /usr/include /usr/local/include
)

FIND_LIBRARY(LIB_GTEST
    NAMES gtest
    PATHS /usr/lib
          /usr/lib64
          /usr/local/lib
          /usr/local/lib64
)

FIND_LIBRARY(LIB_GTEST_MAIN
    NAMES gtest_main
    PATHS /usr/lib
          /usr/lib64
          /usr/local/lib
          /usr/local/lib64
)

IF(LIB_GTEST AND LIB_GTEST_MAIN)
    SET(${MODULE_NAME}_FOUND TRUE)
    SET(${MODULE_NAME}_LIBRARIES ${LIB_GTEST} ${LIB_GTEST_MAIN})
ENDIF()

MARK_AS_ADVANCED(
    ${MODULE_NAME}_FOUND
    ${MODULE_NAME}_INCLUDE_DIRS
    ${MODULE_NAME}_LIBRARIES
)

MESSAGE(STATUS "${MODULE_NAME}_INCLUDE_DIRS: ${${MODULE_NAME}_INCLUDE_DIRS}")
MESSAGE(STATUS "${MODULE_NAME}_LIBRARIES:    ${${MODULE_NAME}_LIBRARIES}")
