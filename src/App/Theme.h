#pragma once
#include <stdint.h>
#include <algorithm>

namespace theme 
{
	constexpr uint32_t min( uint32_t a, uint32_t b ) { return a < b ? a : b; }

	constexpr uint32_t multiply( uint32_t c, float factor )
	{
		uint32_t r = ( c & 0xff );
		uint32_t g = ( ( c >> 8 ) & 0xff );
		uint32_t b = ( ( c >> 16 ) & 0xff );

		return 
			( min( static_cast< uint32_t >( r * factor ), 255 ) ) | 
			( min( static_cast< uint32_t >( g * factor ), 255 ) << 8 ) |
			( min( static_cast< uint32_t >( b * factor ), 255 ) << 16 );
	}

	static constexpr uint32_t WINDOW_BACKGROUND = 0x252526;
	static constexpr uint32_t TAB_NORMAL_BACKGROUND = multiply(WINDOW_BACKGROUND, 2.5f);
	static constexpr uint32_t TAB_ACTIVE_BACKGROUND = 0x73C991;
}