#pragma once

class MyAuiTabArt : public wxAuiSimpleTabArt
{
public:

	MyAuiTabArt( wxIcon _sys_tab_ico, wxIcon _sys_tab_active_ico );

	virtual wxAuiTabArt* Clone() override;

	virtual void DrawTab(wxDC& dc,
                 wxWindow* wnd,
                 const wxAuiNotebookPage& pane,
                 const wxRect& inRect,
                 int closeButtonState,
                 wxRect* out_tab_rect,
                 wxRect* out_button_rect,
                 int* xExtent) override;

private:

	wxBitmap m_SysTabImg;
	wxBitmap m_SysTabActiveImg;

};