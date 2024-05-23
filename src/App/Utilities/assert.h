#pragma once

#include <stdint.h>
#include <stdlib.h>

#define TRACE_ASSERT( cond, msg, ... ) ((cond) ? true : assert::assert_msg( #cond, __FUNCTION__, __LINE__, msg, __VA_ARGS__ ) )

class IAssertCallback
{
public:
	virtual ~IAssertCallback() = default;
	virtual void ReportAssert( char const* msg ) = 0;
};

IAssertCallback* get_assert_callback();

namespace assert
{
	inline bool assert_msg( char const* _condition_str, char const* _fn, uint32_t _line, char const* _msg, ... )
	{
		va_list arglist;

		va_start( arglist, _msg );

		char buf_msg[256];
		int result = vsnprintf( buf_msg, sizeof( buf_msg ), _msg, arglist );
		if ( ( result < 0 ) || ( result > sizeof( buf_msg ) ) )
			strcpy_s( buf_msg, sizeof( buf_msg ), "!Invalid assert message!" );

		va_end( arglist );

		char buf_msg_final[256];
		snprintf( buf_msg_final, sizeof( buf_msg_final ), "%s : %s (fn:%s: line:%u)", _condition_str, &buf_msg[0], _fn, _line );
		
		get_assert_callback()->ReportAssert( buf_msg_final );
		return false;
	}
}
