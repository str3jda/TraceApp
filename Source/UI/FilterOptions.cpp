#include "stdafx.h"
#include "FilterOptions.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace details {

	uint32_t get_sum( char const* p, size_t& uLen )
	{
		char const* p0 = p;
		uint32_t uSum = 0;
		while ( *p )
			uSum += *p++;
		uLen = ( size_t )( p - p0 );
		return uSum;
	}

	char const* find_text( char const* from, size_t uFromSize, char const* szSearch )
	{
		if ( 0 == szSearch || 0 == *szSearch )
			return 0;
		size_t uLen;
		uint32_t uSearchHash = get_sum( szSearch, uLen );
		if ( uLen > uFromSize )
			return 0;
		size_t i = 0;
		uint32_t uHash = 0;
		char const* p0 = from, *p1 = from, *pTmp, *pS;

		for ( ; i < uLen; ++i )
			uHash += *p1++;
		do
		{
			if ( uHash == uSearchHash )
			{//checking whether its really searched string and not just the same hash...
				pTmp = p0;
				pS = szSearch;
				while ( *pS )
					if ( *pTmp++ != *pS++ )
						goto gogo;
				return p0;
			}
		gogo:
			uHash += *p1;
			uHash -= *p0++;
		} while ( *p1++ );
		return 0;
	}
}

// -------------------------------------------------------------------------------------------------------------------------

C_MsgFilter::C_MsgFilter()
	: m_RegExpr( nullptr )
{
}

// -------------------------------------------------------------------------------------------------------------------------

C_MsgFilter::~C_MsgFilter()
{
	if ( m_RegExpr != nullptr )
	{
		tre_regfree( m_RegExpr );
		delete m_RegExpr;
	}
}

// -------------------------------------------------------------------------------------------------------------------------

bool C_MsgFilter::Apply( S_FilterOptions const& options )
{
	if ( m_CurrentOptions.m_CaseSensitive == options.m_CaseSensitive &&
		 m_CurrentOptions.m_Regex == options.m_Regex &&
		 m_CurrentOptions.m_Text == options.m_Text && 
		 m_CurrentOptions.m_MsgLimit == options.m_MsgLimit &&
		 m_CurrentOptions.m_TypeFlags == options.m_TypeFlags )
	{
		return false;
	}

	if ( m_RegExpr != nullptr )
	{
		tre_regfree( m_RegExpr );
		delete m_RegExpr;
		m_RegExpr = nullptr;
	}

	if ( options.m_Regex )
	{
		m_RegExpr = new regex_t();

		if ( REG_OK != tre_regcomp( m_RegExpr, options.m_Text.c_str(), REG_EXTENDED | ( options.m_CaseSensitive ? 0 : REG_ICASE ) ) )
		{
			delete m_RegExpr;
			m_RegExpr = nullptr;
			return false;
		}
	}

	m_CurrentOptions = options;

	m_MsgTextCSCached = m_CurrentOptions.m_Text;
	std::transform( m_MsgTextCSCached.begin(), m_MsgTextCSCached.end(), m_MsgTextCSCached.begin(), ::tolower );

	return true;
}

// -------------------------------------------------------------------------------------------------------------------------

bool C_MsgFilter::FilterMsgText( wxString const& txt, trace::E_TracedMessageType type ) const
{
	if ( ( m_CurrentOptions.m_TypeFlags & type ) == 0 )
	{
		return false;
	}

	if ( !m_CurrentOptions.m_Text.empty() )
	{
		if ( m_CurrentOptions.m_Regex )
		{
			if ( REG_NOMATCH == tre_regexec( m_RegExpr, txt.c_str().AsChar(), 0, nullptr, 0 ) )
			{
				return false;
			}
		}
		else if ( m_CurrentOptions.m_CaseSensitive )
		{
			if ( details::find_text( txt.c_str().AsChar(), txt.length(), m_CurrentOptions.m_Text.c_str() ) == nullptr )
			{
				return false;
			}
		}
		else
		{
			size_t len = txt.length();
			char const* txtChars = txt.c_str().AsChar();

			for ( size_t i = 0; i < len; ++i )
			{
				m_FilterBuffer[ i ] = (char)::tolower( txtChars[i] );
			}

			m_FilterBuffer[ len ] = '\0';

			if ( details::find_text( m_FilterBuffer, len, m_MsgTextCSCached.c_str() ) == nullptr )
			{
				return false;
			}
		}
	}

	return true;
}