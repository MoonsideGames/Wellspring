cmake_minimum_required(VERSION 2.8.12)
project(Wellspring C)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(USE_SDL2 "Use SDL2" ON)

SET(LIB_MAJOR_VERSION "0")
SET(LIB_MINOR_VERSION "3")
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

if(USE_SDL2)
	add_definitions(-DUSE_SDL2)
endif()

add_library(Wellspring
	#Public header
	include/Wellspring.h
	#Source
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

# SDL2 Dependency
if(USE_SDL2)
	if (DEFINED SDL2_INCLUDE_DIRS AND DEFINED SDL2_LIBRARIES)
		message(STATUS "using pre-defined SDL2 variables SDL2_INCLUDE_DIRS and SDL2_LIBRARIES")
		target_include_directories(Wellspring PUBLIC "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
		target_link_libraries(Wellspring PUBLIC ${SDL2_LIBRARIES})
	else()
		# Only try to autodetect if both SDL2 variables aren't explicitly set
		find_package(SDL2 CONFIG)
		if (TARGET SDL2::SDL2)
			message(STATUS "using TARGET SDL2::SDL2")
			target_link_libraries(Wellspring PUBLIC SDL2::SDL2)
		elseif (TARGET SDL2)
			message(STATUS "using TARGET SDL2")
			target_link_libraries(Wellspring PUBLIC SDL2)
		else()
			message(STATUS "no TARGET SDL2::SDL2, or SDL2, using variables")
			target_include_directories(Wellspring PUBLIC "$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
			target_link_libraries(Wellspring PUBLIC ${SDL2_LIBRARIES})
		endif()
	endif()
endif()
