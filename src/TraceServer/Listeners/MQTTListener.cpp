#include <stdafx.h>
#include "MQTTListener.h"
#include "../Messages/Message.h"
#include "../Messages/MessagePump.h"
#include <TraceCommon/Config.h>
#include <rapidjson/reader.h>

///////////////////////////////////////////////////////////////

struct JsonStreamAdapter
{
	using Ch = char;

	JsonStreamAdapter( char const* _buffer, size_t _buffer_len ) 
		: m_Buffer( _buffer )
		, m_BufferIt( _buffer )
		, m_BufferEnd( _buffer + _buffer_len )
	{}

	char Peek() const { return m_BufferIt < m_BufferEnd ? *m_BufferIt : '\0'; }
	char Take() { return m_BufferIt < m_BufferEnd ? *m_BufferIt++ : '\0'; }
	size_t Tell() const { return m_BufferIt - m_Buffer; }

	char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
	void Put(char) { RAPIDJSON_ASSERT(false); }
	size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

	char const* m_Buffer;
	char const* m_BufferIt;
	char const* m_BufferEnd;
};

///////////////////////////////////////////////////////////////

struct JsonHandler 
{
	// Ugly rapidjson interface
	bool Null() { return true; }
	bool Bool(bool b) { return true; }
	bool Int(int i) 
	{
		m_WasNumber = true;
		m_ValueNumber = static_cast< uint32_t >( i );
		return true; 
	}
	bool Uint(unsigned u)
	{
		m_WasNumber = true;
		m_ValueNumber = u;
		return true; 
	}
	bool Int64(int64_t i) { return true; }
	bool Uint64(uint64_t u) { return true; }
	bool Double(double d) { return true; }
	bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) 
	{
		return true;
	}
	bool String(const char* str, rapidjson::SizeType length, bool copy)
	{
		m_ValueString = str;
		return true;
	}
	bool Key(const char* str, rapidjson::SizeType length, bool copy) 
	{
		Property = str;
		return true;
	}
	bool StartObject() { m_WasStartObject = true; return true; }
	bool EndObject(rapidjson::SizeType) { return true; }
	bool StartArray() { return true; }
	bool EndArray(rapidjson::SizeType) { return true; }

	//

	JsonHandler( rapidjson::Reader& _reader, JsonStreamAdapter& _stream )
		: m_Reader( _reader )
		, m_Stream( _stream )
	{
		m_Reader.IterativeParseInit();
	}

	bool Begin()
	{
		m_WasStartObject = false;
		m_Reader.IterativeParseNext< rapidjson::kParseDefaultFlags >( m_Stream, *this );
		return m_WasStartObject;
	}

	bool NextProperty()
	{
		Property = nullptr;
		m_Reader.IterativeParseNext< rapidjson::kParseDefaultFlags >( m_Stream, *this );
		return Property != nullptr;
	}

	bool ReadPropertyValue( std::string& _value )
	{
		m_ValueString = nullptr;
		m_Reader.IterativeParseNext< rapidjson::kParseDefaultFlags >( m_Stream, *this );
		if ( m_ValueString == nullptr )
			return false;

		_value = m_ValueString;
		return true;
	}

	bool ReadPropertyValue( uint32_t& _value )
	{
		m_WasNumber = false;
		m_Reader.IterativeParseNext< rapidjson::kParseDefaultFlags >( m_Stream, *this );
		if ( !m_WasNumber )
			return false;

		return _value = m_ValueNumber;
	}

	char const* Property;

private:

	rapidjson::Reader& m_Reader;
	JsonStreamAdapter& m_Stream;

	bool m_WasStartObject;
	bool m_WasNumber;
	uint32_t m_ValueNumber;
	char const* m_ValueString;

};

///////////////////////////////////////////////////////////////

void C_MQTTListener::OnMessage( char* topicName, int topicLen, MQTTClient_message *message )
{
	if ( strncmp( topicName, "/trace/", 7 ) == 0 )
	{	
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

///////////////////////////////////////////////////////////////

void C_MQTTListener::ParseAppMsg( trace::TracedMessageType type, char* payload, int payload_len )
{
	S_Message* rv = new S_Message();
	rv->m_Type = type;

    rapidjson::Reader reader;
    JsonStreamAdapter ss( payload, payload_len );

	JsonHandler handler( reader, ss );

	if ( handler.Begin() )
	{
		while ( handler.NextProperty() )
		{
			if ( rv->m_Msg.empty() && ( strcmp( handler.Property, "name" ) == 0 ) )
				handler.ReadPropertyValue( rv->m_Msg );
			else if ( ( rv->m_SourceID == 0 ) && ( strcmp( handler.Property, "id" ) == 0 ) )
				handler.ReadPropertyValue( rv->m_SourceID );
			else if ( strcmp( handler.Property, "thread" ) == 0 )
				handler.ReadPropertyValue( rv->m_Thread );
		}
	}

	if ( rv->m_SourceID != 0 )
	{
		time( reinterpret_cast< time_t* >( &rv->m_Time ) );
		m_MsgPump->PushNewMessage( rv );
	}
	else
		delete rv;

}

///////////////////////////////////////////////////////////////

void C_MQTTListener::ParseTraceMsg( trace::TracedMessageType type, char* payload, int payload_len, char const* prefix )
{
	S_Message* rv = new S_Message();
	rv->m_Type = type;

	rapidjson::Reader reader;
    JsonStreamAdapter ss( payload, payload_len );

	JsonHandler handler( reader, ss );

	if ( handler.Begin() )
	{
		while ( handler.NextProperty() )
		{
			if ( ( rv->m_SourceID == 0 ) && ( strcmp( handler.Property, "app" ) == 0 ) )
				handler.ReadPropertyValue( rv->m_SourceID );
			else if ( rv->m_Msg.empty() && ( strcmp( handler.Property, "msg" ) == 0 ) )
				handler.ReadPropertyValue( rv->m_Msg );
			else if ( rv->m_Fn.empty() && strcmp( handler.Property, "fn" ) == 0 )
				handler.ReadPropertyValue( rv->m_Fn );
			else if ( rv->m_File.empty() && strcmp( handler.Property, "file" ) == 0 )
				handler.ReadPropertyValue( rv->m_File );
			else if ( ( rv->m_Thread == 0 ) && strcmp( handler.Property, "thread" ) == 0 )
				handler.ReadPropertyValue( rv->m_Thread );
			else if ( ( rv->m_Line == 0 ) && strcmp( handler.Property, "line" ) == 0 )
				handler.ReadPropertyValue( rv->m_Line );
		}
	}

	if ( rv->m_SourceID != 0 )
	{
		if ( prefix != nullptr )
			rv->m_Msg = prefix + rv->m_Msg;

		time( reinterpret_cast< time_t* >( &rv->m_Time ) );

		m_MsgPump->PushNewMessage( rv );
	}
	else
		delete rv;
}

///////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////

void C_MQTTListener::SysMessage( trace::TracedMessageType type, char const* msg )
{
	S_Message* rv = new S_Message();
	rv->m_Type = type;
	rv->m_Msg = msg;

	time( reinterpret_cast< time_t* >( &rv->m_Time ) );

	m_MsgPump->PushNewMessage( rv );
}

///////////////////////////////////////////////////////////////

void C_MQTTListener::ThreadProc()
{
	MQTTClient_create( &m_Client, m_MQTTBroker.c_str(), m_MQTTId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, nullptr );

	bool firstConnect = true;
	bool doReport = true;
	DWORD lostConnectionTime = 0;

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

				lostConnectionTime = GetTickCount();
			}

			if ( Connect() )
			{
				if ( !firstConnect )
				{
					DWORD regainConnectionTime = GetTickCount();

					char buf[128];
					sprintf_s( buf, "Connection to MQTT broker %s re-established in %.1fs!", m_MQTTBroker.c_str(), ( regainConnectionTime - lostConnectionTime ) * 1e-3f );
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
}

///////////////////////////////////////////////////////////////

bool C_MQTTListener::Init( C_MessagePump* msgPump, char const* mqttbroker, std::string const& id )
{
	m_Exit = false;
	m_MQTTBroker = mqttbroker;
	m_MQTTId = id;
	m_MsgPump = msgPump;
	m_Thread = std::thread( std::bind( &C_MQTTListener::ThreadProc, this ) );

	return true;
}

///////////////////////////////////////////////////////////////

void C_MQTTListener::Deinit()
{
	m_Exit = true;
	std::atomic_thread_fence( std::memory_order_release );

	if ( m_Thread.joinable() )
		m_Thread.join();
	
	m_Thread = std::thread();
	m_MQTTBroker.clear();
	m_MQTTId.clear();
	m_MsgPump = nullptr;
}