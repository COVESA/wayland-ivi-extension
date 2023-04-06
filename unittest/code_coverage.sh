#!/bin/bash

############################################################################
#
# Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#               http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

# Global variable declare =============================================================================================

declare BUILD_DIR

declare -r RED='\033[0;31m'          # Red
declare -r GREEN='\033[0;32m'        # Green
declare -r YELLOW='\033[0;33m'       # Yellow
declare -r WHITE='\033[0;37m'        # White
declare -r NC='\033[0m'              # No color

print_error(){
    printf "${RED}$1${NC}\n"
}

print_information(){
    printf "${YELLOW}$1${NC}\n"
}

print_done(){
    printf "${GREEN}$1${NC}\n"
}

run_command(){
    if ! $1; then
        print_error "Failed at: $1" && exit 1
    fi
}

# Functions ============================================================================================================

build_tests(){
    print_information "Building tests....."

    UNITTEST_DIR=$(dirname "$(readlink -f "$0")")

    if [ -d "$UNITTEST_DIR/build_unittest/" ]; then
        run_command "rm -rf $UNITTEST_DIR/build_unittest/"
    fi

    BUILD_DIR=$UNITTEST_DIR/build_unittest/

    run_command "mkdir $BUILD_DIR"
    run_command "cd $BUILD_DIR"
    run_command "cmake -DBUILD_ILM_UNIT_TESTS=ON -DBUILD_CODE_COVERAGE=ON -DCMAKE_INSTALL_PREFIX=$WLD -DLIB_SUFFIX=//x86_64-linux-gnu ../../"
    run_command "make"

    print_done "Finish build tests!"
}

run_tests(){
    print_information "Running test files....."

    run_command "lcov -z -d $BUILD_DIR/"

    run_command "ctest --test-dir $BUILD_DIR/unittest/client/"
    run_command "ctest --test-dir $BUILD_DIR/unittest/server/"

    print_done "Finish run test files!"
}

generate_ccov(){
    print_information "Generating ccov....."

    run_command "lcov -c -d $BUILD_DIR/../ -o $BUILD_DIR/coverage.info --rc lcov_branch_coverage=1"
    run_command "lcov --remove $BUILD_DIR/coverage.info ' */install/include/* */usr/include/* */include/weston/* */build_ivi_extension/* \
                                                          */build_unittest/* */wayland-ivi-extension/unittest/* */bitmap.c */writepng.c \
                                                          */src/*.h ' -o $BUILD_DIR/coverage.info --rc lcov_branch_coverage=1"
    run_command "genhtml -o $BUILD_DIR/results/ $BUILD_DIR/coverage.info --rc lcov_branch_coverage=1"

    print_done "Finish generating ccov!"
}

show_coverage_data(){
    print_information "Show coverage data....."

    run_command "xdg-open $BUILD_DIR/results/index.html"

    print_done "Finish open html file!"
}

# Main =================================================================================================================

print_information "Starting script....."

build_tests
run_tests
generate_ccov
show_coverage_data

print_done "Finish script!"

# End main =============================================================================================================
