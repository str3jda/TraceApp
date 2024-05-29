#include <Trace/Backend/Backend_MQTT.h>
#include <Trace/Config.h>
#include <charconv>

namespace trace
{
	static constexpr char const* TopicPrefix = "trace/";
	static constexpr size_t TopicPrefixLen = std::char_traits< char >::length( TopicPrefix );

	std::tuple< char const*, int > get_message_type_descriptor( TracedMessageType_t _type )
	{
		switch ( _type )
		{
			case TMT_Information: 	return { "info", 0 };
			case TMT_Log: 			return { "log", 2 };
			case TMT_Warning: 		return { "warning", 0 }; 
			case TMT_Error: 		return { "error", 2 }; 
			case TMT_Greetings:		return { "hi", 2 }; 
			case TMT_Farewell:		return { "bye", 2 }; 
			case TMT_NewThread:		return { "thread", 2 };
			default: 				return { nullptr, 0 };
		}
	};

	TracedMessageType_t get_message_type_from_name( char const* _type_name )
	{
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Information ); strcmp( name, _type_name ) == 0 ) return TMT_Information;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Log ); strcmp( name, _type_name ) == 0 ) return TMT_Log;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Warning ); strcmp( name, _type_name ) == 0 ) return TMT_Warning;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Error ); strcmp( name, _type_name ) == 0 ) return TMT_Error;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Greetings ); strcmp( name, _type_name ) == 0 ) return TMT_Greetings;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_Farewell ); strcmp( name, _type_name ) == 0 ) return TMT_Farewell;
		if ( auto [ name, _ ] = get_message_type_descriptor( TMT_NewThread ); strcmp( name, _type_name ) == 0 ) return TMT_NewThread;
		return 0;
	};

	struct BackendMQTT::JsonComposer
	{
		JsonComposer( char ( &_output )[ MAX_PAYLOAD_SIZE ] )
			: Iter( &_output[ 0 ] )
			, Begin( &_output[ 0 ] )
			, End( &_output[ MAX_PAYLOAD_SIZE ] )
		{
			*Iter++ = '{';
		}

		uint32_t Finish()
		{
			if ( Error || ( Iter == End ) )
				return 0;

			*Iter++ = '}';

			return static_cast< uint32_t >( std::distance( Begin, Iter ) );
		}

		template < typename T >
		bool operator()( char _attribute, T _value )
		{
			if ( IsFirst )
				IsFirst = false;
			else
				Append( ',' );

			Append( '"' );
			Append( _attribute );
			Append( "\":" );

			if constexpr ( std::is_same_v< T, char const* > )
			{
				size_t len = strlen( _value );
				size_t quotes_count = std::count( _value, _value + len, '"' );
				size_t remain = std::distance( Iter, End );

				Append( '"' );

				if ( quotes_count == 0 )
				{
					Error = memcpy_s( Iter, remain, _value, len ) != 0;
					if ( !Error )
						Iter += len;
				}
				else
				{
					if ( len + quotes_count > remain )
						Error = true;
					else
					{
						for ( size_t i = 0; i < len; ++i )
						{
							if ( _value[i] == '"' )
								*Iter++ = '\\'; // We need to escape quotes (interferes with json fmt)
							*Iter++ = _value[i];
						}
					}
				}

				Append( '"' );
			}
			else
			{
				if ( auto [ptr, ec] = std::to_chars( Iter, End, _value ); ec == std::errc() )
					Iter = ptr;
				else
					Error = true;
			}

			return !Error;
		}

	private:

		inline void Append( char const* _str )
		{
			if (!Error)
			{
				size_t len = strlen( _str );
				Error = memcpy_s( Iter, std::distance( Iter, End ), _str, len ) != 0;

				if ( !Error )
					Iter += len;
			}
		}

		inline void Append( char _char )
		{
			if (!Error)
			{
				if ( Iter == End )
					Error = true;
				else
					*Iter++ = _char;
			}
		}

	private:

		char* Iter;
		char* const Begin;
		char* const End;

		bool Error = false;
		bool IsFirst = true;

	};

#if TRACE_BACKEND_LISTENER
	struct BackendMQTT::JsonParser
	{
		JsonParser( char const* _payload, uint32_t _payload_size )
			: Iter( _payload )
			, End( _payload + _payload_size )
		{
			Error = ( _payload_size == 0 ) || ( _payload == nullptr );

			SkipWhitespace();
			Error = Error || ( *Iter != '{' );

			if ( !Error )
				++Iter;
		}

		bool BeginAttribute( char& _attribute )
		{
			if ( Error )
				return false;

			if ( IsFirst )
				IsFirst = false;
			else
			{
				SkipWhitespace();
				if ( *Iter == ',' )
					++Iter;
				else
					return false;
			}
			
			SkipWhitespace();
			Consume( '"' );
			char rv = *Iter++;
			if ( *Iter != '"' )
				rv = 0;

			ConsumeUntil( '"' );
			SkipWhitespace();
			Consume( ':' );

			_attribute = Error ? 0 : rv;
			return !Error;
		}

		template < typename T >
		void ParseValue( T& _value )
		{
			SkipWhitespace();

			if constexpr ( std::is_same_v< T, std::string > )
			{
				Consume( '"' );
				if ( !Error )
				{
					char const* start = Iter;
					ConsumeUntil( '"' );

					if ( !Error )
						_value.assign( start, Iter - 1 );
				}
			}
			else
			{
				if ( auto [ptr, ec] = std::from_chars( Iter, End, _value ); ec == std::errc() )
					Iter = ptr;
				else
					Error = true;
			}
		}

		void SkipValue()
		{
			if ( !Error )
			{
				char prev_char = 0;
				while ( ( Iter != End ) && !( ( *Iter == ',' || *Iter == '}' || *Iter == ']' ) && ( prev_char != '\\' ) ) )
					prev_char = *Iter++;

				Error = ( Iter == End );
			}
		}

		bool Finish() 
		{ 
			SkipWhitespace();
			Consume( '}') ;
			
			return !Error; 
		}

	private:

		void SkipWhitespace()
		{
			if ( !Error )
				while ( ( Iter != End ) && std::isspace( *Iter ) )
					++Iter;
		}

		void ConsumeUntil( char _char )
		{
			if ( !Error )
			{
				char prev_char = 0;
				while ( ( Iter != End ) && ( ( *Iter != _char ) || ( prev_char == '\\' ) ) )
					prev_char = *Iter++;

				Error = ( *Iter != _char );
				if ( !Error )
					++Iter;
			}
		}

		inline void Consume( char const* _str )
		{
			if ( !Error )
			{
				if ( Iter == End )
					Error = true;
				else
				{
					size_t len = strlen( _str );
					Error = std::strncmp( Iter, _str, len ) != 0;

					if ( !Error )
						Iter += len;
				}
			}
		}

		inline void Consume( char  _char )
		{
			if ( !Error )
			{
				if ( Iter == End )
					Error = true;
				else
				{
					Error = *Iter != _char;
					if ( !Error )
						++Iter;
				}
			}
		}

		char const* Iter;
		char const* End;

		bool Error = false;
		bool IsFirst = true;

	};
#endif

	BackendMQTT::BackendMQTT( MQTTClient_t _existing )
		: m_MqttClient( _existing )
	{
	}

	BackendMQTT::BackendMQTT( char const* _broker_address, char const* _client_id )
		: m_ClientOwned( true )
	{
		if ( MQTTClient_create( 
			&m_MqttClient, 
			_broker_address, 
			_client_id, 
			MQTTCLIENT_PERSISTENCE_NONE, 
			nullptr ) != MQTTCLIENT_SUCCESS )
		{
			ReportSysEvent( "Couldn't create new MQTT client!", TMT_Error );
		}
	}

	bool BackendMQTT::Init( TraceAppID_t _app_id, SysEventCallback_t _sys_event_callback )
	{
		m_AppId = _app_id;
		return Backend::Init( _app_id, _sys_event_callback );
	}

	void BackendMQTT::Deinit()
	{
#if TRACE_BACKEND_LISTENER
		if ( m_Listening )
		{
			MQTTClient_unsubscribe( m_MqttClient, "/trace/#" );
			m_Listening = false;
		}

		m_Connected = false;
#endif

		if ( m_ClientOwned )
		{
			MQTTClient_disconnect( m_MqttClient, 10000 );
			MQTTClient_destroy( &m_MqttClient );
		}
		
		m_MqttClient = nullptr;
	}

	void BackendMQTT::SendMessage( 
		TracedMessageType_t _type, 
		char const* _message, 
		char const* _file, 
		char const* _function, 
		uint16_t _line,
		uint16_t _frame,
		uint32_t _thread_id,
		uint32_t _local_time )
	{
		if ( m_MqttClient == nullptr )
			return;

		if ( !MQTTClient_isConnected( m_MqttClient ) )
			if ( !m_ClientOwned || !Connect() )
				return;

		auto [ topic, qos ] = get_message_type_descriptor( _type );

		if ( topic == nullptr )
			return;

		char payload[MAX_PAYLOAD_SIZE];
		uint32_t payload_len = Serialize( payload, _message, _file, _function, _line, _frame, _thread_id, _local_time );
		if ( payload_len == 0 )
		{
			ReportSysEvent( "Failed to serialize json into mqtt payload!", TMT_Error );
			return;
		}

		size_t topic_len = strlen( topic );

		char full_topic[32];
		memcpy_s( full_topic, sizeof( full_topic ), TopicPrefix, TopicPrefixLen );
		memcpy_s( full_topic + TopicPrefixLen, sizeof( full_topic ) - TopicPrefixLen, topic, topic_len );
		full_topic[ TopicPrefixLen + topic_len ] = '\0';

		MQTTClient_publish( m_MqttClient, full_topic, static_cast< int >( payload_len ), payload, qos, 0, nullptr );
	}

#if TRACE_BACKEND_LISTENER
	bool BackendMQTT::PollMessage( Message& _out, ClientId& _out_source, uint32_t _timeout_ms )
	{
		if ( m_MqttClient == nullptr )
			return false;

		bool connected_now = MQTTClient_isConnected( m_MqttClient );

		if ( !connected_now && m_ClientOwned )
		{
			auto milisec = std::chrono::duration_cast< std::chrono::milliseconds >( 
					std::chrono::system_clock::now() - m_TryConnectAt ).count();

			if ( milisec > 10000 )
			{
				bool report_failure = m_TryConnectAt.time_since_epoch() == std::chrono::system_clock::duration::zero();

				m_TryConnectAt = std::chrono::system_clock::now();
				connected_now = Connect();

				if ( report_failure && !connected_now )
					ReportSysEvent( "Failed to connect to MQTT broker!", TMT_Error );
			}
		}

		if ( !connected_now && m_Connected )
		{
			m_LostConnectionAt = std::chrono::system_clock::now();
			ReportSysEvent( "Lost connection to MQTT broker!", TMT_Warning );
		}

		if ( connected_now && !m_Connected )
		{
			if ( !m_ReadyToRecv )
			{			
				m_ReadyToRecv = true;
				ReportSysEvent( "Ready to receive messages", TMT_Information );
			}
			else
			{
				auto milisec = std::chrono::duration_cast< std::chrono::milliseconds >( 
					std::chrono::system_clock::now() - m_LostConnectionAt ).count();

				char buf[ 128 ];
				sprintf_s( buf, "Connection to MQTT broker re-established in %.1fs!",  milisec * 1e-3f );
				ReportSysEvent( buf, TMT_Information );
			}

			m_Listening = false;
		}

		m_Connected = connected_now;

		if ( !m_Connected )
			return false;

		if ( !m_Listening )
		{
			char full_topic[32];
			memcpy_s( full_topic, sizeof( full_topic ), TopicPrefix, TopicPrefixLen );
			memcpy_s( full_topic + TopicPrefixLen, sizeof( full_topic ) - TopicPrefixLen, "#\0", 2 );

			m_Listening = MQTTClient_subscribe( m_MqttClient, full_topic, 0 ) == MQTTCLIENT_SUCCESS;
		}

		char* topic_name = nullptr;
		int topic_name_len;
		MQTTClient_message* m = nullptr;

		MQTTClient_receive( m_MqttClient, &topic_name, &topic_name_len, &m, _timeout_ms );

		if ( topic_name == nullptr )
			return false;

		bool valid_msg = ProcessIncomingMessage( topic_name, topic_name_len, m, _out, _out_source );

		MQTTClient_freeMessage( &m );
		MQTTClient_free( topic_name );

		return valid_msg;
	}

#endif

	bool BackendMQTT::Connect()
	{
		MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
		conn_opts.keepAliveInterval = 20;
		conn_opts.cleansession = 1;
		conn_opts.retryInterval = 5;
		conn_opts.connectTimeout = 5;

		return MQTTClient_connect( m_MqttClient, &conn_opts ) == MQTTCLIENT_SUCCESS;
	}

	uint32_t BackendMQTT::Serialize( 
		char ( &_output )[ MAX_PAYLOAD_SIZE ], 
		char const* _message, 
		char const* _file, 
		char const* _function, 
		uint16_t _line,
		uint16_t _frame,
		uint32_t _thread_id,
		uint32_t _local_time )
	{
		JsonComposer composer( _output );

		composer( 'a', static_cast< uint32_t >( m_AppId ) );
		composer( 'm', _message );
		
		if ( _file != nullptr )
			composer( 'f', _file );

		if ( _function != nullptr )
		{
			composer( 'c', _function );
			composer( 'l', _line );
		}

		if ( _thread_id != 0 )
			composer( 't', _thread_id );

		if ( _local_time != 0 )
			composer( 'i', _local_time );

		if ( _frame != 0 )
			composer( 'r', _frame );

		return composer.Finish();
	}

#if TRACE_BACKEND_LISTENER

	bool BackendMQTT::Deserialize( char const* _payload, uint32_t _payload_size, Message& _out_message, ClientId& _out_source )
	{
		_out_source.AppId = 0;
		_out_message.Msg.clear();
		_out_message.File.clear();
		_out_message.Function.clear();
		_out_message.Line = 0;
		_out_message.Thread = 0;
		_out_message.LocalTime = 0;

		JsonParser parser( _payload, _payload_size );

		for ( char attribute; parser.BeginAttribute( attribute ); )
		{
			switch ( attribute )
			{
				case 'a': parser.ParseValue( _out_source.AppId ); break;
				case 'm': parser.ParseValue( _out_message.Msg ); break;
				case 'f': parser.ParseValue( _out_message.File ); break;
				case 'c': parser.ParseValue( _out_message.Function ); break;
				case 'l': parser.ParseValue( _out_message.Line ); break;
				case 'r': parser.ParseValue( _out_message.Frame ); break;
				case 't': parser.ParseValue( _out_message.Thread ); break;
				case 'i': parser.ParseValue( _out_message.LocalTime ); break;
				default:
					// Should I report unknown attribute?
					parser.SkipValue();
					break;
			}
		}

		if ( _out_source.AppId == 0 )
			return false;

		return parser.Finish();
	}

	bool BackendMQTT::ProcessIncomingMessage( 
		char const* _topic_name, 
		int _topic_len, 
		MQTTClient_message* _message, 
		Message& _out_message,
		ClientId& _out_source )
	{
		if ( strncmp( _topic_name, TopicPrefix, TopicPrefixLen ) != 0 )
			return false;

		TracedMessageType_t type = get_message_type_from_name( _topic_name + TopicPrefixLen );
		if ( type == 0 )
		{
			char buf[ 32 ];
			sprintf_s( buf, "Unsupported message type '%s'",  _topic_name + TopicPrefixLen );
			ReportSysEvent( buf, TMT_Warning );

			return false;
		}

		if ( !Deserialize( 
			static_cast< char const* >( _message->payload ), 
			static_cast< uint32_t >( _message->payloadlen ), 
			_out_message,
			_out_source ) )
		{
			char buf[ 128 ];
			sprintf_s( buf, "Invalid payload: %s", &buf[0] );

			ReportSysEvent( buf, TMT_Warning );
			return false;
		}

		_out_message.Type = type;

		return true;
	}

#endif

	extern bool initialize( Backend& _backend, TraceAppID_t _app_id, char const* _process_name, SysEventCallback_t _callback );

	TraceHandle_t initialize_mqtt( TraceAppID_t _app_id, char const* _broker_address, char const* _client_id, char const* _process_name, SysEventCallback_t _callback )
	{
		static BackendMQTT s_Impl( _broker_address, _client_id );
		if ( !initialize( s_Impl, _app_id, _process_name, _callback ) )
			return InvalidHandle;

		return reinterpret_cast< TraceHandle_t >( &s_Impl );
	}

	TraceHandle_t initialize_mqtt( TraceAppID_t _app_id, MQTTClient_t _existing, char const* _process_name, SysEventCallback_t _callback )
	{
		static BackendMQTT s_Impl( _existing );
		if ( !initialize( s_Impl, _app_id, _process_name, _callback ) )
			return InvalidHandle;

		return reinterpret_cast< TraceHandle_t >( &s_Impl );
	}

}