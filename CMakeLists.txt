cmake_minimum_required(VERSION 2.6)
project(d3dgl)


if(WIN32)
    add_definitions("-D_WIN32 -D_WIN32_WINNT=0x0600")

    if(MINGW AND NOT DLLTOOL)
        if(HOST)
            set(DLLTOOL "${HOST}-dlltool")
        else()
            set(DLLTOOL "dlltool")
        endif()
    endif()
endif()


include(CheckCCompilerFlag)
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

check_c_compiler_flag("-std=c99" HAVE_STD_C99)
if(HAVE_STD_C99)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()
check_c_compiler_flag("-Wall" HAVE_C_WALL_SWITCH)
if(HAVE_C_WALL_SWITCH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
check_c_compiler_flag("-Wextra" HAVE_C_WEXTRA_SWITCH)
if(HAVE_C_WEXTRA_SWITCH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()


add_library(mojoshader STATIC mojoshader/mojoshader.c
                              mojoshader/mojoshader_common.c
                              mojoshader/mojoshader.h
                              mojoshader/mojoshader_internal.h
                              mojoshader/mojoshader_version.h
)


set(LIBNAME d3d9)

include_directories("${d3dgl_SOURCE_DIR}/include" "${d3dgl_SOURCE_DIR}" "${d3dgl_BINARY_DIR}" ${OPENGL_INCLUDE_DIR})
add_definitions("-DGLEW_STATIC")

set(EXTRA_LIBS  mojoshader ${OPENGL_LIBRARIES} dxguid)
if(WIN32 AND CMAKE_COMPILER_IS_GNUCC)
    set(EXTRA_LIBS  ${EXTRA_LIBS} -static-libgcc -static-libstdc++ -Wl,--enable-stdcall-fixup)
endif()


set(HDRS  include/query.hpp
          include/swapchain.hpp
          include/rendertarget.hpp
          include/bufferobject.hpp
          include/plainsurface.hpp
          include/vertexshader.hpp
          include/pixelshader.hpp
          include/vertexdeclaration.hpp
          include/texture.hpp
          include/texture3d.hpp
          include/texturecube.hpp
          include/device.hpp
          include/adapter.hpp
          include/d3dgl.hpp
          include/glformat.hpp
          include/trace.hpp
          include/commandqueue.hpp
          include/private_iids.hpp
          include/allocators.hpp
)

set(SRCS  src/query.cpp
          src/swapchain.cpp
          src/rendertarget.cpp
          src/bufferobject.cpp
          src/plainsurface.cpp
          src/vertexshader.cpp
          src/pixelshader.cpp
          src/vertexdeclaration.cpp
          src/texture.cpp
          src/texture3d.cpp
          src/texturecube.cpp
          src/device.cpp
          src/adapter.cpp
          src/d3dgl.cpp
          src/glformat.cpp
          src/commandqueue.cpp
          main.cpp
          glew.c
)


add_library(${LIBNAME} SHARED ${SRCS} ${HDRS} d3d9.def)
target_link_libraries(${LIBNAME}  ${EXTRA_LIBS})
if(WIN32)
    set_target_properties(${LIBNAME} PROPERTIES PREFIX "")
endif()

add_executable(d3dtest  d3dtest.cpp)
