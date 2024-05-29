#include <Trace/Backend/Backend_Pipe.h>
#include <Trace/Config.h>
#include <vector>
#include <memory>
#include <tuple>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define TRACE_PIPE_NAME "\\\\.\\pipe\\tracepipe"

namespace trace
{

	class BackendPipeImpl : public BackendPipe
	{
#if TRACE_BACKEND_LISTENER
		struct PipeClient;
#endif

	public:

		virtual void Deinit() override final
		{
			if ( m_WritePipe != INVALID_HANDLE_VALUE )
			{
				CloseHandle( m_WritePipe ); 
				m_WritePipe = INVALID_HANDLE_VALUE;
			}

#if TRACE_BACKEND_LISTENER
			
			for ( auto& cl : m_ReadClients )
			{
				DisconnectNamedPipe( cl->m_ReadPipe );
				CloseHandle( cl->m_ReadPipe ); 
			}
			
			m_ReadClients.clear();

			if ( m_ConnectPipe != INVALID_HANDLE_VALUE )
			{
				CloseHandle( m_ConnectPipe ); 
				m_ConnectPipe = INVALID_HANDLE_VALUE;
			}

			if ( m_ConnectEvent != INVALID_HANDLE_VALUE )
			{
				CloseHandle( m_ConnectEvent ); 
				m_ConnectEvent = INVALID_HANDLE_VALUE;
			}
#endif
		}

		bool CreateWritePipe()
		{
			HANDLE pipe;

			while ( true )
			{
				pipe = CreateFileA(
					TRACE_PIPE_NAME, 
					GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);

				if ( pipe != INVALID_HANDLE_VALUE ) 
					break;

				if ( GetLastError() != ERROR_PIPE_BUSY )
					return false;

				if ( !WaitNamedPipeA( TRACE_PIPE_NAME, 5000 ) )
					return false;
			}

			DWORD dwMode = PIPE_READMODE_MESSAGE; 
			if ( SetNamedPipeHandleState( pipe, &dwMode, NULL, NULL ) != TRUE )
			{
				CloseHandle( pipe );
				return false;
			}

			m_WritePipe = pipe;

			return true;
		}

		virtual void Publish( uint8_t const* _packet, uint32_t _packet_size ) override final
		{
			if ( m_WritePipe == INVALID_HANDLE_VALUE )
			{
				if ( !CreateWritePipe() )
				{
					ReportSysEvent( "Failed to create a write pipe!", TMT_Error );
					return;
				}
			}

			DWORD written;
			if ( !WriteFile( m_WritePipe, _packet, _packet_size, &written, NULL ) || ( written != _packet_size ) )
				ReportSysEvent( "Failed to publish a message!", TMT_Error );
		}

#if TRACE_BACKEND_LISTENER
		std::pair< HANDLE, bool > CreateReadPipe()
		{
			HANDLE pipe = CreateNamedPipeA(
				TRACE_PIPE_NAME,        // pipe name 
				PIPE_ACCESS_INBOUND |    // read access 
				FILE_FLAG_OVERLAPPED,    // overlapped mode 
				PIPE_TYPE_MESSAGE |      // message-type pipe 
				PIPE_READMODE_MESSAGE |  // message-read mode 
				PIPE_WAIT,               // blocking mode 
				PIPE_UNLIMITED_INSTANCES,// number of instances 
				0,                       // output buffer size 
				MAX_PACKET_SIZE,		 // input buffer size 
				0,                       // client time-out 
				NULL );                  // default security attributes 

			if ( INVALID_HANDLE_VALUE == pipe )
				return { INVALID_HANDLE_VALUE, false };

			if ( ConnectNamedPipe( pipe, &m_Overlap ) == TRUE )
			{
				CloseHandle( pipe );
				return { INVALID_HANDLE_VALUE, false };
			}

			bool pending = false;
			switch ( GetLastError() )
			{
				case ERROR_IO_PENDING:
					pending = true;
					break;

				case ERROR_PIPE_CONNECTED:
					if ( SetEvent( m_Overlap.hEvent ) )
						break;
				default:
					break;
			}

			return { pipe, pending };
		}

		virtual void Poll( uint32_t _timeout_ms ) override final
		{
			if ( m_ConnectEvent == INVALID_HANDLE_VALUE )
			{
				m_ConnectEvent = CreateEventA( NULL, TRUE, TRUE, NULL );
				m_Overlap.hEvent = m_ConnectEvent;
			}

			bool pending = false;
			if ( m_ConnectPipe == INVALID_HANDLE_VALUE )
			{
				std::tie( m_ConnectPipe, pending ) = CreateReadPipe();

				if ( m_ConnectPipe == INVALID_HANDLE_VALUE )
					ReportSysEvent( "Failed to create a read pipe for new client!", TMT_Error );
				else if ( !m_ReadyToRecv )
				{
					m_ReadyToRecv = true;
					ReportSysEvent( "Ready to receive messages", TMT_Information );
				}
			}

			switch ( WaitForSingleObjectEx( m_ConnectEvent, _timeout_ms, TRUE ) )
			{
				case WAIT_OBJECT_0:
				{
					DWORD bytesRead;
					if ( pending )
					{
						BOOL success = GetOverlappedResult(
							m_ConnectPipe,
							&m_Overlap,
							&bytesRead,
							FALSE );

						if ( !success )
							break;
					}

					std::unique_ptr< PipeClient > client = std::make_unique< PipeClient >( *this, m_ConnectPipe );
					m_ConnectPipe = INVALID_HANDLE_VALUE;

					PipeClient* client_ptr = client.get();
					m_ReadClients.push_back( std::move( client ) );

					CompletedReadRoutine( 0, 0, (LPOVERLAPPED)client_ptr );					
					break;
				}

				case WAIT_IO_COMPLETION:
					// The wait is satisfied by a completed read or write 
					// operation. This allows the system to execute the 
					// completion routine. 
					break;
				case WAIT_TIMEOUT:
					break;
			}
		}

		static void CompletedReadRoutine( DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap )
		{
			PipeClient* client = ( PipeClient* )lpOverLap;
			bool ok = false;

			if ( dwErr == 0 )
			{
				if ( cbBytesRead != 0 )
					client->m_Backend.OnNewMessage( reinterpret_cast< uint8_t const* >( &client->m_RecvBuffer[0] ), cbBytesRead );

				ok = ReadFileEx(
					client->m_ReadPipe,
					client->m_RecvBuffer,
					sizeof( client->m_RecvBuffer ),
					( LPOVERLAPPED )lpOverLap,
					( LPOVERLAPPED_COMPLETION_ROUTINE )CompletedReadRoutine ) == TRUE;
			}

			if ( !ok )
			{
				for ( auto& cl : client->m_Backend.m_ReadClients )
				{
					if ( cl.get() == client )
					{
						DisconnectNamedPipe( cl->m_ReadPipe );
						CloseHandle( cl->m_ReadPipe ); 

						BackendPipeImpl& backend = client->m_Backend;

						cl = std::move( backend.m_ReadClients.back() );
						backend.m_ReadClients.pop_back();

						break;
					}
				}
				
			}
		}

#endif 

	private:

		//
		// Write to pipe 

		HANDLE m_WritePipe = INVALID_HANDLE_VALUE;

#if TRACE_BACKEND_LISTENER
		//
		// Read from pipe

		OVERLAPPED m_Overlap;
		HANDLE m_ConnectEvent = INVALID_HANDLE_VALUE;
		HANDLE m_ConnectPipe = INVALID_HANDLE_VALUE;

		bool m_ReadyToRecv = false;

		struct PipeClient : public OVERLAPPED
		{
			PipeClient( BackendPipeImpl& _backend, HANDLE _read_pipe )
				: m_Backend( _backend )
				, m_ReadPipe( _read_pipe )
			{
				memset( this, 0, sizeof( OVERLAPPED ) );
			}

			BackendPipeImpl& m_Backend;
			HANDLE m_ReadPipe = INVALID_HANDLE_VALUE;
			uint32_t m_RecvBuffer[MAX_PACKET_SIZE];
		};

		std::vector< std::unique_ptr< PipeClient > > m_ReadClients;	

#endif

	};

	BackendPipe* BackendPipe::Create()
	{
		return new BackendPipeImpl();
	}

	extern bool initialize( Backend& _backend, TraceAppID_t _app_id, char const* _process_name, SysEventCallback_t _callback );

	TraceHandle_t initialize_pipe( TraceAppID_t _app_id, char const* _process_name, SysEventCallback_t _callback )
	{
		static BackendPipeImpl s_Impl;
		if ( !initialize( s_Impl, _app_id, _process_name, _callback ) )
			return InvalidHandle;

		return reinterpret_cast< TraceHandle_t >( &s_Impl );
	}
}
