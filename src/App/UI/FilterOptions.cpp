#include <App/stdafx.h>
#include <App/UI/FilterOptions.h>

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

bool MsgFilter::Apply( FilterOptions const& options )
{
	if ( m_CurrentOptions == options )
		return false;

	m_CurrentOptions = options;

	m_MsgTextCSCached = m_CurrentOptions.Text;
	std::transform( m_MsgTextCSCached.begin(), m_MsgTextCSCached.end(), m_MsgTextCSCached.begin(), ::tolower );

	return true;
}

// -------------------------------------------------------------------------------------------------------------------------

MessageFlags_t MsgFilter::FilterMsgText( wxString const& txt, trace::TracedMessageType_t type ) const
{
	MessageFlags_t rv = E_MF_Visible;

	if ( ( m_CurrentOptions.TypeFlags & ( 1 << type ) ) == 0 )
		ResolveFilterOut( rv );

	if ( !m_CurrentOptions.Text.empty() )
	{
		if ( m_CurrentOptions.CaseSensitive )
		{
			if ( details::find_text( txt.c_str().AsChar(), txt.length(), m_CurrentOptions.Text.c_str() ) == nullptr )
				ResolveFilterOut( rv );
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
				ResolveFilterOut( rv );
		}
	}

	return rv;
}

void MsgFilter::ResolveFilterOut( MessageFlags_t& _flags ) const
{
	if ( m_CurrentOptions.GreyOutFilteredOut )
	{
		_flags |= E_MF_GreydOut;
	}
	else
		_flags &= ~E_MF_Visible;
}