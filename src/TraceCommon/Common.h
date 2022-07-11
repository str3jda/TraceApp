#pragma once
#include <stdint.h>

 namespace trace
 {

	 using TraceAppID = uint32_t;
	 using TracedMessageType = uint8_t;

	 static TracedMessageType const TMT_Information		= 0x01;
	 static TracedMessageType const TMT_Warning			= 0x02;
	 static TracedMessageType const TMT_Error			= 0x04;

 }