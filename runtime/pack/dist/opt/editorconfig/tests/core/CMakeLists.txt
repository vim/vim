# CMakeLists.txt for core testing in
# editorconfig-core-vimscript and editorconfig-vim.

# Copyright (c) 2011-2019 EditorConfig Team
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# To perform the test, from the root of the project tree, run
#   mkdir build
#   cd build
#   cmake ..
#   ctest .

cmake_minimum_required(VERSION 3.5)
#set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# Do not check any compiler
project(editorconfig-core-vimscript NONE)

enable_testing()

# The test executable to use
if(NOT WIN32)
    set(EDITORCONFIG_CMD "${CMAKE_SOURCE_DIR}/editorconfig")
else()
    set(EDITORCONFIG_CMD "${CMAKE_SOURCE_DIR}/editorconfig.bat")
endif()
set(EDITORCONFIG_CMD_IS_TARGET FALSE)

add_subdirectory(tests)

# CTestCustom.cmake contains platform-specific test configuration.
configure_file(CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
