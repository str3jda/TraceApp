add_executable(TraceServer WIN32)

target_compile_options(TraceServer 
    PRIVATE 
        -fno-rtti
)

target_include_directories(TraceServer PRIVATE "${PROJECT_SOURCE_DIR}/src")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"${PROJECT_SOURCE_DIR}/src/TraceServer/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/TraceServer/*.h"
    "${PROJECT_SOURCE_DIR}/src/TraceServer/**/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/TraceServer/**/*.h"
)

list(APPEND SRC_FILES ${PROJECT_SOURCE_DIR}/src/TraceServer/TraceServer.rc)

target_sources(TraceServer PRIVATE ${SRC_FILES})
target_precompile_headers(TraceServer PRIVATE stdafx.h)
target_include_directories(TraceServer PRIVATE ${PROJECT_SOURCE_DIR}/src/TraceServer)

find_package(wxWidgets CONFIG REQUIRED)
find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

target_link_libraries(TraceServer 
	PUBLIC
		TraceCommon
	PRIVATE 
		TraceCommon 
		wx::core 
		wx::base 
		wx::aui 
		eclipse-paho-mqtt-c::paho-mqtt3c-static 
		rapidjson)


install(TARGETS TraceServer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
