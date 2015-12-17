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

cmake_minimum_required (VERSION 2.6)
project (ivi-layermanagement-examples)

#===========================================================================================================
# cmake configuration
#===========================================================================================================
add_subdirectory(LayerManagerControl)
add_subdirectory(EGLWLMockNavigation)
add_subdirectory(EGLWLInputEventExample)
add_subdirectory(layer-add-surfaces)
add_subdirectory(multi-touch-viewer)
add_subdirectory(simple-ivi-share)
