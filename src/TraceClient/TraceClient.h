#pragma once
#include <TraceCommon/Common.h>

 namespace trace 
 {

	 class TraceClient
	 {
	 public:

		static TraceClient& GetInstance();

		virtual ~TraceClient() {}

		virtual bool Connect( TraceAppID _srcId, char const* _app_name = nullptr ) = 0;
		virtual void Disconnect() = 0;

		void ReportNewThread( char const* _thread_name );
		bool LogMessage( TracedMessageType _type, char const* _message, char const* _file, char const* _function, unsigned int _line );

	protected:

		virtual bool Publish( uint8_t const* _packet_data, uint32_t _packet_data_size ) = 0;

	protected:

		TraceAppID m_AppID;
		bool m_Ready = false;

	 };

}
