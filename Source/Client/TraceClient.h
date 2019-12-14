#pragma once
#include <stdint.h>

#define TRACER_SYNC_NAME "traceserver"
#define TRACER_PIPE_NAME "\\\\.\\pipe\\tracepipe"

 namespace trace {

	 typedef uint32_t T_TraceAppID;
	 typedef unsigned char E_TracedMessageType;
	 static E_TracedMessageType const TMT_Information = 1 << 0;
	 static E_TracedMessageType const TMT_Warning = 1 << 1;
	 static E_TracedMessageType const TMT_Error = 1 << 2;
	 static E_TracedMessageType const TMT_Greetings = 1 << 3;
	 static E_TracedMessageType const TMT_Farewell = 1 << 4;
	 static E_TracedMessageType const TMT_NewThread = 1 << 5;

	 class C_TraceClient
	 {
	 public:
		 C_TraceClient();
		 ~C_TraceClient();

		 bool Connect( T_TraceAppID srcId  );
		 void Disconnect();

		 bool LogMessage( E_TracedMessageType inType, const char* inMessage, const char* inFile, const char* inFunction, unsigned int inLine );

	 private:

		 bool CreatePipe();

	 private:

		 void* m_Pipe;
		 T_TraceAppID m_AppID;
	 };

}
