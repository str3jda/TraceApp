#pragma once
#include <Trace/Trace.h>
#include <string>

namespace trace
{
	// Message can contain rich text. Use as {{key:value|formatted text}}, where
	// Supported @key are:
	//	"c:hex color value" - RGB value - 3 or 6 characters long (in 3 version, it's multiple of 16 => 5c0 equals to 50c000)
	// 		Example: "Hello {{c:f00|wonderer}}!" - Will print "wonderer" in red color
	//	"g:hex color value" - RGB value - 3 or 6 characters long (in 3 version, it's multiple of 16 => 5c0 equals to 50c000)
	// 		Example: "Hello {{c:f00|wonderer}}!" - Will print "wonderer" with red background color
	//	"b" - bold font
	// 		Example: "I'm {{b|bold guy}}"
	//	"i" - italic font
	// 		Example: "I'm {{i|italic guy}}"
	//	"u" - underlined font
	// 		Example: "I'm {{i|underlined guy}}"
	static constexpr char BLOCK_START_FORMAT[3] = "{{";
	static constexpr char BLOCK_END_FORMAT[3] 	= "}}";
	static constexpr char BLOCK_FMT_FONT_COLOR = 'c';
	static constexpr char BLOCK_FMT_FONT_BACK_COLOR = 'g';
	static constexpr char BLOCK_FMT_FONT_BOLD = 'b';
	static constexpr char BLOCK_FMT_FONT_ITALIC = 'i';
	static constexpr char BLOCK_FMT_FONT_UNDERLINED = 'u';

	struct Message
	{
		Message() = default;
		Message( TracedMessageType_t _type, 
			char const* _msg, 
			char const* _fn = nullptr, 
			char const* _file = nullptr, 
			uint16_t _line = 0, 
			uint32_t _thread = 0, 
			uint32_t _local_time = 0 )

			: Type( _type )
			, Line( _line )
			, Thread( _thread )
			, LocalTime( _local_time )
		{
			if ( _msg != nullptr ) Msg = _msg;
			if ( _fn != nullptr ) Function = _fn;
			if ( _file != nullptr ) File = _file;
		}
	
		Message( Message const& ) = delete;
		Message( Message && ) noexcept = default;

		Message& operator=( Message const& ) = delete;
		Message& operator=( Message && ) noexcept = default;
	
		TracedMessageType_t Type = TMT_Information;

		uint16_t Line = 0;
		uint16_t Frame = 0; // Frame index supplied (used to help recognizing messages within the same frame)
		uint32_t Thread = 0;
		uint32_t LocalTime = 0; // App local time (in milliseconds? it's up to client to give it a meaning, UI is only visualizing number)

		std::string Msg;
		std::string Function;
		std::string File;

	};
}