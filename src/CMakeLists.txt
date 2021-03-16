CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(SmallFBX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++17")
if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --bind")

    # download external libraries
    set(EXTERNALS_URL "https://github.com/i-saint/SmallFBX/releases/download/data/Externals.7z")
    set(EXTERNALS_ARCHIVE "${CMAKE_SOURCE_DIR}/Externals/Externals.7z")
    if(NOT EXISTS ${EXTERNALS_ARCHIVE})
        file(DOWNLOAD ${EXTERNALS_URL} ${EXTERNALS_ARCHIVE} SHOW_PROGRESS)
        execute_process(COMMAND 7za x -y -o${CMAKE_SOURCE_DIR}/Externals ${EXTERNALS_ARCHIVE})
    endif()

    set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Externals/include)
    set(ZLIB_LIBRARY_DEBUG ${CMAKE_SOURCE_DIR}/Externals/lib_emscripten/libz.a)
    set(ZLIB_LIBRARY_RELEASE ${CMAKE_SOURCE_DIR}/Externals/lib_emscripten/libz.a)
else()
endif()

option(ENABLE_TEST "Enable test" ON)

add_subdirectory(SmallFBX)
if (ENABLE_TEST)
    add_subdirectory(Test)
endif()
