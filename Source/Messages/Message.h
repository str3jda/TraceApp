#pragma once

struct S_Message
{
	static S_Message* Create( T_SourceID sourceId, trace::E_TracedMessageType type, uint32_t line, char const* msg, char const* fn, char const* file, uint32_t inThread );
	static S_Message* Deserialize( T_SourceID sourceId, uint8_t const* data, size_t dataSize );

	T_SourceID const m_SourceID;

	uint64_t m_Time;
	uint32_t m_Line;
	uint32_t m_Thread;

	trace::E_TracedMessageType m_Type;

	char m_Msg[768];
	char m_Fn[128];
	char m_File[128];
	
	S_Message( T_SourceID sourceId )
		: m_SourceID( sourceId )
	{}

};