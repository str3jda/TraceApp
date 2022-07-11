//--------------------------------------------------------------------------------------------------------------------------
// Filename    : FilterOptions.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 16.1.2016
// Description : 
//--------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <regex.hpp>

struct S_FilterOptions
{
	S_FilterOptions()
		: m_CaseSensitive( true )
		, m_TypeFlags( trace::TMT_Information | trace::TMT_Warning | trace::TMT_Error )
		, m_MsgLimit( 0xffff )
	{}

	// Message specific
	bool m_CaseSensitive;
	std::string	m_Text;
	uint32_t m_TypeFlags; // TracedMessageType
	uint16_t m_MsgLimit;
};


class C_MsgFilter
{
public:

	bool Apply( S_FilterOptions const& options );

	bool FilterMsgText( wxString const& txt, trace::TracedMessageType type ) const;

	uint16_t GetMsgCountLimit() const { return m_CurrentOptions.m_MsgLimit == 0 ? 0xffff : m_CurrentOptions.m_MsgLimit; }

private:

	S_FilterOptions	m_CurrentOptions;
	std::string m_MsgTextCSCached;
	mutable char m_FilterBuffer[2048];


};