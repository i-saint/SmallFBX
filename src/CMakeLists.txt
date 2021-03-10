CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(WebAlembicViewer)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "" FORCE)
endif()
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/_dist" CACHE PATH "" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++17")
if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_GLFW=3 -s EXIT_RUNTIME=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --bind")

    # download external libraries
    set(EXTERNALS_URL "https://github.com/i-saint/WebAlembicViewer/releases/download/data/Externals.7z")
    set(EXTERNALS_ARCHIVE "${CMAKE_SOURCE_DIR}/Externals/Externals.7z")
    if(NOT EXISTS ${EXTERNALS_ARCHIVE})
        file(DOWNLOAD ${EXTERNALS_URL} ${EXTERNALS_ARCHIVE} SHOW_PROGRESS)
        execute_process(COMMAND 7za x -y -o${CMAKE_SOURCE_DIR}/Externals ${EXTERNALS_ARCHIVE})
    endif()

    set(ext_includes
        ${CMAKE_SOURCE_DIR}/Externals/include
        ${CMAKE_SOURCE_DIR}/Externals/include/OpenEXR
    )
    file(GLOB ext_libs ${CMAKE_SOURCE_DIR}/Externals/lib_emscripten/*.a)
else()
    option(DISABLE_GL "disable OpenGL. mainly for benchmark test." OFF)
    if(DISABLE_GL)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DwabcDisableGL")
    endif()

    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
    find_package(OpenEXR REQUIRED)
    find_package(Alembic REQUIRED)
    set(ext_includes ${ALEMBIC_INCLUDE_DIRS})
    set(ext_libs ${ALEMBIC_LIBRARIES})
endif()

add_subdirectory(SmallFBX)

file(GLOB sources *.cpp *.h)
add_executable(WebAlembicViewer ${sources})
target_include_directories(WebAlembicViewer
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${ext_includes}
)
target_link_libraries(WebAlembicViewer
    PRIVATE
        SmallFBX
        ${ext_libs}
)