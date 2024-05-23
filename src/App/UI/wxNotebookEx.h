//--------------------------------------------------------------------------------------------------------------------------
// Filename    : wxNotebookEx.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 11.1.2014
// Description : 
//--------------------------------------------------------------------------------------------------------------------------
#pragma once

class wxNotebookEx : public wxAuiNotebook
{
public:
	wxNotebookEx(wxWindow *parent, wxWindowID winid,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize)
		: wxAuiNotebook(parent, winid, pos, size)
	{
	}

private:
	void OnPageClose(wxAuiNotebookEvent& event);

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(wxEVT_NOTEBOOK_PAGE_CLOSE, wxBookCtrlEvent);

#define EVT_NOTEBOOK_PAGE_CLOSE(func) \
	wx__DECLARE_EVT0(wxEVT_NOTEBOOK_PAGE_CLOSE, wxBookCtrlEventHandler(func))
