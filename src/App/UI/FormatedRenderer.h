//--------------------------------------------------------------------------------------------------------------------------
// Filename    : FormatedRenderer.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 11.1.2014
// Description : 
//--------------------------------------------------------------------------------------------------------------------------
#ifndef TRACESERVER_FORMATEDRENDERER_H
#define TRACESERVER_FORMATEDRENDERER_H

struct UiMessage;

class C_FormatedRenderer : public wxDataViewTextRenderer
{
public:
	C_FormatedRenderer(
		wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT,
		int align = wxDVR_DEFAULT_ALIGNMENT);

	bool Render(wxRect rect, wxDC *dc, int state) override;

	bool SetValue( const wxVariant &value ) override;

	static wxString StripFormatting( wxString const& _text );

private:

	UiMessage const* m_CurrentMessage = nullptr;

	wxFont& GetFontBold( wxDC* dc );
	wxFont& GetFontItalic( wxDC* dc );
	wxFont& GetFontUnderlined( wxDC* dc );

	wxFont m_FontBold;
	wxFont m_FontItalic;
	wxFont m_FontUnderlined;
};

#endif // #ifndef TRACESERVER_FORMATEDRENDERER_H