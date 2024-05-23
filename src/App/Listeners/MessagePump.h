#pragma once

#include <Trace/Message.h>

#include <vector>
#include <mutex>
#include <functional>
#include <memory>
#include <atomic>

namespace trace
{
	class BackendMQTT;
	class BackendPipe;
}

class C_MessagePump
{
public:

	C_MessagePump();
	~C_MessagePump();

	typedef std::function< void( trace::Message&, trace::TraceAppID_t ) > T_MessageCallback;

	bool Init();
	void Deinit();

	void InitPipes();
	void InitMqtt( char const* _broker_address );

	void StartListening();

	//
	// UI access
	//

	void RetrieveMessages( T_MessageCallback clbk );
	void SetMQTTBroker( char const* _server );

	// 
	// Listeners 
	//

	void PushNewMessage( trace::TraceAppID_t _app, trace::Message&& msg );

private:

	void StartListening( trace::Backend* _backend );

private:

	std::unique_ptr< trace::BackendMQTT > m_MQTT;
	std::unique_ptr< trace::BackendPipe > m_Pipe;

	struct BackendContext
	{
		trace::Backend* Backend;
		std::jthread Thread;
	};

	std::vector< std::unique_ptr< BackendContext > > m_Backends;

	std::vector< std::pair< trace::TraceAppID_t, trace::Message > > m_PendingMessages;
	std::vector< std::pair< trace::TraceAppID_t, trace::Message > > m_EnumeratedMessages;

	std::mutex m_Mutex;
	std::string m_MqttName;
	std::string m_MqttBroker;

	bool m_Listening = false;
};