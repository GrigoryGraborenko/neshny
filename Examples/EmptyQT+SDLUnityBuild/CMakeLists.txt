cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

# modify this
set(PROJECT_NAME "EmptyQTUnityBuild")

include(${CMAKE_SOURCE_DIR}/UserSettings.cmake)

# set the project name
project(${PROJECT_NAME})

# startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# puts ZERO_CHECK and ALL_BUILD into subdirectories
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# message(STATUS "Setting up executable")

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt5 COMPONENTS Core Gui REQUIRED)

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
  set(SDL2_LIBRARIES "${SDL2_DIR}/lib/x64/SDL2.lib;${SDL2_DIR}/lib/x64/SDL2main.lib")
else ()
  set(SDL2_LIBRARIES "${SDL2_DIR}/lib/x86/SDL2.lib;${SDL2_DIR}/lib/x86/SDL2main.lib")
endif ()

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
