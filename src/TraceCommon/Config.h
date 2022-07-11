#pragma once
#include "Common.h"

#define TRACER_SYNC_NAME "traceserver"
#define TRACER_PIPE_NAME "\\\\.\\pipe\\tracepipe"

 namespace trace 
 {
	// System message types 
	static TracedMessageType const TMT_Greetings	= 0x08;
	static TracedMessageType const TMT_Farewell		= 0x10;
	static TracedMessageType const TMT_NewThread	= 0x20;
 }