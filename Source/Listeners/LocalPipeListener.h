#pragma once

#include <unordered_map>
#include <thread>

class C_MessagePump;
struct S_Message;

class C_LocalPipeListener
{
public:

	bool    Init( C_MessagePump* msgPump );
	void    Done();

private:

	struct S_Client : public OVERLAPPED
	{
		static size_t const BUFFER_SIZE = 2048;

		S_Client( C_MessagePump& msgPump, C_LocalPipeListener& owner, HANDLE pipe )
			: m_MsgPump( msgPump )
			, m_Owner( owner )
			, m_PipeInst( pipe )
		{
			memset( this, 0, sizeof( OVERLAPPED ) );
			m_Buffer = ( uint8_t* )malloc( S_Client::BUFFER_SIZE );
		}

		~S_Client()
		{
			CloseHandle( m_PipeInst );
			free( m_Buffer );
		}

		HANDLE m_PipeInst;
		uint8_t* m_Buffer = nullptr;
		C_MessagePump& m_MsgPump;
		C_LocalPipeListener& m_Owner;

	};

	bool ProcessMessage( S_Client* inClient, uint8_t const* inMsgData, uint32_t inMsgDataSize );

	void ThreadProc();
	std::pair< HANDLE, bool > CreateAndConnectPipe();

	void ClientDisconnects( S_Client* client );

	static void WINAPI CompletedReadRoutine( DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap );

private:

	typedef std::vector< S_Client* > T_Clients;

	T_Clients m_Clients;
	HANDLE m_Event;
	OVERLAPPED m_Overlap;

	C_MessagePump* m_MsgPump = nullptr;

	std::thread m_Thread;
	bool m_Exit = false;

};
