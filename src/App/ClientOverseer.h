#pragma once

#include <Trace/Message.h>
#include <vector>

class C_MessagePump;
class C_TabModel;
class C_MainWindow;

class C_ClientOverseer
{
public:

	bool Init( C_MainWindow* window, char const* _broker_address );
	void Deinit();

	void StartListening();

	void Tick();

	void SetMQTTBroker( char const* server );

	char const* ResolveThreadName( ThreadNameHandle _handle ) const;

private:

	struct ClientApp;

	void LoadSourceMap();
	void SaveSourceMap();

	ClientApp& GetOrCreateAppClient( trace::TraceAppID_t _app_id, bool _default_init_on_create );
	bool TryGetThreadName( trace::TraceAppID_t _app_id, uint32_t _app_thread_id, ThreadNameHandle& _out ) const;

	C_MessagePump* m_MsgPump = nullptr;
	C_MainWindow* m_MainWindow = nullptr;

	struct ThreadName
	{
		trace::TraceAppID_t AppId;
		uint32_t AppThreadId;

		std::string Name;
	};

	std::vector< ThreadName > m_ThreadNames;

	struct ClientApp
	{
		trace::TraceAppID_t Id;
		std::string Name;
		bool Inited = false;
	};

	std::vector< ClientApp > m_Clients;
};