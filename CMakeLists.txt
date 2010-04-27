cmake_minimum_required( VERSION 2.6 )

if ( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )        # MacOS

   if( UNIVERSAL )
        set( CMAKE_OSX_ARCHITECTURES "i386;x86_64" )
    else()
        set( CMAKE_OSX_ARCHITECTURES "i386" )
    endif()

    add_definitions(
        -DDARWIN
    )

endif()


if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )

    add_definitions(
        -DLINUX
    )

endif()


if ( CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux" )

    add_definitions(
        -fPIC
        -pthread
    )

    if ( CMAKE_BUILD_TYPE STREQUAL "Debug" OR
         CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" )

        add_definitions(
            -g
            -ggdb
            -DDEBUG
            -Wall
            -Wno-write-strings
        )

    endif()

    if ( CMAKE_BUILD_TYPE STREQUAL "Release" OR
         CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" )

        add_definitions(
            -funroll-loops
            -fomit-frame-pointer
            -pipe
            -msse3
            -mfpmath=sse
            -O3
            #-march=prescott
        )

    endif()

elseif ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )   # Windows

    add_definitions( -DWINDOWS
        # TODO
    )

endif()


set( Boost_USE_STATIC_LIBS TRUE )
set( Boost_MULTITHREADED TRUE )
find_package( Boost 1.39 REQUIRED COMPONENTS
    filesystem
    system
    thread )


include_directories(
	include
    ${Boost_INCLUDE_DIRS} )

set( FileCache_LIB_SRCS
	src/filecache.cpp )

add_library( FileCache MODULE ${FileCache_LIB_SRCS} )

target_link_libraries( FileCache ${Boost_LIBRARIES} )

