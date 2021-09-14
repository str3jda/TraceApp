#include "stdafx.h"
#include "FormatedRenderer.h"
#include <malloc.h>

namespace details {

	uint32_t htoi( char const* str )
	{
		uint32_t result = 0;
		if ( *str == '0' && *( str + 1 ) == 'X' )
		{
			str += 2;
		}
		while ( *str )
		{
			if ( *str >= 'a' && *str <= 'f' )
			{
				result <<= 4;
				result |= ( *str - 'a' ) + 10;
			}
			else if ( *str >= 'A' && *str <= 'F' )
			{
				result <<= 4;
				result |= ( *str - 'A' ) + 10;
			}
			else if ( *str >= '0' && *str <= '9' )
			{
				result <<= 4;
				result |= ( *str - '0' );
			}
			++str;
		}
		return result;
	}

}

// -------------------------------------------------------------------------------------------------------------------------

C_FormatedRenderer::C_FormatedRenderer(wxDataViewCellMode mode, int align)
	: wxDataViewTextRenderer(wxT("string"), mode, align)
{

}

// -------------------------------------------------------------------------------------------------------------------------

bool C_FormatedRenderer::Render(wxRect rect, wxDC *dc, int state)
{
	//dc->SetTextForeground(wxColor(0xff0000));
	//dc->DrawText(m_text, rect.GetLeftTop());

	wxColor defColor(dc->GetTextForeground());
	wxColor actColor = defColor;

	size_t len		= m_text.length();
	char* text		= (char*)alloca(len + 1);
	strncpy( text, m_text.c_str(), len + 1 );

	char* const end = text + len;
	char* now		= text;

	while (*now)
	{
		while (*now && *now != '{')
		{
			if ('\t' == *now)
			{
				rect.Offset(18, 0);
				*now = ' ';
			}
			++now;
		}

		if (now != text)
		{
			*now = '\0';
			wxString wxText(text);

			dc->SetTextForeground(actColor);
			dc->DrawText(wxText, rect.GetLeftTop());

			if (now == end)
			{
				break;
			}

			wxSize size = dc->GetTextExtent(wxText);
			rect.Offset(size.GetX(), 0);
		}

		char type = now[1];
		switch (type)
		{
		case 'c':
			{
				char* colEnd = now + 3;
				while (*colEnd != ':')
				{
					++colEnd;
				}

				*colEnd = '\0';
				uint32_t c = details::htoi(now + 3);
				actColor = wxColor(uint8_t((c>>16) & 0xff), uint8_t((c>>8) & 0xff), uint8_t(c & 0xff));

				now = colEnd + 1;
				text = now;
				while (*now && *now != '}')
				{
					++now;
				}

				*now = '\0';
				wxString wxText(text);

				dc->SetTextForeground(actColor);
				dc->DrawText(wxText, rect.GetLeftTop());

				wxSize size = dc->GetTextExtent(wxText);
				rect.Offset(size.GetX(), 0);

				++now;
				text		= now;
				actColor	= defColor;
			}
			break;

		default:
			text = now;
			*now = '{';
			++now;

			break;
		}
	}

	return true;
}