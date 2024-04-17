#include "TraceClient.h"
#include <TraceCommon/Config.h>
#include <time.h>
#include <thread>

namespace trace
{
	void TraceClient::ReportNewThread( char const* _thread_name )
	{
		LogMessage( TMT_NewThread, _thread_name, nullptr, nullptr, 0 );
	}

	bool TraceClient::LogMessage( TracedMessageType _type, char const* _message, char const* _file, char const* _function, unsigned int _line )
	{
		if ( !m_Ready )
			return false;
	
	 	std::hash<std::thread::id> hasher;
		uint32_t threadId = hasher(std::this_thread::get_id());

		uint8_t buffer[1024];
		uint8_t* p = buffer;

		*reinterpret_cast<uint32_t*>( p ) = m_AppID;
		p += sizeof( uint32_t );
	
		*p = _type;      
		++p;

		uint32_t lenMsg		= ( _message != nullptr ) ? ( uint32_t )strlen( _message ) : 0;
		uint32_t lenFile	= ( _file != nullptr ) ? ( uint32_t )strlen( _file ) : 0;
		uint32_t lenFn		= ( _function != nullptr ) ? ( uint32_t )strlen( _function ) : 0;

		if ( _file != nullptr )
		{
			char const* delim = strrchr( _file, '\\' );
			if ( delim != nullptr )
			{
				lenFile -= static_cast< uint32_t >( ( delim - _file ) + 1 );
				_file = delim + 1;
			}
		}

		if ( ( lenMsg >= 0xffff ) || ( lenFile >= 0xff ) || ( lenFn >= 0xff ) || 
			( lenMsg  + lenFile + lenFn + 25 ) > sizeof( buffer ) )
		{
			return false;
		}
    
		*reinterpret_cast<uint16_t*>(p) = ( uint16_t )lenMsg;
		p += sizeof( uint16_t );

		*reinterpret_cast<uint8_t*>(p) = ( uint8_t )lenFile;
		p += sizeof( uint8_t );

		*reinterpret_cast<uint8_t*>(p) = ( uint8_t )lenFn;
		p += sizeof( uint8_t );
	
		if ( lenMsg > 0 )
		{
			memcpy(p, _message, lenMsg);
			p += lenMsg;
		}

		if ( lenFile > 0 )
		{
			memcpy( p, _file, lenFile );
			p += lenFile;
		}

		if ( lenFn > 0 )
		{
			memcpy( p, _function, lenFn );
			p += lenFn;
		}

		*reinterpret_cast<uint32_t*>(p) = _line;
		p += sizeof( uint32_t );

		*reinterpret_cast<uint32_t*>(p) = threadId;
		p += sizeof( uint32_t );

		uint64_t* t = reinterpret_cast<uint64_t*>(p);
		*t = 0;
		time(reinterpret_cast<time_t*>(t));
		p += sizeof( uint64_t );

		uint32_t toWrite = static_cast< uint32_t >( p - buffer );
		return Publish( buffer, toWrite );
	}


}