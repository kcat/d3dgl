cmake_minimum_required(VERSION 2.6)
project(d3dgl)

set(LIBNAME d3d9)

if(WIN32)
    add_definitions("-D_WIN32 -D_WIN32_WINNT=0x0502")

    if(MINGW)
        if(NOT DLLTOOL)
            if(HOST)
                set(DLLTOOL "${HOST}-dlltool")
            else()
                set(DLLTOOL "dlltool")
            endif()
        endif()
    endif()
endif()


include(CheckCXXCompilerFlag)

find_package(OpenGL REQUIRED)

check_cxx_compiler_flag("-std=c++11" HAVE_STD_CXX11)
if(HAVE_STD_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
check_cxx_compiler_flag("-Wall" HAVE_WALL_SWITCH)
if(HAVE_WALL_SWITCH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
check_cxx_compiler_flag("-Wextra" HAVE_WEXTRA_SWITCH)
if(HAVE_WEXTRA_SWITCH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
endif()


include_directories("${d3dgl_SOURCE_DIR}/include" "${d3dgl_BINARY_DIR}" ${OPENGL_INCLUDE_DIR})
add_definitions("-DGLEW_STATIC")

set(EXTRA_LIBS  ${OPENGL_LIBRARIES} dxguid)
if(WIN32 AND CMAKE_COMPILER_IS_GNUCC)
    set(EXTRA_LIBS  ${EXTRA_LIBS} -static-libgcc -static-libstdc++ -Wl,--enable-stdcall-fixup)
endif()


set(HDRS  include/d3dgl.hpp
          include/trace.hpp
)

set(SRCS  src/d3dgl.cpp
          main.cpp
          glew.c
)


add_library(${LIBNAME} SHARED ${SRCS} ${HDRS} d3d9.def)
target_link_libraries(${LIBNAME}  ${EXTRA_LIBS})
if(WIN32)
    set_target_properties(${LIBNAME} PROPERTIES PREFIX "")
endif()
