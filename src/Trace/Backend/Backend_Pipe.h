#pragma once
#include <Trace/Backend.h>

namespace trace
{
	class BackendPipe : public Backend
	{
	public:

		static BackendPipe* Create();

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

		static constexpr uint32_t MAX_PACKET_SIZE = 1024;

		virtual void Publish( uint8_t const* _packet, uint32_t _packet_size ) = 0;

#if TRACE_BACKEND_LISTENER
		virtual uint32_t Poll( uint8_t* _packet, uint32_t _packet_size, uint32_t _timeout_ms ) = 0;
#endif

	private:

		struct __attribute__((__packed__)) MsgHeader
		{
			uint32_t Line;
			uint32_t Thread;
			uint32_t LocalTime;
			uint16_t MsgLength;
			uint16_t FileLength;
			uint8_t FunctionLength;
			uint8_t Type;
		};

		uint32_t Serialize( 
			uint8_t ( &_output )[ MAX_PACKET_SIZE ], 
			TracedMessageType_t _type, 
			char const* _message, 
			char const* _file, 
			char const* _function, 
			uint32_t _line,
			uint32_t _thread_id,
			uint32_t _local_time );

#if TRACE_BACKEND_LISTENER
		bool Deserialize( uint8_t const* _packet, uint32_t _packet_size, Message& _out_message );
#endif
	};
}