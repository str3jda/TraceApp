#pragma once
#include <Trace/export.h>
#include <stdint.h>
#include <initializer_list>

 namespace trace
 {
	struct Message;
	class Backend;

	using TraceAppID_t = uint32_t;
	using TracedMessageType_t = uint8_t;
	using SysEventCallback_t = void (*)( trace::TracedMessageType_t _type, char const* _text );
	using TraceHandle_t = intptr_t;

	static constexpr TraceHandle_t InvalidHandle = 0;

	static TracedMessageType_t const TMT_Information		= 1; // Information - for diagnostic, updating status on things, ..
	static TracedMessageType_t const TMT_Log				= 2; // Log - similar to Information, but these are more important, possibly even archived 
	static TracedMessageType_t const TMT_Warning			= 3; // Warning - non critical issue raised
	static TracedMessageType_t const TMT_Error				= 4; // Error - failure in app, should be investigated and fixed

	static constexpr std::initializer_list< TracedMessageType_t > AvailableTypes = 
	{  
		TMT_Information,
		TMT_Log,
		TMT_Warning,
		TMT_Error
	};

#if TRACE_SUPPORT_MQTT
	using MQTTClient_t = void*; // eclipse-paho-mqtt-c instance

	// Initializes trace library with new MQTT connection
	TRACE_EXPORT TraceHandle_t initialize_mqtt( 
		TraceAppID_t _app_id, 
		char const* _broker_address, 
		char const* _client_id, 
		char const* _process_name = nullptr, 
		SysEventCallback_t _sys_event_callback = nullptr );

	// Initializes trace library with pre-existing MQTT connection
	TRACE_EXPORT TraceHandle_t initialize_mqtt( 
		TraceAppID_t _app_id, 
		MQTTClient_t _existing, 
		char const* _process_name = nullptr, 
		SysEventCallback_t _sys_event_callback = nullptr );
#endif

#ifdef TRACE_SUPPORT_PIPE
	// Initializes trace library with IPC pipe connection (only works if client is on the same machine)
	TRACE_EXPORT TraceHandle_t initialize_pipe( TraceAppID_t _app_id, char const* _process_name = nullptr );
#endif

	TRACE_EXPORT void deinitialize( TraceHandle_t _handle );

	// Explicitly give thread a name, which app will show in its column
	TRACE_EXPORT void send_thread_name( TraceHandle_t _handle, uint32_t _thread_id, char const* _thread_name );

	TRACE_EXPORT void send(
		TraceHandle_t _handle,
		TracedMessageType_t _type, 
		char const* _message, 
		char const* _file = nullptr, 
		char const* _function = nullptr, 
		uint32_t _line = 0,
		uint32_t _thread_id = 0,
		uint32_t _local_time = 0 );

	
 }