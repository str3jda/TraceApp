#include <stdafx.h>
#include "Message.h"
#include <time.h>

S_Message* S_Message::Create( T_SourceID sourceId, trace::TracedMessageType type, uint32_t line, char const* msg, char const* fn, char const* file, uint32_t inThread )
{
	S_Message* rv = new S_Message();
	rv->m_SourceID = sourceId;
	rv->m_Type = type;
	rv->m_Line = line;

	if ( nullptr != msg )
		rv->m_Msg = msg;

	if ( nullptr != fn )
		rv->m_Fn = fn;

	if ( nullptr != file )
		rv->m_File = file;

	rv->m_Thread = inThread;

	time( reinterpret_cast< time_t* >( &rv->m_Time ) );

	return rv;
}

S_Message* S_Message::Deserialize( T_SourceID sourceId, uint8_t const* data, size_t dataSize )
{
	S_Message* rv = new S_Message();
	rv->m_SourceID = sourceId;

	uint8_t const* end = data + dataSize;

	rv->m_Type = static_cast< trace::TracedMessageType >( *data ); 
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
		rv->m_Msg.assign( reinterpret_cast< char const* >( data ), lenMsg );
		data += lenMsg;
	}
	else
	{
		rv->m_Msg[0] = '\0';
	}

	if ( lenFile > 0 )
	{
		rv->m_File.assign( reinterpret_cast< char const* >( data ), lenFile );
		data += lenFile;
	}
	else
	{
		rv->m_File[0] = '\0';
	}

	if ( lenFn > 0 )
	{
		rv->m_Fn.assign( reinterpret_cast< char const* >( data ), lenFn );
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