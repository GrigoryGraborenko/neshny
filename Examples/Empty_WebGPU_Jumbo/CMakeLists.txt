###############################################################################
# WINDOWS:
# cmake . -B build -G "Visual Studio 17 2022" -A x64
# 
# MACOS: (different dirs for Debug/Release)
# cmake . -B build -G "Unix Makefiles"-DCMAKE_BUILD_TYPE=Debug
# cmake --build build
# 
# BROWSER:
# emcmake cmake -B build-web -G "Ninja" -DCMAKE_BUILD_TYPE=Release
# cmake --build build-web
# node server

# rmdir build-web /s /q & emcmake cmake -B build-web -G "Ninja" -DCMAKE_BUILD_TYPE=Release & cmake --build build-web & node server/server

cmake_minimum_required(VERSION 3.18)

include("${CMAKE_SOURCE_DIR}/UserSettings.cmake")
include("${CMAKE_SOURCE_DIR}/ProjectSettings.cmake")

if(APPLE)
    project(${PROJECT_NAME} LANGUAGES CXX OBJC OBJCXX)
else()
    project(${PROJECT_NAME})
endif()

include(${NESHNY_DIR}/CMake/WebGPU_Jumbo.txt)