#pragma once
#include <string>

struct S_Message
{
	static S_Message* Create( T_SourceID sourceId, trace::TracedMessageType type, uint32_t line, char const* msg, char const* fn, char const* file, uint32_t inThread );
	static S_Message* Deserialize( T_SourceID sourceId, uint8_t const* data, size_t dataSize );

	T_SourceID m_SourceID = 0;

	uint32_t m_Line = 0;
	uint32_t m_Thread = 0;

	trace::TracedMessageType m_Type;

	std::string m_Msg;
	std::string m_Fn;
	std::string m_File;

	uint64_t m_Time = 0;	
};