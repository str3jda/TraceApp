#include <stdafx.h>
#include "ClientOverseer.h"
#include "Messages/MessagePump.h"
#include "UI/TabModel.h"
#include "UI/mainwindow.h"

static char const* const SOURCE_MAP_FILE = "source_cache.trace";

bool C_ClientOverseer::Init( C_MainWindow* window )
{
	m_MsgPump = new C_MessagePump();
	if ( !m_MsgPump->Init() )
	{
		return false;
	}

	m_MainWindow = window;

	m_MainWindow->OnMessageGreetings( "TraceServer", 0 );
	m_MainWindow->LogSystemMessage( "Awaits connection" );

	LoadSourceMap();
	m_Clients[0].m_Inited = true;

	return true;
}

void C_ClientOverseer::Deinit()
{
	m_MainWindow->OnMessageFarewell( 0 );

	m_MsgPump->Deinit();
	delete m_MsgPump;
	m_MsgPump = nullptr;
}

void C_ClientOverseer::Tick()
{
	m_MsgPump->RetrieveMessages( [this]( S_Message* msg ) 
	{
		S_Client& client = m_Clients[msg->m_SourceID];

		switch ( msg->m_Type )
		{
			case trace::TMT_Information:
			case trace::TMT_Warning:
			case trace::TMT_Error:
			{
				if ( !client.m_Inited )
				{
					if ( client.m_AppName.empty() )
					{
						char buf[64];
						sprintf( buf, "UnknownApp-0x%x", msg->m_SourceID );
						client.m_AppName = buf;
					}

					m_MainWindow->OnMessageGreetings( client.m_AppName.c_str(), msg->m_SourceID );
					client.m_Inited = true;
				}

				char buf[32];
				char const* threadName = nullptr;

				auto itT = client.m_ThreadNames.find( msg->m_Thread );
				if ( itT != client.m_ThreadNames.end() )
				{
					threadName = itT->second.c_str();
				}
				else
				{
					itoa( msg->m_Thread, buf, 10 );
					threadName = buf;
				}
					
				m_MainWindow->OnTraceMessage( *msg, threadName );
				break;
			}

			case trace::TMT_Greetings:
			{
				// client re-connected ...

				S_Message* delMsg = S_Message::Create( msg->m_SourceID, trace::TMT_Information, 0, "[ ----------------------------- NEW PROCESS INSTANCE ----------------------------- ]", nullptr, nullptr, 0 );
				m_MainWindow->OnTraceMessage( *delMsg, nullptr );
				delete delMsg;

				m_MainWindow->OnMessageGreetings( msg->m_Msg, msg->m_SourceID );
				client.m_AppName = msg->m_Msg;
				client.m_Inited = true;

				SaveSourceMap();
				break;
			}

			case trace::TMT_Farewell:
				// Client was nice enough to tell us he quits
				m_MainWindow->OnMessageFarewell( msg->m_SourceID );
				break;

			case trace::TMT_NewThread:
				// Custom mapping from thread-id to thread-name
				client.m_ThreadNames[msg->m_Thread] = msg->m_Msg;
				SaveSourceMap();
				break;
		}
		
	} );
}

void C_ClientOverseer::SetMQTTBroker( char const* server )
{
	m_MsgPump->SetMQTTBroker( server );
}

void C_ClientOverseer::LoadSourceMap()
{
	FILE* f = fopen( SOURCE_MAP_FILE, "rb" );
	if ( f != nullptr )
	{
		char appName[128];

		uint16_t clLen;
		fread( &clLen, sizeof( uint16_t ), 1, f );

		while ( clLen-- )
		{
			T_SourceID id;
			fread( &id, sizeof( T_SourceID ), 1, f );

			uint8_t len;
			fread( &len, sizeof( uint8_t ), 1, f );
			fread( appName, sizeof( char ), len, f );
			appName[len] = '\0';

			S_Client client;
			client.m_AppName = appName;

			uint16_t thLen;
			fread( &thLen, sizeof( uint16_t ), 1, f );
			while ( thLen-- )
			{
				uint32_t thId;
				fread( &thId, sizeof( uint32_t ), 1, f );

				fread( &len, sizeof( uint8_t ), 1, f );
				fread( appName, sizeof( char ), len, f );
				appName[len] = '\0';

				client.m_ThreadNames.emplace( thId, appName );
			}

			m_Clients.emplace( id, std::move( client ) );
		}

		fclose( f );
	}
}

void C_ClientOverseer::SaveSourceMap()
{
	FILE* f = fopen( SOURCE_MAP_FILE, "wb" );
	if ( f != nullptr )
	{
		uint16_t clLen = static_cast<uint16_t>( m_Clients.size() );
		fwrite( &clLen, sizeof(uint16_t), 1, f );

		for ( auto const& p : m_Clients )
		{
			S_Client const& client = p.second;
			fwrite( &p.first, sizeof( T_SourceID ), 1, f );

			uint8_t len = static_cast<uint8_t>( client.m_AppName.length() );
			fwrite( &len, sizeof(uint8_t), 1, f );
			fwrite( client.m_AppName.c_str(), sizeof(char), len, f );

			uint16_t thLen = static_cast<uint16_t>( client.m_ThreadNames.size() );
			fwrite( &thLen, sizeof(uint16_t), 1, f );

			for ( auto const& t : client.m_ThreadNames )
			{
				fwrite( &t.first, sizeof(uint32_t), 1, f );

				len = static_cast<uint8_t>( t.second.length() );
				fwrite( &len, sizeof(uint8_t), 1, f );
				fwrite( t.second.c_str(), sizeof(char), len, f );
			}
		}

		fclose( f );
	}
}