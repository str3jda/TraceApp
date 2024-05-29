#include <Trace/Trace.h>
#include <Trace/Config.h>
#include <Trace/Backend.h>

namespace trace
{
	static Backend* BackendIfy( TraceHandle_t _handle ) { return reinterpret_cast< Backend* >( _handle ); }

	bool initialize( Backend& _backend, TraceAppID_t _app_id, char const* _process_name, SysEventCallback_t _callback )
	{
		if ( _app_id == 0 )
			return false;

		if ( !_backend.Init( _app_id, _callback ) )
			return false;

		_backend.SendMessage( TMT_Greetings, _process_name, nullptr, nullptr, 0, 0, 0, 0 );

		return true;
	}

	void deinitialize( TraceHandle_t _handle )
	{
		if ( Backend* backend = BackendIfy( _handle ) )
		{
			backend->SendMessage( TMT_Farewell, nullptr, nullptr, nullptr, 0, 0, 0, 0 );
			backend->Deinit();
		}
	}

	void send_thread_name( TraceHandle_t _handle, uint32_t _thread_id, char const* _thread_name )
	{
		if ( Backend* backend = BackendIfy( _handle ) )
			backend->SendMessage( TMT_NewThread, _thread_name, nullptr, nullptr, 0, 0, _thread_id, 0 );
	}

	void send(
		TraceHandle_t _handle, 
		TracedMessageType_t _type, 
		char const* _message, 
		char const* _file, 
		char const* _function, 
		uint16_t _line,
		uint16_t _thread_id,
		uint32_t _local_time,
		uint32_t _frame )
	{
		if ( Backend* backend = BackendIfy( _handle ) )
			backend->SendMessage( _type, _message, _file, _function, _line, _frame, _thread_id, _local_time );
	}
}