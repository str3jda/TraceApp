#include <App/stdafx.h>
#include <App/UI/FormatedRenderer.h>
#include <App/UI/UiMessage.h>
#include <Trace/Message.h>
#include <malloc.h>

namespace details {

	uint8_t htoi( char _char )
	{
		if ( _char >= 'a' && _char <= 'f' ) return ( _char - 'a' ) + 10;
		else if ( _char >= 'A' && _char <= 'F' ) return ( _char - 'A' ) + 10;
		else if ( _char >= '0' && _char <= '9' ) return ( _char - '0' );
		else return 0;
	}
	
	struct BlockFmt
	{
		wxColor FontColor;
		wxColor FontBackground;
		bool Bold = false;
		bool Italic = false;
		bool Underlined = false;

		bool IsFontChanged( BlockFmt const& _other ) const 
		{ 
			return ( Bold != _other.Bold ) || ( Italic != _other.Italic ) || ( Underlined != _other.Underlined ); 
		}
	};

	struct TextParser
	{
		TextParser( char const* _text, size_t _text_length, BlockFmt const& _default_fmt )
			: m_DefaultFmt( _default_fmt )
			, m_Iter( _text )
			, m_End( _text + _text_length )
		{
		}

		bool IsTrivial() const { return FindBlock( trace::BLOCK_START_FORMAT ) == m_End; }

		bool BeginBlock( wxString** _out_text, BlockFmt& _out_fmt )
		{
			_out_fmt = m_DefaultFmt;
			char const* start_block = m_Iter;

			if ( start_block == m_End )
				return false;

			char const* next_block;

			if ( TestMark( start_block, trace::BLOCK_START_FORMAT ) )
			{
				next_block = FindBlock( trace::BLOCK_END_FORMAT );
				if ( next_block == m_End )
					return false;

				std::string_view fmt_str = ParseFmt( start_block, next_block, _out_fmt );
				m_ScratchPad.assign( fmt_str.data(), fmt_str.length() );

				next_block += sizeof( trace::BLOCK_END_FORMAT ) - 1;
			}
			else
			{
				next_block = FindBlock( trace::BLOCK_START_FORMAT );
				m_ScratchPad.assign( start_block, next_block );
			}			

			*_out_text = &m_ScratchPad;
			m_Iter = next_block;

			return true;		
		}

	private:

		std::string_view ParseFmt( char const* _start_block, char const* _next_block, BlockFmt& _fmt ) const
		{
			char const* const key_start = _start_block + sizeof( trace::BLOCK_START_FORMAT ) - 1;
			char const* key_end = FindChar( key_start, ':' );

			char const* text_start;
			bool const has_value = key_end < _next_block;

			if ( has_value )
			{
				char const* const value_start = key_end + 1;
				char const* const value_end = FindChar( value_start, '|' );

				if ( value_end >= _next_block )
					return {};
			
				text_start = value_end + 1;

				if ( 
					( 
						( *key_start == trace::BLOCK_FMT_FONT_COLOR ) || 
						( *key_start == trace::BLOCK_FMT_FONT_BACK_COLOR )
					) && ( key_start + 1 == key_end ) )
				{
					wxColor color;
					size_t len = ( value_end - value_start );
					if ( len == 3 )
					{
						color.Set( 
							htoi( value_start[0] ) << 4,
							htoi( value_start[1] ) << 4,
							htoi( value_start[2] ) << 4 );
					}
					else if ( len == 6 )
					{
						color.Set( 
							( htoi( value_start[0] ) << 4 ) | htoi( value_start[1] ),
							( htoi( value_start[2] ) << 4 ) | htoi( value_start[3] ),
							( htoi( value_start[4] ) << 4 ) | htoi( value_start[5] ) );
					}

					if ( *key_start == trace::BLOCK_FMT_FONT_COLOR )
						_fmt.FontColor = color;
					else if ( *key_start == trace::BLOCK_FMT_FONT_BACK_COLOR )
						_fmt.FontBackground = color;
				}
			}
			else
			{
				key_end = FindChar( key_start + 1, '|' );
				if ( key_end >= _next_block )
					return {};

				text_start = key_end + 1;

				if ( ( *key_start == trace::BLOCK_FMT_FONT_BOLD ) && ( key_start + 1 == key_end ) )
					_fmt.Bold = true;
				else if ( ( *key_start == trace::BLOCK_FMT_FONT_ITALIC ) && ( key_start + 1 == key_end ) )
					_fmt.Italic = true;
				else if ( ( *key_start == trace::BLOCK_FMT_FONT_UNDERLINED ) && ( key_start + 1 == key_end ) )
					_fmt.Underlined = true;
			}

			char const* const text_end = _next_block;

			return { text_start, text_end };
		}

		template < size_t TMarkSize >
		static bool TestMark( char const* _str, char const ( &_mark )[TMarkSize] ) 
		{ 
			if      constexpr ( TMarkSize == 2 ) return _str[0] == _mark[0];
			else if constexpr ( TMarkSize == 3 ) return *std::bit_cast< uint16_t const* >( _str ) == *std::bit_cast< uint16_t const* >( &_mark[0] );
			else static_assert( false, "TMarkSize is unsupported" );
		}

		template < size_t TMarkSize >
		char const* FindBlock( char const ( &_mark )[TMarkSize] ) const
		{
			char const* x = m_Iter;
			
			// It can read behind the memory, but it shouldn't matter. There's always '\0', so it
			// will never actually find anything there
			while ( ( x < m_End ) && !TestMark( x, _mark ) )
				x++;

			return x;
		}

		char const* FindChar( char const* _from, char c ) const
		{
			while ( ( _from != m_End ) && ( *_from != c )  )
				_from++;

			return _from;
		}

		BlockFmt const m_DefaultFmt;

		wxString m_ScratchPad;
		char const* m_Iter;
		char const* const m_End;
	}; 
}

C_FormatedRenderer::C_FormatedRenderer(wxDataViewCellMode mode, int align)
	: wxDataViewTextRenderer(wxT("void*"), mode, align)
{
}

bool C_FormatedRenderer::SetValue( const wxVariant &value )
{
	m_CurrentMessage = reinterpret_cast< UiMessage const* >( value.GetVoidPtr() );
	return true;
}

bool C_FormatedRenderer::Render(wxRect rect, wxDC *dc, int state)
{
	details::BlockFmt const default_fmt{ dc->GetTextForeground(), dc->GetTextBackground(), false, false, false };
	bool use_user_formating = ( m_CurrentMessage->Flags & E_MF_GreydOut ) == 0;

	if ( !use_user_formating )
	{
		wxColour const full_color = default_fmt.FontColor;
		wxColour const no_color = default_fmt.FontBackground;

		static constexpr float LERP_FACTOR = 0.8f;

		auto lerp = []( float a, float b, float f )
		{
			return static_cast< uint8_t >( a + ( b - a ) * f );
		};

		wxColour greyd( 0xff000000 );
		greyd.Set(
			lerp( full_color.Red(), no_color.Red(), LERP_FACTOR ),
			lerp( full_color.Green(), no_color.Green(), LERP_FACTOR ),
			lerp( full_color.Blue(), no_color.Blue(), LERP_FACTOR )
		);

		dc->SetTextForeground( greyd );
	}

	details::TextParser parser( m_CurrentMessage->Msg.c_str(), m_CurrentMessage->Msg.size(), default_fmt );

	int const default_bg_mode = dc->GetBackgroundMode();

	if ( parser.IsTrivial() )
	{
		dc->DrawText( m_CurrentMessage->Msg, rect.GetLeftTop() );
	}
	else
	{
		wxFont default_font = dc->GetFont();
		details::BlockFmt last_fmt = default_fmt;

		wxString* text;
		details::BlockFmt fmt;
		
		while ( parser.BeginBlock( &text, fmt ) )
		{
			if ( use_user_formating ) 
			{
				dc->SetTextForeground( fmt.FontColor );

				if ( fmt.FontBackground != default_fmt.FontBackground )
				{
					dc->SetBackgroundMode( wxBRUSHSTYLE_SOLID );
					dc->SetTextBackground( fmt.FontBackground );
				}
				else
				{
					dc->SetBackgroundMode( default_bg_mode );
					dc->SetTextBackground( default_fmt.FontBackground );
				}
						
				if ( last_fmt.IsFontChanged( fmt ) )
				{
					last_fmt = fmt;

					if ( fmt.Bold )
						dc->SetFont( GetFontBold( dc ) );
					else if ( fmt.Italic )
						dc->SetFont( GetFontItalic( dc ) );
					else if ( fmt.Underlined )
						dc->SetFont( GetFontUnderlined( dc ) );
					else
						dc->SetFont( default_font );
				}
			}

			dc->DrawText( *text, rect.GetLeftTop() );
			
			wxSize size = dc->GetTextExtent( *text );
			rect.Offset( size.GetX(), 0 );
		}

		if ( last_fmt.IsFontChanged( default_fmt ) )
			dc->SetFont( default_font );

		dc->SetBackgroundMode( default_bg_mode );
		dc->SetTextBackground( default_fmt.FontBackground );
	}
	
	return true;
}

 wxString C_FormatedRenderer::StripFormatting( wxString const& _text )
 {
	details::TextParser parser( _text.c_str(), _text.size(), {} );

	if ( parser.IsTrivial() )
	{
		return _text;
	}
	else
	{
		wxString rv;
		wxString* text;
		details::BlockFmt fmt;
		
		while ( parser.BeginBlock( &text, fmt ) )
			rv += *text;

		return rv;
	}
 }

wxFont& C_FormatedRenderer::GetFontBold( wxDC* dc )
{
	if ( !m_FontBold.Ok() ) 
		m_FontBold = dc->GetFont().Bold();

	return m_FontBold;
}

wxFont& C_FormatedRenderer::GetFontItalic( wxDC* dc )
{
	if ( !m_FontItalic.Ok() ) 
		m_FontItalic = dc->GetFont().Italic();

	return m_FontItalic;
}

wxFont& C_FormatedRenderer::GetFontUnderlined( wxDC* dc )
{
	if ( !m_FontUnderlined.Ok() ) 
		m_FontUnderlined = dc->GetFont().Underlined();

	return m_FontUnderlined;
}