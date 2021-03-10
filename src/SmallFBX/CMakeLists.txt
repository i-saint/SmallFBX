find_package(ZLIB REQUIRED)

file(GLOB sources *.h *.cpp)
add_library(SmallFBX STATIC ${sources})
target_include_directories(SmallFBX
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${ZLIB_INCLUDE_DIRS}
)
