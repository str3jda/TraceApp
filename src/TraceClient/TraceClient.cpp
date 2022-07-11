#include "TraceClient.h"
#include <TraceCommon/Config.h>

#ifdef TS_PLATFORM_WINDOWS
#	define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#	include <windows.h>
#endif

#include <time.h>

namespace trace
{

	TraceClient::TraceClient()
		: m_Pipe( INVALID_HANDLE_VALUE )
	{
	}

	TraceClient::~TraceClient()
	{
		if ( m_Pipe != INVALID_HANDLE_VALUE )
		{
			CloseHandle(m_Pipe); 
		}
	}

	bool TraceClient::Connect( TraceAppID _srcId, char const* _app_name )
	{
		if ( !CreatePipe() )
		{
			return false;
		}

		char app_name_buffer[ MAX_PATH ];
		char const* app_name_to_send = _app_name;

#ifdef TS_PLATFORM_WINDOWS
		if ( app_name_to_send == nullptr )
		{
			GetModuleFileNameA( 0, app_name_buffer, MAX_PATH );

			app_name_to_send = app_name_buffer;

			char const* delim = strrchr( app_name_to_send, '\\' );
			if ( delim != nullptr )
			{
				app_name_to_send = delim + 1;
			}
		}
#else
#  error get app name somehow
#endif
		
		m_AppID = _srcId;
    
		return LogMessage( TMT_Greetings, app_name_to_send, nullptr, nullptr, 0 );
	}

	void TraceClient::Disconnect()
	{
#ifdef TS_PLATFORM_WINDOWS
		if ( m_Pipe != INVALID_HANDLE_VALUE )
		{
			LogMessage( TMT_Farewell, nullptr, nullptr, nullptr, 0 );
		
			CloseHandle( m_Pipe );
			m_Pipe = INVALID_HANDLE_VALUE;
		}
#endif
	}

	bool TraceClient::CreatePipe()
	{
#ifdef TS_PLATFORM_WINDOWS
		while ( true )
		{
			m_Pipe = CreateFileA(
				TRACER_PIPE_NAME, 
				GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if ( m_Pipe != INVALID_HANDLE_VALUE ) 
			{
				break;
			}

			if ( GetLastError() != ERROR_PIPE_BUSY )
			{ // Failed to create pipe
				return false;
			}

			if ( !WaitNamedPipeA( TRACER_PIPE_NAME, 5000 ) )
			{
				return false;
			}
		}

		DWORD dwMode = PIPE_READMODE_MESSAGE; 
		BOOL success = SetNamedPipeHandleState( m_Pipe, &dwMode, NULL, NULL );
		if (!success) 
		{
			return false;
		}

		return true;
#else
		return false;
#endif
	}

	void TraceClient::ReportNewThread( char const* _thread_name )
	{
		LogMessage( TMT_NewThread, _thread_name, nullptr, nullptr, 0 );
	}

	bool TraceClient::LogMessage( TracedMessageType _type, char const* _message, char const* _file, char const* _function, unsigned int _line )
	{
		if ( m_Pipe == INVALID_HANDLE_VALUE )
		{
			return false;
		}
	
		uint32_t threadId = GetCurrentThreadId();

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
    
		DWORD written;
		DWORD toWrite = (DWORD)(p - buffer);
#ifdef TS_PLATFORM_WINDOWS
		return WriteFile(m_Pipe, buffer, toWrite, &written, NULL) && written == toWrite;
#else
		return false;
#endif
	}


}