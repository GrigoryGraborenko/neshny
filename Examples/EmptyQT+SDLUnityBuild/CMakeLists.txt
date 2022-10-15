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
# turn off ZERO_CHECK if you want with this next line
# set(CMAKE_SUPPRESS_REGENERATION true)

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
message(STATUS "Including shaders")

# Creates C resources file from files in given directory
function(create_resources dir output_fname)
    # Create empty output file
    file(WRITE ${output_fname} "")
    # Collect input files
    file(GLOB bins ${dir}/*)

    set(resource_vector "struct EmbeddedFile{ std::string p_FileName=\"\"\\; unsigned long long p_Size=0\\; const unsigned char* p_Data=nullptr\\; }\\;\n std::vector<EmbeddedFile> g_EmbeddedFiles = {\n")

    # Iterate through input files
    foreach(bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" c_filename ${filename})
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output_fname} "const unsigned char _g_${c_filename}[] = {${filedata}};\n")

        string(APPEND resource_vector "{ \"${filename}\", sizeof(_g_${c_filename}), _g_${c_filename} },\n")
    endforeach()
    string(APPEND resource_vector "{}}\\;\n")
    file(APPEND ${output_fname} ${resource_vector})
endfunction()

create_resources(${NESHNY_DIR}/shaders "src/EmbeddedFiles.cpp")

###############################################################################
message(STATUS "Project Structure")

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    src/PreCompiledHeader.h
)

file(GLOB_RECURSE HeaderList src/*.h)
file(GLOB_RECURSE SourceList src/*.cpp)
file(GLOB_RECURSE GeneratedList *.cxx *.hxx)

file(GLOB NeshnyList ${NESHNY_DIR}/*.h ${NESHNY_DIR}/*.cpp ${NESHNY_DIR}/shaders/*.*)

list (APPEND FullList ${HeaderList} ${SourceList})

set_source_files_properties(${SourceList} PROPERTIES
    HEADER_FILE_ONLY TRUE
)
set_source_files_properties(${NeshnyList} PROPERTIES
    HEADER_FILE_ONLY TRUE
)
set_source_files_properties(src/main.cpp PROPERTIES
    HEADER_FILE_ONLY FALSE
)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${HeaderList}
    ${SourceList}
    ${NeshnyList}
)

source_group(TREE src PREFIX src FILES ${FullList})
source_group(TREE ${NESHNY_DIR} PREFIX neshny FILES ${NeshnyList})
source_group("generated" FILES ${GeneratedList})

###############################################################################
message(STATUS "Setting up file system")

configure_file(${SDL2_LIB_DIR}/SDL2.dll release/SDL2.dll COPYONLY)

configure_file(${QT_DIR}/bin/QT5Cored.dll release/QT5Cored.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Guid.dll release/QT5Guid.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Core.dll release/QT5Core.dll COPYONLY)
configure_file(${QT_DIR}/bin/QT5Gui.dll release/QT5Gui.dll COPYONLY)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/release)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)