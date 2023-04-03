###############################################################################

# modify this
set(PROJECT_NAME "EmptyQTJumboBuild")

project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_SOURCE_DIR}/UserSettings.cmake)

# startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# puts ZERO_CHECK and ALL_BUILD into subdirectories
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# turn off ZERO_CHECK if you want with this next line
# set(CMAKE_SUPPRESS_REGENERATION true)

###############################################################################
message(STATUS "Setting up compilation")

# add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# include files
target_include_directories(${PROJECT_NAME}
    PRIVATE
    external
    ${NESHNY_DIR}
)

# lib files
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

# ensure visual studio compiles as a windowed project not command line
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

###############################################################################
message(STATUS "Including shaders")

# adds shaders to directory structure and embeds them
# remove CONFIGURE_DEPENDS if you are getting too many cmake reruns when files in this dir change
file(GLOB_RECURSE ShaderList CONFIGURE_DEPENDS ${SHADER_PATH}/*.*)

# for release builds, we create a file called EmbeddedFiles.cpp that includes all shaders hardcoded inside it, thus embedding them in the exe.  
# this creates a near-empty project whose entire purpose is to rerun the embed script "CMakeEmbedded.txt" when the shaders change ONLY IN RELEASE BUILD
# for debug builds, the files are loaded dynamically, allowing rapid reloading during runtime
# a token file is required for this process - ".embedded_token" does nothing other than exist to signal that the embed step has run
add_custom_target(EMBEDDED_BUILD ALL DEPENDS .embedded_token)
add_custom_command(
    OUTPUT .embedded_token
    COMMAND if $<CONFIG:Release> neq 0 ("${CMAKE_COMMAND}" -P CMakeEmbedded.txt)
    DEPENDS ${SHADER_PATH}/*.vert DEPENDS ${SHADER_PATH}/*.frag DEPENDS ${SHADER_PATH}/*.comp DEPENDS ${SHADER_PATH}/*.glsl
    DEPENDS ${NESHNY_DIR}/shaders/*.vert DEPENDS ${NESHNY_DIR}/shaders/*.frag DEPENDS ${NESHNY_DIR}/shaders/*.comp DEPENDS ${NESHNY_DIR}/shaders/*.glsl
)
add_dependencies(${PROJECT_NAME} EMBEDDED_BUILD)

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

source_group(TREE src PREFIX src FILES ${FullList})
source_group(TREE ${NESHNY_DIR} PREFIX neshny FILES ${NeshnyList})
source_group("generated" FILES ${GeneratedList})

###############################################################################
message(STATUS "Setting up file system")

# copy over dlls required for execution
configure_file(${QT_DIR}/bin/QT5Cored.dll release/QT5Cored.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Guid.dll release/QT5Guid.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Widgetsd.dll release/QT5Widgetsd.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Core.dll release/QT5Core.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Gui.dll release/QT5Gui.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Widgets.dll release/QT5Widgets.dll COPYONLY)

configure_file(${QT_DIR}/plugins/platforms/qwindows.dll release/plugins/platforms/qwindows.dll COPYONLY)
configure_file(${QT_DIR}/plugins/platforms/qwindowsd.dll release/plugins/platforms/qwindowsd.dll COPYONLY)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/release)