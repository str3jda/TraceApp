#pragma once
#include <Trace/Backend.h>
#include <MQTTClient.h>

#if TRACE_BACKEND_LISTENER
#	include <chrono>
#endif

/// MQTT Topics:
//  trace/info			# Info message
//  trace/log			# Log message
//  trace/warning		# Warning message
//  trace/error			# Error message
//  trace/hi			# New process signing in
//  trace/bye			# Process signout out
//  trace/thread		# Giving string name to a thread (each message only has a number ID for thread)
// 
/// MQTT Payload format
//{
//	'a': <uint32_t>  	# Unique App id
//	'm': <string>, 		# Actual message
//	'f': <string>,		# Code file, where message originates from
//	'c': <string>,		# Function, where message originates from
//	'l': <uint32_t>,	# Line number in code file
//	't': <uint32_t>,	# Thread id (use with trace::send_thread_name() to give a thread real name)
//	'i': <uint32_t>		# Local time, target app dependent (shown as simple number)
//}

namespace trace
{
	class BackendMQTT : public Backend
	{
	public:

		BackendMQTT( MQTTClient_t _existing );
		BackendMQTT( char const* _broker_address, char const* _client_id );

		virtual void Deinit() override;

		virtual void SendMessage( 
			TracedMessageType_t _type, 
			char const* _message, 
			char const* _file, 
			char const* _function, 
			uint32_t _line,
			uint32_t _thread_id,
			uint32_t _local_time ) override final;

#if TRACE_BACKEND_LISTENER
		virtual bool PollMessage( Message& _out, ClientId& _out_source, uint32_t _timeout_ms ) override final;
#endif

	protected:

		static constexpr uint32_t MAX_PAYLOAD_SIZE = 1024;
		
		struct JsonComposer;
		struct JsonParser;

	private:

		bool Connect();

		uint32_t Serialize( 
			char ( &_output )[ MAX_PAYLOAD_SIZE ], 
			char const* _message, 
			char const* _file, 
			char const* _function, 
			uint32_t _line,
			uint32_t _thread_id,
			uint32_t _local_time );

#if TRACE_BACKEND_LISTENER
		bool Deserialize( char const* _payload, uint32_t _payload_size, Message& _out_message, ClientId& _out_source );
		bool ProcessIncomingMessage( 
			char const* _topic_name, 
			int _topic_len, 
			MQTTClient_message* _message, 
			Message& _out_message,
			ClientId& _out_source );
#endif

	private:

		MQTTClient m_MqttClient = nullptr;
		bool const m_ClientOwned = false;

#if TRACE_BACKEND_LISTENER
		bool m_Listening = false;
		bool m_Connected = false;
		bool m_ReadyToRecv = false;
		
		std::chrono::time_point< std::chrono::system_clock > m_LostConnectionAt;
		std::chrono::time_point< std::chrono::system_clock > m_TryConnectAt;
#endif

	};
}