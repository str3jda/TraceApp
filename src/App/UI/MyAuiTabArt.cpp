#include <App/UI/MyAuiTabArt.h>
#include <App/UI/MainWindow.h>
#include <App/UI/ListView.h>
#include <App/Theme.h>
#include <bit>

MyAuiTabArt::MyAuiTabArt( wxIcon _sys_tab_ico, wxIcon _sys_tab_active_ico )
{
	wxBitmapBundle btm( _sys_tab_ico );
	m_SysTabImg = btm.GetBitmap( wxSize( 16, 16 ) );

	wxBitmapBundle btm2( _sys_tab_active_ico );
	m_SysTabActiveImg = btm2.GetBitmap( wxSize( 16, 16 ) );
}

wxAuiTabArt* MyAuiTabArt::Clone()
{
	return new MyAuiTabArt( *this );
}

void MyAuiTabArt::DrawTab(wxDC& dc,
				wxWindow* wnd,
				const wxAuiNotebookPage& page,
				const wxRect& in_rect,
				int closeButtonState,
				wxRect* out_tab_rect,
				wxRect* out_button_rect,
				int* x_extent)
{
	ListView* list = std::bit_cast< ListView* >( page.window->GetClientData() );

	if ( list->SystemView )
	{
		*x_extent += 15;
		wxSize tab_size = { 20, 20 };

		wxCoord tab_height = tab_size.y;
		wxCoord tab_width = tab_size.x;
		wxCoord tab_x = in_rect.x;
		wxCoord tab_y = in_rect.y + in_rect.height - tab_height;

		dc.SetClippingRegion( in_rect );		
		dc.DrawBitmap( page.hover || page.active ? m_SysTabActiveImg : m_SysTabImg, tab_x + 1, tab_y + 1, true );
    	dc.DestroyClippingRegion();
		
		*out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);
	}
	else
	{
		wxAuiSimpleTabArt::DrawTab( dc, wnd, page, in_rect, closeButtonState, out_tab_rect, out_button_rect, x_extent );
		*x_extent += 4;
	}
}
