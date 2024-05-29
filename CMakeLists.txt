cmake_minimum_required(VERSION 3.3)
project(
	TraceApp 
	VERSION 1.0.0
	DESCRIPTION "Logging events from apps in a separate UI"
	LANGUAGES CXX)

set(CMAKE_DEBUG_POSTFIX d)

set(TRACE_BACKEND_LISTENER FALSE)
set(TRACE_BUILD_APP FALSE)
set(TRACE_SUPPORT_PIPE FALSE)
set(TRACE_SUPPORT_MQTT FALSE)

if ("app" IN_LIST VCPKG_MANIFEST_FEATURES)
	set(TRACE_BUILD_APP TRUE)
	set(TRACE_BACKEND_LISTENER TRUE)
	message(STATUS "[Trace] Including UI app")
endif ()

if ("pipe" IN_LIST VCPKG_MANIFEST_FEATURES)
	set(TRACE_SUPPORT_PIPE TRUE)
	message(STATUS "[Trace] Support for interprocess pipes")
endif ()

if ("mqtt" IN_LIST VCPKG_MANIFEST_FEATURES)
	set(TRACE_SUPPORT_MQTT TRUE)
	message(STATUS "[Trace] Support for MQTT")
endif ()

if (TRACE_SUPPORT_MQTT)
    find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
endif ()

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

target_compile_features(trace PRIVATE cxx_std_17)

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

    target_link_libraries(trace 
        PRIVATE 
            eclipse-paho-mqtt-c::paho-mqtt3c-static
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
    $<$<BOOL:${TRACE_BACKEND_LISTENER}>:TRACE_BACKEND_LISTENER>
)

include(GNUInstallDirs)

install(
    TARGETS trace
    EXPORT TraceTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
