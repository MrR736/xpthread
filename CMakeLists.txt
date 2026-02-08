cmake_minimum_required(VERSION 3.20)
project(xpthread VERSION 1.0.0 LANGUAGES C CXX)

# ------------------------------------
# Verbose & helper options
# ------------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------
# Project options
# ------------------------------------
option(XPTHREAD_BUILD_SHARED "Build xpthread as shared library" ON)


if (XPTHREAD_BUILD_SHARED)
	add_library(xpthread SHARED ${CMAKE_SOURCE_DIR}/src/xpthread.c)
else()
	add_library(xpthread STATIC ${CMAKE_SOURCE_DIR}/src/xpthread.c)
endif()

if (WIN32)
	set(CMAKE_STATIC_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_IMPORT_LIBRARY_PREFIX "")
	target_link_options(xpthread PRIVATE -static -static-libgcc -static-libstdc++)
endif()

target_include_directories(xpthread
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

if (NOT WIN32)
	target_link_libraries(xpthread PUBLIC pthread)
endif()


add_executable(xpthread_tests ${CMAKE_SOURCE_DIR}/test/xpthread_tests.c)
target_link_libraries(xpthread_tests PUBLIC xpthread)
