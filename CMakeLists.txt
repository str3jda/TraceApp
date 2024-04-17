cmake_minimum_required(VERSION 3.3)
project(TraceApp VERSION 1.0)

option(TRACE_BUILD_SERVER "Build server window app" OFF)

# TODO: Install goes local, I don't want to handle all the admin-rights nuisanse
# TODO: Paho MQTT is dumb and won't allow me to stop his install of useless stuff
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/bin CACHE BOOL "" FORCE)

add_subdirectory("src/TraceCommon")
add_subdirectory("src/TraceClient")

if(TRACE_BUILD_SERVER)
	add_subdirectory("src/TraceServer")
endif()