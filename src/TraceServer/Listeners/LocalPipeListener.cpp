#include "stdafx.h"
#include "LocalPipeListener.h"
#include "../Messages/Message.h"
#include "../Messages/MessagePump.h"
#include <TraceCommon/Config.h>

//--------------------------------------------------------------------------------------------------------------------------

bool C_LocalPipeListener::Init( C_MessagePump* msgPump )
{
	m_MsgPump = msgPump;
	m_Event = CreateEventA( nullptr, true, true, nullptr );
	m_Overlap.hEvent = m_Event;

	m_Thread = std::thread( std::bind( &C_LocalPipeListener::ThreadProc, this ) );

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------

void C_LocalPipeListener::Done()
{
	m_Exit = true;
	m_Thread.join();
	m_Thread = std::thread();

	CloseHandle( m_Event );

	for ( uint32_t i = 0; i < m_Clients.size(); ++i )
	{
		delete m_Clients[i];
	}

	m_MsgPump = nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------

void C_LocalPipeListener::ClientDisconnects( S_Client* client )
{
	auto it = std::find( m_Clients.begin(), m_Clients.end(), client );
	if ( it != m_Clients.end() )
	{
		delete *it;

		*it = m_Clients.back();
		m_Clients.pop_back();
	}
}

//--------------------------------------------------------------------------------------------------------------------------

void WINAPI C_LocalPipeListener::CompletedReadRoutine( DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap )
{
	S_Client* client = static_cast< S_Client* >( lpOverLap );
	bool ok = false;

	if ( ( dwErr == 0 ) && ( cbBytesRead != 0 ) )
	{
		T_SourceID srcID = *( T_SourceID* )client->m_Buffer;
		client->m_MsgPump.PushNewMessage( S_Message::Deserialize( srcID, client->m_Buffer + sizeof( T_SourceID ), ( size_t )cbBytesRead - sizeof( T_SourceID ) ) );

		ok = ReadFileEx(
			client->m_PipeInst,
			client->m_Buffer,
			S_Client::BUFFER_SIZE,
			( LPOVERLAPPED )lpOverLap,
			( LPOVERLAPPED_COMPLETION_ROUTINE )CompletedReadRoutine ) == TRUE;
	}

	if ( !ok )
	{ // client left us apparently :/

		DisconnectNamedPipe( client->m_PipeInst );

		client->m_Owner.ClientDisconnects( client );
	}
}

//--------------------------------------------------------------------------------------------------------------------------

std::pair< HANDLE, bool > C_LocalPipeListener::CreateAndConnectPipe()
{
	HANDLE pipe = CreateNamedPipeA(
		TRACER_PIPE_NAME,        // pipe name 
		PIPE_ACCESS_INBOUND |    // read access 
		FILE_FLAG_OVERLAPPED,    // overlapped mode 
		PIPE_TYPE_MESSAGE |      // message-type pipe 
		PIPE_READMODE_MESSAGE |  // message-read mode 
		PIPE_WAIT,               // blocking mode 
		PIPE_UNLIMITED_INSTANCES,// number of instances 
		0,                       // output buffer size 
		16 * sizeof( S_Message ),  // input buffer size 
		0,                       // client time-out 
		NULL );                   // default security attributes 

	if ( INVALID_HANDLE_VALUE == pipe )
	{
		return std::make_pair( INVALID_HANDLE_VALUE, false );
	}

	bool pending = ConnectNamedPipe( pipe, &m_Overlap ) == TRUE;
	if ( pending )
	{
		CloseHandle( pipe );
		return std::make_pair( INVALID_HANDLE_VALUE, false );
	}

	switch ( GetLastError() )
	{
	case ERROR_IO_PENDING:
		pending = true;
		break;

	case ERROR_PIPE_CONNECTED:
		if ( SetEvent( m_Overlap.hEvent ) )
			break;
	}

	return std::make_pair( pipe, pending );
}

//--------------------------------------------------------------------------------------------------------------------------

void C_LocalPipeListener::ThreadProc()
{
	HANDLE pipe;
	bool pending;
	std::tie( pipe, pending ) = CreateAndConnectPipe();

	if ( pipe == INVALID_HANDLE_VALUE )
	{ // Hmmm
		return;
	}

	while ( !m_Exit )
	{
		switch ( WaitForSingleObjectEx( m_Event, 60, true ) )
		{
			case WAIT_OBJECT_0:
			{
				DWORD bytesRead;
				if ( pending )
				{
					BOOL success = GetOverlappedResult(
						pipe,     // pipe handle 
						&m_Overlap, // OVERLAPPED structure 
						&bytesRead,    // bytes transferred 
						false );    // does not wait 

					if ( !success )
					{
						break;
					}
				}

				S_Client* client = new S_Client( *m_MsgPump, *this, pipe );
				m_Clients.push_back( client );

				BOOL succ = ReadFileEx(
					client->m_PipeInst,
					client->m_Buffer,
					S_Client::BUFFER_SIZE,
					client,
					( LPOVERLAPPED_COMPLETION_ROUTINE )CompletedReadRoutine );

				std::tie( pipe, pending ) = CreateAndConnectPipe();

				if ( pipe == INVALID_HANDLE_VALUE )
				{ // Hmmm
					return;
				}

				break;
			}

		case WAIT_IO_COMPLETION:
			break;

		case WAIT_TIMEOUT:
			break;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------
