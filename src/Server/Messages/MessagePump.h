#pragma once

#include <vector>
#include <mutex>
#include <functional>

class C_LocalPipeListener;
class C_UDPMulticastListener;
class C_MQTTListener;

struct S_Message;

class C_MessagePump
{
public:

	typedef std::function< void( S_Message* ) > T_MessageCallback;

	bool Init();
	void Deinit();

	//
	// UI access
	//

	void RetrieveMessages( T_MessageCallback clbk );
	void SetMQTTBroker( char const* server );

	// 
	// Listeners 
	//

	void PushNewMessage( S_Message* msg );

private:

	C_LocalPipeListener* m_PipeSource;
	C_UDPMulticastListener* m_UDPSource;
	C_MQTTListener* m_MQTTSource;

	std::vector< S_Message* > m_PendingMessages;
	std::vector< S_Message* > m_EnumeratedMessages;

	std::mutex m_Mutex;
	std::string m_MqttName;
};