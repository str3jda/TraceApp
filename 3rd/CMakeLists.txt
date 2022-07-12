if(TRACE_BUILD_SERVER)

	# wxWidgets
	set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
	set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)

	add_subdirectory(${PROJECT_SOURCE_DIR}/3rd/wx)

	# PAHO - MQTT client
	set(PAHO_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(PAHO_BUILD_STATIC ON CACHE BOOL "" FORCE)
	set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
	set(PAHO_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
	set(PAHO_ENABLE_CPACK OFF CACHE BOOL "" FORCE)

	add_subdirectory(${PROJECT_SOURCE_DIR}/3rd/paho-mqtt)

	# Regex

	add_library(regex INTERFACE)
	target_include_directories(regex INTERFACE "${CMAKE_SOURCE_DIR}/3rd/regex")

	#
	# RapidJSON
	#

	add_library(rapidjson INTERFACE)
	target_include_directories(rapidjson INTERFACE "${CMAKE_SOURCE_DIR}/3rd/rapidjson/include")
	
endif()