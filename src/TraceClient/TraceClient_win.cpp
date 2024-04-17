#include "TraceClient.h"
#include <TraceCommon/Config.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

namespace trace
{
	class TraceClientImpl : public TraceClient
	{
	public:

		~TraceClientImpl()
		{
			if ( m_Pipe != INVALID_HANDLE_VALUE )
			{
				CloseHandle(m_Pipe); 
			}
		}
		
	private:

		bool Connect( TraceAppID _srcId, char const* _app_name ) override
		{
			if ( !CreatePipe() )
			{
				return false;
			}

			char app_name_buffer[ MAX_PATH ];
			char const* app_name_to_send = _app_name;

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
			
			m_AppID = _srcId;
			m_Ready = true;
		
			return LogMessage( TMT_Greetings, app_name_to_send, nullptr, nullptr, 0 );
		}

		void Disconnect() override
		{
			if ( m_Pipe != INVALID_HANDLE_VALUE )
			{
				LogMessage( TMT_Farewell, nullptr, nullptr, nullptr, 0 );
			
				CloseHandle( m_Pipe );
				m_Pipe = INVALID_HANDLE_VALUE;
			}

			m_Ready = false;
		}

		bool CreatePipe()
		{
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
		}

		bool Publish( uint8_t const* _packet_data, uint32_t _packet_data_size ) override
		{
			DWORD written;
			return WriteFile(m_Pipe, _packet_data, _packet_data_size, &written, NULL) && ( written == _packet_data_size );
		}

	 private:

		 HANDLE m_Pipe;

	} g_TraceInstance;

	TraceClient& TraceClient::GetInstance()
	{
		return g_TraceInstance;
	}
}