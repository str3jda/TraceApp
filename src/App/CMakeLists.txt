add_executable(app WIN32)

set_target_properties(app PROPERTIES OUTPUT_NAME "TraceApp")

target_compile_features(app PUBLIC cxx_std_20)

target_compile_definitions(app 
    PRIVATE
        $<$<BOOL:${TRACE_APP_THEME_BLACK}>:BLACK_THEME>
		$<$<BOOL:${TRACE_BACKEND_LISTENER}>:TRACE_BACKEND_LISTENER>
)

target_include_directories(app PRIVATE "${PROJECT_SOURCE_DIR}/src")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"${PROJECT_SOURCE_DIR}/src/App/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/App/*.h"
    "${PROJECT_SOURCE_DIR}/src/App/**/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/App/**/*.h"
)

list(APPEND SRC_FILES ${PROJECT_SOURCE_DIR}/src/App/resource.rc)

target_sources(app PRIVATE ${SRC_FILES})
target_precompile_headers(app PRIVATE stdafx.h)
target_include_directories(app 
	PRIVATE 
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/include
)

CPMAddPackage(
	NAME wxWidgets
	GITHUB_REPOSITORY wxWidgets/wxWidgets
	GIT_TAG v3.2.5
	OPTIONS
		"wxBUILD_SHARED OFF"
)

target_link_libraries(app 
	PUBLIC
		trace
	PRIVATE 
		wx::core 
		wx::base 
		wx::aui
		wx::richtext)


install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
