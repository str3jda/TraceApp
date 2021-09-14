#include <stdafx.h>
#include "MessagePump.h"
#include "../Listeners/LocalPipeListener.h"
#include "../Listeners/UDPMulticastListener.h"
#include "../Listeners/MQTTListener.h"
#include "Message.h"
#include <Windows.h>

bool C_MessagePump::Init()
{
	m_PipeSource = new C_LocalPipeListener();
	bool rv = m_PipeSource->Init( this );

	//m_UDPSource = new C_UDPMulticastListener();
	//rv = m_UDPSource->Init( this ) && rv;

	char hostname[64];
	int result = gethostname(hostname, sizeof(hostname));
	
	if ( !result )
	{
		m_MqttName = hostname;
		m_MqttName += "|";
	}

	m_MqttName += "TraceServer";
	m_MQTTSource = new C_MQTTListener();

	return rv;
}

void C_MessagePump::Deinit()
{
	m_MQTTSource->Deinit();
	delete m_MQTTSource;
	m_MQTTSource = nullptr;

	//m_UDPSource->Deinit();
	//delete m_UDPSource;
	//m_UDPSource = nullptr;

	m_PipeSource->Done();
	delete m_PipeSource;
	m_PipeSource = nullptr;
}

void C_MessagePump::RetrieveMessages( T_MessageCallback clbk )
{
	{
		std::lock_guard< std::mutex > lock( m_Mutex );
		std::swap( m_PendingMessages, m_EnumeratedMessages );
	}

// 	static bool aa = true;
// 	if ( aa )
// 	{
// 		aa = false;
// 		for ( size_t i = 0; i < 2000; ++i )
// 		{
// 			clbk( S_Message::Create( 5, trace::TMT_Greetings, 0, "TEST", nullptr, nullptr, 1 ) );
// 			clbk( S_Message::Create( 5, trace::TMT_Information, 0, "Lolec", nullptr, nullptr, 1 ) );
// 			clbk( S_Message::Create( 5, trace::TMT_Information, 0, "kakanec", nullptr, nullptr, 1 ) );
// 			clbk( S_Message::Create( 5, trace::TMT_Warning, 0, "bzzz", nullptr, nullptr, 1 ) );
// 			clbk( S_Message::Create( 5, trace::TMT_Information, 0, "omg", nullptr, nullptr, 1 ) );
// 			clbk( S_Message::Create( 5, trace::TMT_Information, 0, "wut?", nullptr, nullptr, 1 ) );
// 		}
// 	}

	for ( S_Message* msg : m_EnumeratedMessages )
	{
		clbk( msg );
	}

	m_EnumeratedMessages.clear();
}

void C_MessagePump::PushNewMessage( S_Message* msg )
{
	std::lock_guard< std::mutex > lock( m_Mutex );
	m_PendingMessages.push_back( msg );
}

void C_MessagePump::SetMQTTBroker( char const* server )
{
	if ( m_MQTTSource->GetServer() != server )
	{
		m_MQTTSource->Deinit();

		if ( server != nullptr )
		{
			m_MQTTSource->Init( this, server, m_MqttName );
		}
	}
}