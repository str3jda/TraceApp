//--------------------------------------------------------------------------------------------------------------------------
// Filename    : FilterOptions.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 16.1.2016
// Description : 
//--------------------------------------------------------------------------------------------------------------------------

#pragma once
#include <App/UI/UiMessage.h>

struct FilterOptions
{
	bool CaseSensitive = false;
	bool GreyOutFilteredOut = true;
	std::string	Text;
	uint32_t TypeFlags = ( 1 << trace::TMT_Information ) | ( 1 << trace::TMT_Log ) | ( 1 << trace::TMT_Warning ) | ( 1 << trace::TMT_Error );
	uint16_t MsgLimit = 0xffff;

	bool operator==( FilterOptions const& _other ) const
	{
		return  
			CaseSensitive == _other.CaseSensitive &&
			GreyOutFilteredOut == _other.GreyOutFilteredOut &&
			Text == _other.Text && 
			MsgLimit == _other.MsgLimit &&
			TypeFlags == _other.TypeFlags &&
			MsgLimit == _other.MsgLimit;
	}
};


class MsgFilter
{
public:

	bool Apply( FilterOptions const& options );

	MessageFlags_t FilterMsgText( wxString const& txt, trace::TracedMessageType_t type ) const;

	uint16_t GetMsgCountLimit() const { return m_CurrentOptions.MsgLimit == 0 ? 0xffff : m_CurrentOptions.MsgLimit; }

private:

	void ResolveFilterOut( MessageFlags_t& _flags ) const;

private:

	FilterOptions m_CurrentOptions;
	std::string m_MsgTextCSCached;
	mutable char m_FilterBuffer[2048];


};