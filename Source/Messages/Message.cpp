#include <stdafx.h>
#include "Message.h"
#include <time.h>

S_Message* S_Message::Create( T_SourceID sourceId, trace::E_TracedMessageType type, uint32_t line, char const* msg, char const* fn, char const* file, uint32_t inThread )
{
	S_Message* rv = new S_Message( sourceId );

	rv->m_Type = type;
	rv->m_Line = line;

	if ( nullptr != msg )
	{
		strcpy( rv->m_Msg, msg );
	}
	else
	{
		rv->m_Msg[0] = '\0';
	}

	if ( nullptr != fn )
	{
		strcpy( rv->m_Fn, fn );
	}
	else
	{
		rv->m_Fn[0] = '\0';
	}

	if ( nullptr != file )
	{
		strcpy( rv->m_File, file );
	}
	else
	{
		rv->m_File[0] = '\0';
	}

	rv->m_Thread = inThread;

	time( reinterpret_cast< time_t* >( &rv->m_Time ) );

	return rv;
}

S_Message* S_Message::Deserialize( T_SourceID sourceId, uint8_t const* data, size_t dataSize )
{
	S_Message* rv = new S_Message( sourceId );

	uint8_t const* end = data + dataSize;

	rv->m_Type = static_cast< trace::E_TracedMessageType >( *data ); 
	++data;

	uint16_t lenMsg = *reinterpret_cast< uint16_t const* >( data ); 
	data += 2;

	uint8_t lenFile = *reinterpret_cast< uint8_t const* >( data ); 
	++data;

	uint8_t lenFn = *reinterpret_cast< uint8_t const* >( data ); 
	++data;

	if ( data + lenMsg + lenFile + lenFn + 16 > end )
	{
		delete rv;
		return nullptr;
	}

	if ( lenMsg > 0 )
	{
		memcpy( rv->m_Msg, data, lenMsg );
		rv->m_Msg[lenMsg] = '\0';
		data += lenMsg;
	}
	else
	{
		rv->m_Msg[0] = '\0';
	}

	if ( lenFile > 0 )
	{
		memcpy( rv->m_File, data, lenFile );
		rv->m_File[lenFile] = '\0';
		data += lenFile;
	}
	else
	{
		rv->m_File[0] = '\0';
	}

	if ( lenFn > 0 )
	{
		memcpy( rv->m_Fn, data, lenFn );
		rv->m_Fn[lenFn] = '\0';
		data += lenFn;
	}
	else
	{
		rv->m_Fn[0] = '\0';
	}

	rv->m_Line = *reinterpret_cast< uint32_t const* >( data );
	data += 4;

	rv->m_Thread = *reinterpret_cast< uint32_t const* >( data );
	data += 4;

	rv->m_Time = *reinterpret_cast< uint64_t const* >( data );
	data += 8;

	if ( rv->m_Time == 0 )
	{
		time( reinterpret_cast< time_t* >( &rv->m_Time ) );
	}

	return rv;
}