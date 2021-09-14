#pragma once

#include <unordered_map>
#include "Messages/Message.h"

class C_MessagePump;
class C_TabModel;
class C_MainWindow;

class C_ClientOverseer
{
public:

	bool Init( C_MainWindow* window );
	void Deinit();

	void Tick();

	void SetMQTTBroker( char const* server );

private:

	void LoadSourceMap();
	void SaveSourceMap();

	C_MessagePump* m_MsgPump = nullptr;
	C_MainWindow* m_MainWindow = nullptr;

	struct S_Client
	{
		std::unordered_map< uint32_t, std::string > m_ThreadNames;
		std::string m_AppName;
		bool m_Inited = false;
	};

	std::unordered_map< T_SourceID, S_Client > m_Clients;
};