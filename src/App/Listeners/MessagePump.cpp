#include <App/stdafx.h>
#include <App/Listeners/MessagePump.h>
#include <Trace/Backend/Backend_MQTT.h>
#include <Trace/Backend/Backend_Pipe.h>

C_MessagePump* g_MessagePump = nullptr;

C_MessagePump::C_MessagePump() = default;
C_MessagePump::~C_MessagePump() = default;

static void OnBackendSysEvent( trace::TracedMessageType_t _type, char const* _prefix, char const* _text )
{
	if ( g_MessagePump != nullptr )
	{
		char buf[1024];
		std::sprintf( buf, "[%s] %s", _prefix, _text );
		g_MessagePump->PushNewMessage( 0, trace::Message( _type, buf ) );
	}
}

static void OnMqttBackendSysEvent( trace::TracedMessageType_t _type, char const* _text ) { OnBackendSysEvent( _type, "{{c:2f1|MQTT}}", _text ); }
static void OnPipeBackendSysEvent( trace::TracedMessageType_t _type, char const* _text ) { OnBackendSysEvent( _type, "{{c:89f|PIPE}}", _text ); }

bool C_MessagePump::Init()
{
	bool rv = true;

	g_MessagePump = this;

	m_MqttName = "TraceServer";
	m_MqttBroker = "localhost";

	char hostname[64];
	int result = gethostname( hostname, sizeof( hostname ) );
	
	if ( result == 0 )
	{
		m_MqttName += "|";
		m_MqttName = hostname;
	}

	return rv;
}

void C_MessagePump::InitPipes()
{
	m_Pipe.reset( trace::BackendPipe::Create() );
	if ( !m_Pipe->Init( 0, &OnPipeBackendSysEvent ) )
		m_Pipe = nullptr;
}

void C_MessagePump::InitMqtt( char const* _broker_address )
{
	m_MQTT = std::make_unique< trace::BackendMQTT >( _broker_address, m_MqttName.c_str() );
	if ( !m_MQTT->Init( 0, &OnMqttBackendSysEvent ) )
		m_MQTT = nullptr;
}

void C_MessagePump::Deinit()
{
	m_Listening = false;
	
	for ( auto& ctx : m_Backends )
		if ( ctx->Thread.joinable() )
			ctx->Thread.request_stop();

	for ( auto& ctx : m_Backends )
		if ( ctx->Thread.joinable() )
			ctx->Thread.join();

	m_Backends.clear();
	
	if ( m_MQTT != nullptr )
	{
		m_MQTT->Deinit();
		m_MQTT = nullptr;
	}

	if ( m_Pipe != nullptr )
	{
		m_Pipe->Deinit();
		m_Pipe = nullptr;
	}

	g_MessagePump = nullptr;
}

void C_MessagePump::StartListening()
{
	m_Listening = true;

	if ( m_MQTT != nullptr )
		StartListening( m_MQTT.get() );

	if ( m_Pipe != nullptr )
		StartListening( m_Pipe.get() );
}

void C_MessagePump::StartListening( trace::Backend* _backend )
{
	std::unique_ptr< BackendContext > ctx = std::make_unique< BackendContext >();
	ctx->Backend = _backend;
	ctx->Thread = std::jthread( [this, ctx = ctx.get()]( std::stop_token _stop )
	{
		trace::Message msg;
		trace::ClientId client;

		while ( !_stop.stop_requested() )
		{
			if ( ctx->Backend->PollMessage( msg, client ) )
				PushNewMessage( client.AppId, std::move( msg ) );
		}

		client.AppId = 0;
	} );

	m_Backends.push_back( std::move( ctx ) );
}

void C_MessagePump::RetrieveMessages( T_MessageCallback clbk )
{
	{
		std::lock_guard< std::mutex > lock( m_Mutex );
		std::swap( m_PendingMessages, m_EnumeratedMessages );
	}

#if 0
	static bool aa = true;
	if ( aa )
	{
		auto add = [&clbk](auto t, char const* m)
		{
			trace::Message x( t, m );
			clbk(x, 0);
		};
		aa = false;
		for ( size_t i = 0; i < 200; ++i )
		{
			add( trace::TMT_Error, "TEST" );
			add( trace::TMT_Information,  "Lolec" );
			add( trace::TMT_Information, "kakanec");
			add( trace::TMT_Warning, "bzzz" );
			add( trace::TMT_Log, "omg" );
			add( trace::TMT_Information, "wut?" );
		}
	}
#endif
#if 0
	{
		auto add = [&clbk](auto t, char const* m, uint32_t app)
		{
			trace::Message x( t, m );
			clbk(x, 0);
		};

		uint32_t const rate = 4;
		static uint32_t cooldown = 0;

		if ( cooldown == 0 )
		{
			uint32_t const apps[5] = { 666, 667, 668, 669, 0 };

			trace::Message x( trace::TracedMessageType_t( ( rand() % 4 ) + 1 ),  "caues" );
			clbk(x, apps[ rand() % 5 ]);

			cooldown = rate;
		}
		else
			--cooldown;
	}
#endif

	for ( auto& x : m_EnumeratedMessages )
	{
		clbk( x.second, x.first );
	}

	m_EnumeratedMessages.clear();
}

void C_MessagePump::PushNewMessage( trace::TraceAppID_t _app, trace::Message&& msg )
{
	std::lock_guard< std::mutex > lock( m_Mutex );
	m_PendingMessages.emplace_back( _app, std::move( msg ) );
}

void C_MessagePump::SetMQTTBroker( char const* _server )
{
	if ( m_MqttBroker != _server )
	{
		if ( m_MQTT != nullptr )
		{
			for ( auto& ctx : m_Backends )
			{
				if ( ctx->Backend == m_MQTT.get() )
				{
					if ( ctx->Thread.joinable() )
					{
						ctx->Thread.request_stop();
						ctx->Thread.join();
					}

					ctx = std::move( m_Backends.back() );
					m_Backends.pop_back();
					break;
				}
			}

			m_MQTT->Deinit();
			m_MQTT = nullptr;
		}

		m_MqttBroker = _server;

		m_MQTT = std::make_unique< trace::BackendMQTT >( m_MqttBroker.c_str(), m_MqttName.c_str() );
		if ( !m_MQTT->Init( 0, &OnMqttBackendSysEvent ) )
			m_MQTT = nullptr;
		else if ( m_Listening )
			StartListening( m_MQTT.get() );
	}
}