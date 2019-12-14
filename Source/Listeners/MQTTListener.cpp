#include <stdafx.h>
#include "MQTTListener.h"
#include "../Messages/Message.h"
#include "../Messages/MessagePump.h"
#include <vjson/json.h>

void C_MQTTListener::OnMessage( char* topicName, int topicLen, MQTTClient_message *message )
{
	if ( strncmp( topicName, "/trace/", 7 ) == 0 )
	{
		bool validMsg = true;
		
		if ( strcmp( topicName + 7, "log" ) == 0 )
		{
			ParseTraceMsg( trace::TMT_Information, (char*)message->payload, message->payloadlen, "LOG: " );
		}
		else if ( strcmp( topicName + 7, "info" ) == 0 )
		{
			ParseTraceMsg( trace::TMT_Information, (char*)message->payload, message->payloadlen );
		}
		else if ( strcmp( topicName + 7, "warning" ) == 0 )
		{
			ParseTraceMsg( trace::TMT_Warning, (char*)message->payload, message->payloadlen );
		}
		else if ( strcmp( topicName + 7, "error" ) == 0 )
		{
			ParseTraceMsg( trace::TMT_Error, (char*)message->payload, message->payloadlen );
		}
		else if ( strcmp( topicName + 7, "thread" ) == 0 )
		{
			ParseAppMsg(  trace::TMT_NewThread, (char*)message->payload, message->payloadlen );
		}
		else if ( strcmp( topicName + 7, "app" ) == 0 )
		{
			ParseAppMsg(  trace::TMT_Greetings, (char*)message->payload, message->payloadlen );
		}
		else if ( strcmp( topicName + 7, "exit" ) == 0 )
		{
			ParseAppMsg(  trace::TMT_Farewell, (char*)message->payload, message->payloadlen );
		}
	}
}

void C_MQTTListener::ParseAppMsg( trace::E_TracedMessageType type, char* payload, int payload_len )
{
	T_SourceID sourceId = 0;
	unsigned int thread = 0;
	char appName[256];
	appName[0] = '\0';

	char *errorPos = 0;
	const char *errorDesc = 0;
	int errorLine = 0;
	block_allocator allocator(1 << 10);

	json_value* root = json_parse( payload, payload_len, &errorPos, &errorDesc, &errorLine, &allocator );
	if ( root == nullptr )
	{
		return;
	}

	if ( root->type == JSON_OBJECT )
	{
		for (json_value *it = root->first_child; it != nullptr; it = it->next_sibling)
		{
			if ( ( appName[0] == '\0' ) && ( strcmp( it->name, "name" ) == 0 ) )
			{
				strcpy_s( appName, it->string_value );
			}
			else if ( ( sourceId == 0 ) && ( strcmp( it->name, "id" ) == 0 ) )
			{
				sourceId = static_cast< T_SourceID >( it->int_value );
			}
			else if ( strcmp( it->name, "thread" ) == 0 )
			{
				thread = static_cast< unsigned int >( it->int_value );
			}
		}
	}

	if ( sourceId != 0 )
	{
		S_Message* rv = new S_Message( sourceId );
		rv->m_Type = type;
		rv->m_Thread = thread;
		strcpy_s( rv->m_Msg, appName );

		time( reinterpret_cast< time_t* >( &rv->m_Time ) );

		m_MsgPump->PushNewMessage( rv );
	}
}

void C_MQTTListener::ParseTraceMsg( trace::E_TracedMessageType type, char* payload, int payload_len, char const* prefix )
{
	T_SourceID sourceId = 0;
	
	char const* msg = nullptr;
	char const* file = nullptr;
	char const* fn = nullptr;

	unsigned int line = 0;
	unsigned int thread = 0;

	char *errorPos = 0;
	const char *errorDesc = 0;
	int errorLine = 0;
	block_allocator allocator(1 << 10);

	json_value* root = json_parse( payload, payload_len, &errorPos, &errorDesc, &errorLine, &allocator );
	if ( root == nullptr )
	{
		return;
	}

	if ( root->type == JSON_OBJECT )
	{
		for (json_value *it = root->first_child; it != nullptr; it = it->next_sibling)
		{
			if ( ( sourceId == 0 ) && ( strcmp( it->name, "app" ) == 0 ) )
			{
				sourceId = static_cast< T_SourceID >( it->int_value );
			}
			else if ( ( msg == nullptr ) && ( strcmp( it->name, "msg" ) == 0 ) )
			{
				msg = it->string_value;
			}
			else if ( ( fn == nullptr ) && ( strcmp( it->name, "fn" ) == 0 ) )
			{
				fn = it->string_value;
			}
			else if ( ( file == nullptr ) && ( strcmp( it->name, "file" ) == 0 ) )
			{
				file = it->string_value;
			}
			else if ( strcmp( it->name, "thread" ) == 0 )
			{
				thread = static_cast< unsigned int >( it->int_value );
			}
			else if ( strcmp( it->name, "line" ) == 0 )
			{
				line = static_cast< unsigned int >( it->int_value );
			}
		}
	}

	if ( sourceId != 0 )
	{
		S_Message* rv = new S_Message( sourceId );
		rv->m_Type = type;
		rv->m_Thread = thread;

		rv->m_Msg[0] = '\0';
		rv->m_Fn[0] = '\0';
		rv->m_File[0] = '\0';

		if ( prefix != nullptr )
		{
			size_t prefix_len = strlen( prefix );

			strcpy_s( rv->m_Msg, prefix );
			if ( msg != nullptr )
			{
				strncat_s( rv->m_Msg, msg, sizeof( S_Message::m_Msg ) - 1 - prefix_len );
				rv->m_Msg[sizeof( S_Message::m_Msg ) - 1 - prefix_len] = '\0';
			}
		}
		else if ( msg != nullptr )
		{
			strncpy_s( rv->m_Msg, msg, sizeof( S_Message::m_Msg ) - 1 );
			rv->m_Msg[sizeof( S_Message::m_Msg ) - 1] = '\0';
		}

		if ( fn != nullptr )
		{
			strcpy_s( rv->m_Fn, fn );
		}

		if ( file != nullptr )
		{
			strcpy_s( rv->m_File, file );
		}

		rv->m_Line = line;

		time( reinterpret_cast< time_t* >( &rv->m_Time ) );

		m_MsgPump->PushNewMessage( rv );
	}
}

bool C_MQTTListener::Connect()
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.retryInterval = 5;
	conn_opts.connectTimeout = 5;

	if (MQTTClient_connect(m_Client, &conn_opts) != MQTTCLIENT_SUCCESS)
	{
		return false;
	}

	MQTTClient_subscribe( m_Client, "/trace/#", 0 );

	return true;
}

void C_MQTTListener::SysMessage( trace::E_TracedMessageType type, char const* msg )
{
	S_Message* rv = new S_Message( 0 );
	rv->m_Type = type;
	rv->m_Thread = 0;
	rv->m_Line = 0;

	rv->m_Msg[0] = '\0';
	rv->m_Fn[0] = '\0';
	rv->m_File[0] = '\0';

	strcpy_s( rv->m_Msg, msg );

	time( reinterpret_cast< time_t* >( &rv->m_Time ) );

	m_MsgPump->PushNewMessage( rv );
}

void C_MQTTListener::ThreadProc()
{
	MQTTClient_create( &m_Client, m_MQTTBroker.c_str(), m_MQTTId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, nullptr );

	bool firstConnect = true;
	bool doReport = true;

	while ( !m_Exit )
	{
		if ( !MQTTClient_isConnected( m_Client ) )
		{
			if ( !firstConnect && doReport )
			{
				doReport = false;
				char buf[128];
				sprintf_s( buf, "Lost connection to MQTT broker at %s!", m_MQTTBroker.c_str() );

				SysMessage( trace::TMT_Warning, buf );
			}

			if ( Connect() )
			{
				if ( !firstConnect )
				{
					char buf[128];
					sprintf_s( buf, "Connection to MQTT broker %s re-established!", m_MQTTBroker.c_str() );
					SysMessage( trace::TMT_Information, buf );
				}

				firstConnect = false;
				doReport = true;
			}
			else
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for( 5s );
			}
		}
		else
		{
			char* topicName = nullptr;
			int topicNameLen;
			MQTTClient_message* m = nullptr;

			MQTTClient_receive( m_Client, &topicName, &topicNameLen, &m, 1000 );

			if ( topicName != nullptr )
			{
				OnMessage( topicName, topicNameLen, m );

				MQTTClient_freeMessage( &m );
				MQTTClient_free( topicName );
			}
		}
	}

	MQTTClient_unsubscribe( m_Client, "/trace/#" );
	MQTTClient_disconnect( m_Client, 10000 );
	MQTTClient_destroy( &m_Client );

	m_Finished = true;
}

bool C_MQTTListener::Init( C_MessagePump* msgPump, char const* mqttbroker, std::string const& id )
{
	m_Finished = false;
	m_Exit = false;
	m_MQTTBroker = mqttbroker;
	m_MQTTId = id;
	m_MsgPump = msgPump;
	m_Thread = std::thread( std::bind( &C_MQTTListener::ThreadProc, this ) );

	return true;
}

void C_MQTTListener::Deinit()
{
	m_Exit = true;

	while ( !m_Finished )
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for( 10ms );
	}

	m_Thread = std::thread();
	m_MQTTBroker = "";
	m_MQTTId = "";
	m_MsgPump = nullptr;
}