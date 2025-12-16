cmake_minimum_required(VERSION 3.5)

include("cmake/cpm.cmake")
include(CMakeDependentOption)

option(TRACE_BUILD_APP "[Trace] Including UI app" OFF)
option(TRACE_SUPPORT_PIPE "[Trace] Support for interprocess pipes" OFF)
option(TRACE_SUPPORT_MQTT "[Trace] Support for MQTT" ON)

cmake_dependent_option(TRACE_USE_POSTFIX
  "Use postfix for each configuration to make sure you can install multiple build outputs" OFF
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

set(TRACE_BACKEND_LISTENER false)

if (TRACE_BUILD_APP)
    message( "[Trace] Including UI app" )
    set(TRACE_BACKEND_LISTENER true)
endif ()

if (TRACE_SUPPORT_PIPE)
    message( "[Trace] Support for interprocess pipes" )
endif ()

if (TRACE_SUPPORT_MQTT)
    message( "[Trace] Support for MQTT" )
endif ()

project(
	TraceApp 
	VERSION 1.0.2
	DESCRIPTION "Logging events from apps in a separate UI"
	LANGUAGES CXX)

if (TRACE_USE_POSTFIX)
    set(CMAKE_RELWITHDEBINFO_POSTFIX r)
    set(CMAKE_MINSIZEREL_POSTFIX m)
    set(CMAKE_DEBUG_POSTFIX d)
endif()

add_library(trace)
add_library(trace::trace ALIAS trace)

set_target_properties(
    trace
    PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_options(trace 
    PRIVATE 
        -Wall 
        #-Werror
)

target_compile_features(trace PRIVATE cxx_std_20)

include(GenerateExportHeader)
set_target_properties(trace 
    PROPERTIES 
        CXX_VISIBILITY_PRESET "hidden"
        VISIBILITY_INLINES_HIDDEN TRUE
)

generate_export_header(
    trace
    EXPORT_FILE_NAME
    export/Trace/export.h
)

target_include_directories(trace 
    PRIVATE
        src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(trace 
    PRIVATE 
        src/Trace/Trace.cpp
)

if (TRACE_SUPPORT_MQTT)
    target_sources(trace PRIVATE src/Trace/Backend/Backend_MQTT.cpp)
    target_compile_definitions(trace PUBLIC TRACE_SUPPORT_MQTT)
    
    CPMAddPackage(
        NAME paho-mqtt
        GITHUB_REPOSITORY eclipse/paho.mqtt.c
        GIT_TAG v1.3.15
        OPTIONS
            "PAHO_BUILD_STATIC ON"
            "PAHO_BUILD_SHARED OFF"
            "PAHO_ENABLE_TESTING OFF"
    )

    target_compile_options(paho-mqtt3a-static PRIVATE -w)
    target_compile_options(paho-mqtt3c-static PRIVATE -w)

    target_link_libraries(trace 
        PUBLIC 
            paho-mqtt3c-static
    )
endif ()

if (TRACE_SUPPORT_PIPE)
    target_sources(trace 
        PRIVATE 
            src/Trace/Backend/Backend_Pipe.cpp
            $<$<PLATFORM_ID:Windows>:src/Trace/Backend/Backend_Pipe_win.cpp>
    )
    target_compile_definitions(trace PUBLIC TRACE_SUPPORT_PIPE)
endif ()

# Platform specific macros
target_compile_definitions(trace PRIVATE
    $<$<PLATFORM_ID:Windows>:TS_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:TS_PLATFORM_LINUX>
    TRACE_BACKEND_LISTENER=${TRACE_BACKEND_LISTENER}
)

include(GNUInstallDirs)

install(
    TARGETS trace
    EXPORT TraceTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#            COMPONENT Trace_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT Trace_Runtime
#            NAMELINK_COMPONENT Trace_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#            COMPONENT Trace_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#            COMPONENT Trace_Development
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install the generated export header
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/export/Trace/export.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Trace
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "TraceConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/TraceConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TraceConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/trace/cmake
)

install(
    FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/TraceConfig.cmake 
        ${CMAKE_CURRENT_BINARY_DIR}/TraceConfigVersion.cmake
    DESTINATION 
        ${CMAKE_INSTALL_DATAROOTDIR}/trace/cmake)

# install the CMake targets
install(
    EXPORT TraceTargets
    FILE TraceTargets.cmake
    NAMESPACE trace::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/trace/cmake
)

if (TRACE_BUILD_APP)
	add_subdirectory("src/App")
endif()
