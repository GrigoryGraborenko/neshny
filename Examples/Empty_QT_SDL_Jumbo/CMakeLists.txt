###############################################################################
# example: cmake -G "Visual Studio 16 2019" -A x64 .

cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
include("${CMAKE_SOURCE_DIR}/UserSettings.cmake")
include("${CMAKE_SOURCE_DIR}/ProjectSettings.cmake")
project(${PROJECT_NAME})

# startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# puts ZERO_CHECK and ALL_BUILD into subdirectories
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(BASE_DIR ${CMAKE_SOURCE_DIR})

# turn off ZERO_CHECK if you want with this next line
# set(CMAKE_SUPPRESS_REGENERATION true)
add_compile_definitions(NESHNY_DIR="${NESHNY_DIR}" PROJECT_SHADER_PATH="${SHADER_PATH}")

###############################################################################
message(STATUS "Setting up compilation")

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt6 COMPONENTS Core Gui REQUIRED)

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
else ()
    set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x86")
endif ()

set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/SDL2.lib;${SDL2_LIB_DIR}/SDL2main.lib")

# include files
target_include_directories(${PROJECT_NAME}
    PRIVATE
    external
    src
    ${NESHNY_DIR}
    ${SDL2_DIR}/include
    ${SDL2_MIXER_DIR}/include
)

# lib files
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    ${SDL2_LIBRARIES}
)

# ensure visual studio compiles as a windowed project not command line
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")

###############################################################################
message(STATUS "Embedding shaders and unit tests")

# adds shaders to directory structure and embeds them
# remove CONFIGURE_DEPENDS if you are getting too many cmake reruns when files in this dir change
file(GLOB_RECURSE ShaderList CONFIGURE_DEPENDS ${SHADER_PATH}/*.*)

# this creates a near-empty project whose entire purpose is to rerun the embed script "CMakeEmbedded.txt" when the shaders change ONLY IN RELEASE BUILD
# it also runs "CMakeUnitTests.txt" when unit tests change
# a token file is required for this process - ".embedded_token" does nothing other than exist to signal that the embed step has run
add_custom_target(EMBEDDED_BUILD ALL DEPENDS ${CMAKE_BINARY_DIR}/.embedded_token ${CMAKE_SOURCE_DIR}/src/UnitTestList.cpp)

# for release builds, we create a file called EmbeddedFiles.cpp that includes all shaders hardcoded inside it, thus embedding them in the exe.  
# for debug builds, the files are loaded dynamically, allowing rapid reloading during runtime
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/.embedded_token
    COMMAND if $<CONFIG:Release> neq 0 ("${CMAKE_COMMAND}" -D BASE_DIR=${CMAKE_SOURCE_DIR} -P ${NESHNY_DIR}/CMake/CMakeEmbedded.txt)
    DEPENDS ${SHADER_PATH}/*.vert DEPENDS ${SHADER_PATH}/*.frag DEPENDS ${SHADER_PATH}/*.comp DEPENDS ${SHADER_PATH}/*.glsl
    DEPENDS ${NESHNY_DIR}/shaders/*.vert DEPENDS ${NESHNY_DIR}/shaders/*.frag DEPENDS ${NESHNY_DIR}/shaders/*.comp DEPENDS ${NESHNY_DIR}/shaders/*.glsl
)

# command that embeds list of unit tests and their names into a cpp file
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/src/UnitTestList.cpp
    COMMAND "${CMAKE_COMMAND}" -D BASE_DIR=${CMAKE_SOURCE_DIR} -P ${NESHNY_DIR}/CMake/CMakeUnitTests.txt
    DEPENDS ${NESHNY_DIR}/UnitTests/*.cpp
    DEPENDS ${CMAKE_SOURCE_DIR}/src/UnitTests/*.cpp
)

# ensure this all happens before the main build
add_dependencies(${PROJECT_NAME} EMBEDDED_BUILD)

# adds list of shader directories to allow dynamic runtime loading in debug
file(WRITE "src/EmbeddedDirectories.cpp" "std::vector<std::string> g_ShaderBaseDirs={ \"${NESHNY_DIR}/shaders\", \"${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_PATH}\" };\n")

###############################################################################
message(STATUS "Project Structure")

# precompiled header setup
target_precompile_headers(${PROJECT_NAME}
    PUBLIC
    src/PreCompiledHeader.h
)

# remove CONFIGURE_DEPENDS if you are getting too many cmake reruns when files in this dir change
file(GLOB_RECURSE HeaderList CONFIGURE_DEPENDS src/*.h)
file(GLOB_RECURSE SourceList CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE GeneratedList *.cxx *.hxx)

# all files from neshny
file(GLOB NeshnyList ${NESHNY_DIR}/*.h ${NESHNY_DIR}/*.cpp ${NESHNY_DIR}/UnitTests/*.cpp ${NESHNY_DIR}/shaders/*.*)

list(APPEND FullList ${HeaderList} ${SourceList} ${ShaderList})

# jumbo build specific - this makes sure all .cpp files are not compiled unless included in main.cpp
set_source_files_properties(${SourceList} ${NeshnyList} PROPERTIES
    HEADER_FILE_ONLY TRUE
)
# jumbo build specific - the only two files that get compiled
set_source_files_properties(src/main.cpp src/PreCompiledHeader.cpp PROPERTIES
    HEADER_FILE_ONLY FALSE
)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${HeaderList}
    ${SourceList}
    ${NeshnyList}
    ${ShaderList}
)

source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX src FILES ${FullList})
source_group(TREE ${NESHNY_DIR} PREFIX neshny FILES ${NeshnyList})
source_group("generated" FILES ${GeneratedList})

###############################################################################
message(STATUS "Setting up file system")

# copy over dlls required for execution
configure_file(${SDL2_LIB_DIR}/SDL2.dll release/SDL2.dll COPYONLY)

configure_file(${QT_DIR}/bin/QT6Cored.dll release/QT6Cored.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT6Guid.dll release/QT6Guid.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT6Core.dll release/QT6Core.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT6Gui.dll release/QT6Gui.dll COPYONLY)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/release)