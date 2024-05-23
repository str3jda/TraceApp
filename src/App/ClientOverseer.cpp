#include <App/stdafx.h>
#include <App/ClientOverseer.h>
#include <App/Listeners/MessagePump.h>
#include <App/UI/TabModel.h>
#include <App/UI/MainWindow.h>
#include <Trace/Config.h>

static char const* const SOURCE_MAP_FILE = "source_cache.trace";

bool C_ClientOverseer::Init( C_MainWindow* window, char const* _broker_address )
{
	m_MsgPump = new C_MessagePump();
	if ( !m_MsgPump->Init() )
		return false;

	m_MsgPump->InitPipes();
	if ( ( _broker_address != nullptr ) && ( *_broker_address != '\0' ) )
		m_MsgPump->InitMqtt( _broker_address );

	m_MainWindow = window;

	LoadSourceMap();
	GetOrCreateAppClient( 0, false );

	return true;
}

void C_ClientOverseer::Deinit()
{
	m_MsgPump->Deinit();
	delete m_MsgPump;
	m_MsgPump = nullptr;
}

void C_ClientOverseer::StartListening()
{
	m_MsgPump->StartListening();
}

void C_ClientOverseer::Tick()
{
	m_MsgPump->RetrieveMessages( [this]( trace::Message& _msg, trace::TraceAppID_t _app ) 
	{
		if ( _app == 0 )
		{
			m_MainWindow->LogSystemMessage( _msg.Msg.c_str(), _msg.Type );
			return;
		}

		switch ( _msg.Type )
		{
			case trace::TMT_Information:
			case trace::TMT_Log:
			case trace::TMT_Warning:
			case trace::TMT_Error:
			{
				ClientApp& client = GetOrCreateAppClient( _app, true );

				ThreadNameHandle thread_handle;
				if ( !TryGetThreadName( _app, _msg.Thread, thread_handle ) )
				{
					char buf[32];
					_itoa_s( _msg.Thread, buf, 10 );

					m_ThreadNames.push_back( ThreadName{ _app, _msg.Thread, buf } );
					thread_handle = m_ThreadNames.size();
				}

				m_MainWindow->OnTraceMessage( _app, _msg, thread_handle );
				break;
			}

			case trace::TMT_Greetings:
			{
				ClientApp& client = GetOrCreateAppClient( _app, false );

				trace::Message instMsg( trace::TMT_Information, "{{i|[ ----------------------------- NEW PROCESS INSTANCE ----------------------------- ]}}" );
				m_MainWindow->OnTraceMessage( _app, instMsg );

				client.Name = std::move( _msg.Msg );
				client.Inited = true;

				m_MainWindow->OnMessageGreetings( client.Name.c_str(), _app );

				SaveSourceMap();
				break;
			}

			case trace::TMT_Farewell:
				// Client was nice enough to tell us he quits
				m_MainWindow->OnMessageFarewell( _app );
				break;

			case trace::TMT_NewThread:
			{
				// Reuse the same thread name for the same App with the same thread name
				auto it_by_name = std::find_if( 
					m_ThreadNames.begin(), m_ThreadNames.end(), 
					[_app, name = _msg.Msg.c_str()]( ThreadName const& p ){ return ( p.AppId == _app ) && ( p.Name == name ); } );

				auto it_by_id = std::find_if( 
					m_ThreadNames.begin(), m_ThreadNames.end(), 
					[_app, id = _msg.Thread]( ThreadName const& p ){ return ( p.AppId == _app ) && ( p.AppThreadId == id ); } );

				if ( it_by_name != m_ThreadNames.end() )
					it_by_name->AppThreadId = _msg.Thread;
				if ( it_by_id != m_ThreadNames.end() )
					it_by_id->Name = std::move( _msg.Msg );
				else
					m_ThreadNames.push_back( ThreadName{ _app, _msg.Thread, std::move( _msg.Msg ) } );

				SaveSourceMap();
				break;
			}
		}
		
	} );
}

C_ClientOverseer::ClientApp& C_ClientOverseer::GetOrCreateAppClient( trace::TraceAppID_t _app_id, bool _default_init_on_create )
{
	ClientApp* rv;

	auto it = std::find_if( m_Clients.begin(), m_Clients.end(), [ _app_id ]( ClientApp const& x ){ return x.Id == _app_id; } );
	if ( it == m_Clients.end() )
	{
		ClientApp app;
		app.Id = _app_id;

		char buf[64];
		sprintf( buf, "UnnamedApp-0x%x", _app_id );
		app.Name = buf;

		m_Clients.push_back( std::move( app ) );
		rv = &m_Clients.back();
	}
	else
		rv = &( *it );
	
	if ( !rv->Inited && _default_init_on_create )
	{
		rv->Inited = true;
		m_MainWindow->OnMessageGreetings( rv->Name.c_str(), _app_id );
	}

	return *rv;
}

char const* C_ClientOverseer::ResolveThreadName( ThreadNameHandle _handle ) const
{
	return ( _handle < m_ThreadNames.size() ) ? m_ThreadNames[_handle].Name.c_str() : "";
}

bool C_ClientOverseer::TryGetThreadName( trace::TraceAppID_t _app_id, uint32_t _app_thread_id, ThreadNameHandle& _out ) const
{
	auto it = std::find_if( 
					m_ThreadNames.begin(), m_ThreadNames.end(), 
					[=]( ThreadName const& p ){ return ( p.AppId == _app_id ) && p.AppThreadId == _app_thread_id; } );

	if ( it == m_ThreadNames.end() )
		return false;
	
	_out = std::distance( m_ThreadNames.begin(), it );
	return true;
}

void C_ClientOverseer::SetMQTTBroker( char const* server )
{
	m_MsgPump->SetMQTTBroker( server );
}

void C_ClientOverseer::LoadSourceMap()
{
	FILE* f = fopen( SOURCE_MAP_FILE, "rb" );
	if ( f == nullptr )
		return;

	char appName[128];
	uint16_t clLen;
	fread( &clLen, sizeof( uint16_t ), 1, f );

	while ( clLen-- > 0 )
	{
		trace::TraceAppID_t id;
		fread( &id, sizeof( trace::TraceAppID_t ), 1, f );

		uint8_t len;
		fread( &len, sizeof( uint8_t ), 1, f );
		fread( appName, sizeof( char ), len, f );
		appName[len] = '\0';

		ClientApp& client = GetOrCreateAppClient( id, false );
		client.Name = appName;
	}

	uint16_t thLen;
	fread( &thLen, sizeof( uint16_t ), 1, f );

	while ( thLen-- )
	{
		decltype( ThreadName::AppId ) app_id;
		decltype( ThreadName::AppThreadId ) app_thread_id;

		fread( &app_id, sizeof( app_id ), 1, f );
		fread( &app_thread_id, sizeof( app_thread_id ), 1, f );

		uint8_t len;
		fread( &len, sizeof( uint8_t ), 1, f );
		fread( appName, sizeof( char ), len, f );
		appName[len] = '\0';

		m_ThreadNames.push_back( ThreadName{ app_id, app_thread_id, appName } );
	}

	fclose( f );
}

void C_ClientOverseer::SaveSourceMap()
{
	FILE* f = fopen( SOURCE_MAP_FILE, "wb" );
	if ( f == nullptr )
		return;

	// Cache app names
	uint16_t clLen = static_cast<uint16_t>( m_Clients.size() - 1 );
	fwrite( &clLen, sizeof(uint16_t), 1, f );

	for ( ClientApp const& p : m_Clients )
	{
		if ( p.Id == 0 )
			continue;

		fwrite( &p.Id, sizeof( ClientApp::Id ), 1, f );

		uint8_t len = static_cast<uint8_t>( p.Name.length() );
		fwrite( &len, sizeof(uint8_t), 1, f );
		fwrite( p.Name.c_str(), sizeof(char), len, f );
	}

	// Cache thread names	
	uint16_t thLen = static_cast<uint16_t>( m_ThreadNames.size() );
	fwrite( &thLen, sizeof(uint16_t), 1, f );

	for ( ThreadName const& t : m_ThreadNames )
	{
		fwrite( &t.AppId, sizeof( ThreadName::AppId ), 1, f );
		fwrite( &t.AppThreadId, sizeof( ThreadName::AppThreadId ), 1, f );

		uint8_t len = static_cast<uint8_t>( t.Name.length() );
		fwrite( &len, sizeof(uint8_t), 1, f );
		fwrite( t.Name.c_str(), sizeof(char), len, f );		
	}

	fclose( f );
}