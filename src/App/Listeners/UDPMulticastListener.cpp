#if 0
#include <App/stdafx.h>
#include "UDPMulticastListener.h"

#include "../Messages/MessagePump.h"
#include "../Messages/Message.h"


//#include <winsock2.h>
#include <ws2ipdef.h>

#pragma comment(lib, "wsock32.lib") 


C_UDPMulticastListener::C_UDPMulticastListener()
	: m_Socket( INVALID_SOCKET )
{
}

bool C_UDPMulticastListener::Init( C_MessagePump* msgPump )
{
	m_MsgPump = msgPump;

	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if ( iResult != 0 )
	{
		return false;
	}

	m_Socket = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( m_Socket == INVALID_SOCKET )
	{
		WSACleanup();
		return false;
	}
	
 	char broadcast = '1';

	if ( setsockopt( m_Socket, SOL_SOCKET, SO_REUSEADDR, &broadcast, 4 ) < 0 )
	{
		closesocket( m_Socket );
		WSACleanup();
		return false;
	}

	u_long nMode = FALSE; // 0: BLOCKING
	if ( ioctlsocket( m_Socket, FIONBIO, &nMode ) == SOCKET_ERROR )
	{
		closesocket( m_Socket );
		WSACleanup();
		return false;
	}

	SOCKADDR_IN server;
	memset( &server, 0, sizeof( server ) );
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( TRACE_MULTICAST_PORT );

	if ( bind( m_Socket, ( struct sockaddr * )&server, sizeof( server ) ) < 0 )
	{
		closesocket( m_Socket );
		WSACleanup();
		return false;
	}

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr( TRACE_MULTICAST_IP );
	mreq.imr_interface.s_addr = INADDR_ANY;
	if ( setsockopt( m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, ( char const* )&mreq, sizeof( mreq ) ) < 0 )
	{
		closesocket( m_Socket );
		WSACleanup();
		return false;
	}

	m_Thread = std::thread( std::bind( &C_UDPMulticastListener::ThreadProc, this ) );

	return true;
}

void C_UDPMulticastListener::Deinit()
{
	m_Exit = true;
	closesocket( m_Socket );
	m_Socket = INVALID_SOCKET;

	m_Thread = std::thread();

	WSACleanup();
}

void C_UDPMulticastListener::ThreadProc()
{
	uint8_t buf[1024];
	int fromLen = sizeof( struct sockaddr_in );
	SOCKADDR_IN from;

	while ( !m_Exit )
	{
		int n = recvfrom( m_Socket, (char*)buf, sizeof( buf ), 0, ( struct sockaddr * )&from, &fromLen );
		if ( n > (int)sizeof( T_SourceID ) )
		{
			T_SourceID srcID = *( T_SourceID* )&buf[0];

			if ( S_Message msg; msg.Deserialize( srcID, buf + sizeof( T_SourceID ), ( size_t )n - sizeof( T_SourceID ) ) )
			{
				m_MsgPump->PushNewMessage( std::move( msg ) );
			}
		}
	}
}
#endif