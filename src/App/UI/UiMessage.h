#pragma once
#include <Trace/Trace.h>

using MessageFlags_t = uint32_t;

static constexpr MessageFlags_t E_MF_Visible 		= 0x01;
static constexpr MessageFlags_t E_MF_GreydOut 		= 0x02;

struct UiMessage
{
	trace::TracedMessageType_t Type;
	MessageFlags_t Flags = 0;

	wxDateTime GTime;
	uint32_t LocalTime;
	uint32_t FileLine;
	ThreadNameHandle Thread = InvalidThreadNameHandle;
	wxString Msg;
	wxString Fn;
	wxString File;

	UiMessage() noexcept = default;
	UiMessage(UiMessage&& _other) noexcept = default;

	UiMessage& operator=(UiMessage&& _other) noexcept = default;
};