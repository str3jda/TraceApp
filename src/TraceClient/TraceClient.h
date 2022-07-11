#pragma once
#include <TraceCommon/Common.h>

 namespace trace 
 {

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
