#include "TraceClient.h"

#ifdef TS_PLATFORM_WINDOWS
#	define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#	include <windows.h>
#endif

#include <time.h>
#include <stdint.h>

#define TRACER_SYNC_NAME "traceserver"
#define TRACER_PIPE_NAME "\\\\.\\pipe\\tracepipe"

namespace trace {

	C_TraceClient::C_TraceClient()
		: m_Pipe(INVALID_HANDLE_VALUE)
	{
	}

	C_TraceClient::~C_TraceClient()
	{
		if (INVALID_HANDLE_VALUE != m_Pipe)
		{
			CloseHandle(m_Pipe); 
		}
	}

	bool C_TraceClient::Connect( T_TraceAppID appId )
	{
		if (!CreatePipe())
		{
			return false;
		}

		// send greetings message
		char appName[MAX_PATH];
		char* toSend;
#ifdef TS_PLATFORM_WINDOWS
		GetModuleFileNameA(0, appName, MAX_PATH);
#else
#  error get app name somehow
#endif // #ifdef WIN32
		char* sz = strrchr(appName,'\\');
		if (sz)
		{
			toSend = sz + 1;
		}
		else
		{
			toSend = appName;
		}

		m_AppID = appId;
    
		if (!LogMessage(TMT_Greetings, toSend, nullptr, nullptr, 0))
		{
			return false;
		}

		return true;
	}

	void C_TraceClient::Disconnect()
	{
#ifdef TS_PLATFORM_WINDOWS
		if (INVALID_HANDLE_VALUE != m_Pipe)
		{
			// send farewell message
			LogMessage(TMT_Farewell, nullptr, nullptr, nullptr, 0);
			//FlushFileBuffers(m_Pipe);

			CloseHandle(m_Pipe);
			m_Pipe = INVALID_HANDLE_VALUE;
		}
#endif
	}

	bool C_TraceClient::CreatePipe()
	{
#ifdef WIN32
		while ( true )
		{
			m_Pipe = CreateFileA(
				TRACER_PIPE_NAME, 
				GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if (m_Pipe != INVALID_HANDLE_VALUE) 
			{
				break;
			}

			if (GetLastError() != ERROR_PIPE_BUSY)
			{ // Failed to create pipe
				return false;
			}

			if (!WaitNamedPipeA(TRACER_PIPE_NAME, 5000))
			{
				return false;
			}
		}

		DWORD dwMode = PIPE_READMODE_MESSAGE; 
		BOOL success = SetNamedPipeHandleState(m_Pipe, &dwMode, NULL, NULL);
		if (!success) 
		{
			return false;
		}

		return true;
#else
		return false;
#endif
	}

	bool C_TraceClient::LogMessage( E_TracedMessageType inType, const char* inMessage, const char* inFile, const char* inFunction, unsigned int inLine)
	{
		if (INVALID_HANDLE_VALUE == m_Pipe)
		{
			return false;
		}
	
		uint32_t threadId = GetCurrentThreadId();

		uint8_t buffer[1024];
		uint8_t* p = buffer;

		*reinterpret_cast<uint32_t*>( p ) = m_AppID;	p += sizeof( uint32_t );
	
		*p = inType;        ++p;

		uint32_t lenMsg		= ( uint32_t )strlen(inMessage);
		uint32_t lenFile	= ( uint32_t )strlen(inFile);
		uint32_t lenFn		= ( uint32_t )strlen(inFunction);

		if (nullptr != inFile)
		{
			char const* end = inFile + lenFile;
			while (end >= inFile && *end != '\\')
			{
				--end;
			}

			if (end != inFile)
			{
				lenFile -= ( uint32_t )(end - inFile + 1);
				inFile = end + 1;
			}
			else
			{
				lenFile -= ( uint32_t )(end - inFile);
			}
		}

		if (lenMsg >= 0xffff || lenFile >= 0xff || lenFn >= 0xff || 
			( lenMsg  + lenFile + lenFn + 21 ) > sizeof( buffer ) )
		{
			return false;
		}
    
		*reinterpret_cast<uint16_t*>(p) = ( uint16_t )lenMsg;	p += sizeof( uint16_t );
		*reinterpret_cast<uint8_t*>(p) = ( uint8_t )lenFile;	p += sizeof( uint8_t );
		*reinterpret_cast<uint8_t*>(p) = ( uint8_t )lenFn;		p += sizeof( uint8_t );
	
		memcpy(p, inMessage, lenMsg);				p += lenMsg;
		memcpy(p, inFile, lenFile);					p += lenFile;
		memcpy(p, inFunction, lenFn);				p += lenFn;

		*reinterpret_cast<uint32_t*>(p) = inLine;		p += sizeof( uint32_t );
		*reinterpret_cast<uint32_t*>(p) = threadId;		p += sizeof( uint32_t );

		uint64_t* t = reinterpret_cast<uint64_t*>(p);
		*t = 0;

		time(reinterpret_cast<time_t*>(t));			p += sizeof( uint64_t );
    
		DWORD written;
		DWORD toWrite = (DWORD)(p - buffer);
#ifdef TS_PLATFORM_WINDOWS
		return WriteFile(m_Pipe, buffer, toWrite, &written, NULL) && written == toWrite;
#else
		return false;
#endif
	}


}