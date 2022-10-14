###############################################################################

# modify this
set(PROJECT_NAME "EmptyQTUnityBuild")

project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_SOURCE_DIR}/UserSettings.cmake)

# startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# puts ZERO_CHECK and ALL_BUILD into subdirectories
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###############################################################################
message(STATUS "Setting up compilation")

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt5 COMPONENTS Core Gui REQUIRED)

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
else ()
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x86")
endif ()

set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/SDL2.lib;${SDL2_LIB_DIR}/SDL2main.lib")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    external
    ${NESHNY_DIR}
    ${SDL2_DIR}/include
    ${SDL2_DIR_PATH}/include
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    ${SDL2_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")

###############################################################################
message(STATUS "Setting up file system")

configure_file(${SDL2_LIB_DIR}/SDL2.dll release/SDL2.dll COPYONLY)

configure_file(${QT_DIR}/bin/QT5Cored.dll release/QT5Cored.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Guid.dll release/QT5Guid.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Core.dll release/QT5Core.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Gui.dll release/QT5Gui.dll COPYONLY)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

###############################################################################
message(STATUS "Precompiled headers")

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    src/PreCompiledHeader.h
)

target_sources(${PROJECT_NAME}
    PRIVATE
    src/PreCompiledHeader.h
#    src/PreCompiledHeader.cpp
)
