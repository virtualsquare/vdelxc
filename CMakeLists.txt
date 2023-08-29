cmake_minimum_required(VERSION 3.12)
project(vdelxc
		VERSION 0.0.1
		DESCRIPTION "vde support for containers"
		HOMEPAGE_URL "https://github.com/virtualsquare/vdelxc"
		LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckSymbolExists)

set(LIBS_REQUIRED vdeplug vdeplug_mod)
set(HEADERS_REQUIRED libvdeplug.h libvdeplug_mod.h  strcase.h)

foreach(THISLIB IN LISTS LIBS_REQUIRED)
  find_library(LIB${THISLIB}_OK ${THISLIB})
  if(NOT LIB${THISLIB}_OK)
    message(FATAL_ERROR "library lib${THISLIB} not found")
  endif()
endforeach(THISLIB)

foreach(HEADER IN LISTS HEADERS_REQUIRED)
  check_include_file(${HEADER} ${HEADER}_OK)
  if(NOT ${HEADER}_OK)
    message(FATAL_ERROR "header file ${HEADER} not found")
  endif()
endforeach(HEADER)

add_definitions(-D_GNU_SOURCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2 -pedantic -Wall -Wextra")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(vdelxcc vdelxcc.c passfd.c fdprintf.c)
install(TARGETS vdelxcc
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(vdelxcc_static vdelxcc.c passfd.c fdprintf.c)
target_link_options(vdelxcc_static PRIVATE "-static")
install(TARGETS vdelxcc_static
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(vdelxcd vdelxcd.c passfd.c fdprintf.c log.c vdeauth.c)
target_link_libraries(vdelxcd vdeplug vdeplug_mod)
install(TARGETS vdelxcd
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")
