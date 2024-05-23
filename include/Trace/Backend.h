#pragma once
#include <Trace/export.h>
#include <Trace/Trace.h>
#include <Trace/Message.h>

namespace trace
{
	struct ClientId
	{
		TraceAppID_t AppId;
	};

	class TRACE_EXPORT Backend
	{
	public:

		virtual ~Backend() = default;

		virtual bool Init( [[maybe_unused]] TraceAppID_t _app_id, SysEventCallback_t _sys_event_callback ) { m_SysEventCallback = _sys_event_callback; return true; }
		virtual void Deinit() {}

		virtual void SendMessage( 
			TracedMessageType_t _type, 
			char const* _message, 
			char const* _file, 
			char const* _function, 
			uint32_t _line,
			uint32_t _thread_id,
			uint32_t _local_time ) = 0;

#if TRACE_BACKEND_LISTENER
		virtual bool PollMessage( Message& _out, ClientId& _out_source, uint32_t _timeout = 200 ) = 0;
#endif

	protected:

		void ReportSysEvent( char const* _text, TracedMessageType_t _type = TMT_Information ) { if ( m_SysEventCallback ) m_SysEventCallback( _type, _text ); }

	private:

		SysEventCallback_t m_SysEventCallback;

	};
}