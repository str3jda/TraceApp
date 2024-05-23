#include <Trace/Backend/Backend_Pipe.h>
#include <Trace/Config.h>

namespace trace
{

	void BackendPipe::SendMessage( 
		TracedMessageType_t _type, 
		char const* _message, 
		char const* _file, 
		char const* _function, 
		uint32_t _line,
		uint32_t _thread_id,
		uint32_t _local_time )
	{
		uint8_t buffer[ 1024 ];
		uint32_t written = Serialize( buffer, _type, _message, _file, _function, _line, _thread_id, _local_time );

		if ( written != 0 )
		{
			Publish( buffer, written );
		}
	}

#if TRACE_BACKEND_LISTENER
	bool BackendPipe::PollMessage( Message& _out, ClientId& _out_source, uint32_t _timeout )
	{
		uint8_t buffer[MAX_PACKET_SIZE];
		uint32_t packet_read = Poll( buffer, sizeof( buffer ), _timeout );

		if ( packet_read == 0 )
			return false;

		return Deserialize( buffer, packet_read, _out );
	}
#endif

	uint32_t BackendPipe::Serialize( 
		uint8_t ( &_output )[ MAX_PACKET_SIZE ], 
		TracedMessageType_t _type, 
		char const* _message, 
		char const* _file, 
		char const* _function, 
		uint32_t _line,
		uint32_t _thread_id,
		uint32_t _local_time )
	{
		constexpr size_t HeaderSize = sizeof( MsgHeader );

		size_t lenMsg	= ( _message != nullptr ) ? strlen( _message ) : 0;
		size_t lenFile	= ( _file != nullptr ) ? strlen( _file ) : 0;
		size_t lenFn	= ( _function != nullptr ) ? strlen( _function ) : 0;

		if ( HeaderSize + lenMsg + lenFile + lenFn > MAX_PACKET_SIZE )
			return 0;

		if ( lenMsg > sizeof( MsgHeader::MsgLength ) )
			return 0;

		if ( lenFile > sizeof( MsgHeader::FileLength ) )
			return 0;

		if ( lenFn > sizeof( MsgHeader::FunctionLength ) )
			return 0;

		MsgHeader header;
		header.Type = static_cast< decltype( MsgHeader::Type ) >( _type );
		header.Line = _line;
		header.Thread = _thread_id;
		header.LocalTime = _local_time;
		header.MsgLength = static_cast< decltype( MsgHeader::MsgLength ) >( lenMsg );
		header.FileLength = static_cast< decltype( MsgHeader::FileLength ) >( lenFile );
		header.FunctionLength = static_cast< decltype( MsgHeader::FunctionLength ) >( lenFn );

		uint8_t* o = &_output[0];

		std::memcpy( o, &header, HeaderSize );
		o += HeaderSize;

		if ( _message != nullptr )
		{
			std::memcpy( o, _message, header.MsgLength );
			o += header.MsgLength;
		}

		if ( _file != nullptr )
		{
			std::memcpy( o, _file, header.FileLength );
			o += header.FileLength;
		}

		if ( _function != nullptr )
		{
			std::memcpy( o, _function, header.FunctionLength );
			o += header.FunctionLength;
		}

		return static_cast< uint32_t >( o - &_output[0] );
	}

#if TRACE_BACKEND_LISTENER
	bool BackendPipe::Deserialize( uint8_t const* _packet, uint32_t _packet_size, Message& _out_message )
	{
		constexpr size_t HeaderSize = sizeof( MsgHeader );

		uint8_t const* data = _packet;

		if ( _packet_size < HeaderSize )
			return false;

		MsgHeader header;
		std::memcpy( &header, data, HeaderSize );
		data += HeaderSize;

		if ( _packet_size < HeaderSize + header.MsgLength + header.FileLength + header.FunctionLength )
			return false;

		_out_message.Type = static_cast< TracedMessageType_t >( header.Type );
		_out_message.Line = header.Line;
		_out_message.Thread = header.Thread;
		_out_message.LocalTime = header.LocalTime;

		if ( header.MsgLength > 0 )
		{
			_out_message.Msg.assign( reinterpret_cast< char const* >( data ), header.MsgLength );
			data += header.MsgLength;
		}
		else
			_out_message.Msg.clear();

		if ( header.FileLength > 0 )
		{
			_out_message.File.assign( reinterpret_cast< char const* >( data ), header.FileLength );
			data += header.FileLength;
		}
		else
			_out_message.File.clear();


		if ( header.FunctionLength > 0 )
		{
			_out_message.Function.assign( reinterpret_cast< char const* >( data ), header.FunctionLength );
			data += header.FunctionLength;
		}
		else
			_out_message.Function.clear();

		return true;
	}

#endif
}