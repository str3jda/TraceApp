//--------------------------------------------------------------------------------------------------------------------------
// Filename    : FilterOptions.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 16.1.2016
// Description : 
//--------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <tre/tre.h>

struct S_FilterOptions
{
	S_FilterOptions()
		: m_CaseSensitive( true )
		, m_Regex( false )
		, m_TypeFlags( trace::TMT_Information | trace::TMT_Warning | trace::TMT_Error )
		, m_MsgLimit( 0xffff )
	{}

	// Message specific
	bool m_CaseSensitive;
	bool m_Regex;
	std::string	m_Text;
	uint32_t m_TypeFlags; // E_TracedMessageType
	uint16_t m_MsgLimit;
};


class C_MsgFilter
{
public:
	
	C_MsgFilter();
	~C_MsgFilter();

	bool Apply( S_FilterOptions const& options );

	bool FilterMsgText( wxString const& txt, trace::E_TracedMessageType type ) const;

	uint16_t GetMsgCountLimit() const { return m_CurrentOptions.m_MsgLimit == 0 ? 0xffff : m_CurrentOptions.m_MsgLimit; }

private:

	S_FilterOptions	m_CurrentOptions;
	std::string m_MsgTextCSCached;
	mutable char m_FilterBuffer[2048];

	regex_t* m_RegExpr;


};