#pragma once

#include <thread>

#define TRACE_MULTICAST_IP "234.5.6.7"
#define TRACE_MULTICAST_PORT 2608

class C_MessagePump;

class C_UDPMulticastListener
{
public:
	
	C_UDPMulticastListener();
	
	bool Init( C_MessagePump* msgPump );
	void Deinit();

private:

	void ThreadProc();

	C_MessagePump* m_MsgPump;
	UINT_PTR m_Socket;
	bool m_Exit = false;

	std::thread m_Thread;
};