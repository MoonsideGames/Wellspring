cmake_minimum_required(VERSION 3.5)
project(Wellspring C)
find_package(SDL3 REQUIRED)

option(BUILD_SHARED_LIBS "Build shared library" ON)

SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "1")
SET(LIB_REVISION "0")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_REVISION}")

# Build Type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	# By default, we use Release
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE
		STRING "Choose the type of build." FORCE
	)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
		STRINGS "Debug" "Release" "RelWithDebInfo"
	)
endif()

# Platform Flags
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
	set(LOBJC "objc")
elseif(WIN32)
	# "Wellspring.dll", not "libWellspring.dll"
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()
if(UNIX)
	set(CMAKE_SKIP_BUILD_RPATH TRUE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH ${BIN_RPATH})
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

add_library(Wellspring
	#Public header
	include/Wellspring.h
	#Source
	lib/json.h
	lib/stb_rect_pack.h
	lib/stb_truetype.h
	src/Wellspring.c
)

# Build flags
if(NOT MSVC)
	set_property(TARGET Wellspring PROPERTY COMPILE_FLAGS "-std=gnu99 -Wall -Wno-strict-aliasing -pedantic")
endif()

# includes
target_include_directories(Wellspring PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Soname
set_target_properties(Wellspring PROPERTIES OUTPUT_NAME "Wellspring"
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_MAJOR_VERSION}
)

# SDL3 Dependency
target_link_libraries(Wellspring
	SDL3::SDL3
	SDL3::Headers
)
