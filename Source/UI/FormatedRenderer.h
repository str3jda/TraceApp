//--------------------------------------------------------------------------------------------------------------------------
// Filename    : FormatedRenderer.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 11.1.2014
// Description : 
//--------------------------------------------------------------------------------------------------------------------------
#ifndef TRACESERVER_FORMATEDRENDERER_H
#define TRACESERVER_FORMATEDRENDERER_H

class C_FormatedRenderer : public wxDataViewTextRenderer
{
public:
	C_FormatedRenderer(
		wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT,
		int align = wxDVR_DEFAULT_ALIGNMENT);

	bool Render(wxRect rect, wxDC *dc, int state);
};

#endif // #ifndef TRACESERVER_FORMATEDRENDERER_H