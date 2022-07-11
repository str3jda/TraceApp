#pragma once

#include <thread>
#include <MQTTClient.h>

class C_MessagePump;

class C_MQTTListener
{
public:

	bool Init( C_MessagePump* msgPump, char const* mqttbroker, std::string const& id );
	void Deinit();

	std::string const& GetServer() const { return m_MQTTBroker; }

private:

	void ParseAppMsg( trace::TracedMessageType type, char* msg, int msg_len );
	void ParseTraceMsg( trace::TracedMessageType type, char* msg, int msg_len, char const* prefix = nullptr );

	void OnMessage( char *topicName, int topicLen, MQTTClient_message *message );

	bool Connect();

	void ThreadProc();

	void SysMessage( trace::TracedMessageType type, char const* msg );

private:

	C_MessagePump* m_MsgPump;
	MQTTClient m_Client;

	bool m_Exit = false;

	std::string m_MQTTBroker;
	std::string m_MQTTId;
	std::thread m_Thread;

};