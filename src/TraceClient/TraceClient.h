#pragma once

#include <stdint.h>

 namespace trace_client {

	 using TraceAppID = uint32_t;
	 using TracedMessageType = uint8_t;

	 static TracedMessageType const TMT_Information		= 0x01;
	 static TracedMessageType const TMT_Warning			= 0x02;
	 static TracedMessageType const TMT_Error			= 0x04;

	 class TraceClient
	 {
	 public:
		 TraceClient();
		 ~TraceClient();

		 bool Connect( TraceAppID _srcId, char const* _app_name = nullptr );
		 void Disconnect();

		 void ReportNewThread( char const* _thread_name );
		 bool LogMessage( TracedMessageType _type, char const* _message, char const* _file, char const* _function, unsigned int _line );

	 private:

		 bool CreatePipe();

	 private:

		 void* m_Pipe;
		 TraceAppID m_AppID;
	 };

}
