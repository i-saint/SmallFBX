find_package(ZLIB REQUIRED)

file(GLOB sources *.h *.cpp)
add_library(SmallFBX STATIC ${sources})
target_include_directories(SmallFBX
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${ZLIB_INCLUDE_DIRS}
)

# install library
install(TARGETS SmallFBX)

# install headers with keeping directory structures
get_filename_component(base_dir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
file(GLOB install_headers ${base_dir}/*.h *.h)
foreach(f pch.h sfbxInternal.h) # remove these
    list(REMOVE_ITEM install_headers ${CMAKE_CURRENT_SOURCE_DIR}/${f})
endforeach(f)
foreach(f ${install_headers})
    string(REPLACE ${base_dir} "" sf ${f})
    get_filename_component(d ${sf} DIRECTORY)
    install(FILES ${f} DESTINATION ${CMAKE_INSTALL_PREFIX}/include${d})
endforeach(f)
