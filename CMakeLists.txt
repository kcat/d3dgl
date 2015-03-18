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
    add_definitions("-std=c++11")
endif()
check_cxx_compiler_flag("-Wall" HAVE_WALL_SWITCH)
if(HAVE_WALL_SWITCH)
    add_definitions("-Wall")
endif()
check_cxx_compiler_flag("-Wextra" HAVE_WEXTRA_SWITCH)
if(HAVE_WEXTRA_SWITCH)
    add_definitions("-Wextra")
endif()


include_directories("${d3dgl_SOURCE_DIR}/include" "${d3dgl_BINARY_DIR}")

add_library(${LIBNAME} SHARED main.cpp)

if(WIN32)
    set_target_properties(${LIBNAME} PROPERTIES PREFIX "")

    if(MINGW)
        find_program(SED_EXECUTABLE NAMES sed DOC "sed executable")
        find_program(DLLTOOL_EXECUTABLE NAMES "${DLLTOOL}" DOC "dlltool executable")
        if(NOT SED_EXECUTABLE OR NOT DLLTOOL_EXECUTABLE)
            message(STATUS "")
            if(NOT SED_EXECUTABLE)
                message(STATUS "WARNING: Cannot find sed, disabling .def/.lib generation")
            endif()
            if(NOT DLLTOOL_EXECUTABLE)
                message(STATUS "WARNING: Cannot find dlltool, disabling .def/.lib generation")
            endif()
        else()
            set_target_properties(${LIBNAME} PROPERTIES LINK_FLAGS "-Wl,--output-def,${LIBNAME}.def")
            add_custom_command(TARGET ${LIBNAME} POST_BUILD
                COMMAND "${SED_EXECUTABLE}" -i -e "s/ @[^ ]*//" ${LIBNAME}.def
                COMMAND "${DLLTOOL_EXECUTABLE}" -d ${LIBNAME}.def -l ${LIBNAME}.lib -D ${LIBNAME}.dll
                COMMENT "Stripping ordinals from ${LIBNAME}.def and generating ${LIBNAME}.lib..."
                VERBATIM
            )
        endif()
    endif()
endif()
