project(988unpack LANGUAGES CXX)

cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_library(common STATIC stb_image.cpp compress.cpp)

add_executable(998pack 998pack.cpp)
add_executable(998unpack 998unpack.cpp)
target_link_libraries(998pack common)
target_link_libraries(998unpack common)

if(MSVC)
	ADD_CUSTOM_COMMAND(
		TARGET 998pack
		POST_BUILD
		DEPENDS
		COMMAND mt.exe -manifest "${CMAKE_SOURCE_DIR}/998pack.manifest.xml" -inputresource:"$<TARGET_FILE:998pack>"\;\#1 -updateresource:"$<TARGET_FILE:998pack>"\;\#1
	)
ENDIF(MSVC)

if(MSVC)
	ADD_CUSTOM_COMMAND(
		TARGET 998pack
		POST_BUILD
		DEPENDS
		COMMAND mt.exe -manifest "${CMAKE_SOURCE_DIR}/998unpack.manifest.xml" -inputresource:"$<TARGET_FILE:998unpack>"\;\#1 -updateresource:"$<TARGET_FILE:998unpack>"\;\#1
	)
ENDIF(MSVC)