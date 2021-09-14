#include "stdafx.h"
#include "wxNotebookEx.h"

// -------------------------------------------------------------------------------------------------------------------------
wxDEFINE_EVENT(wxEVT_NOTEBOOK_PAGE_CLOSE, wxBookCtrlEvent);
// -------------------------------------------------------------------------------------------------------------------------
wxBEGIN_EVENT_TABLE(wxNotebookEx, wxAuiNotebook)
	EVT_AUINOTEBOOK_PAGE_CLOSE( wxID_ANY, wxNotebookEx::OnPageClose)
wxEND_EVENT_TABLE()
// -------------------------------------------------------------------------------------------------------------------------

void wxNotebookEx::OnPageClose(wxAuiNotebookEvent& event)
{
	int page = event.GetSelection();
	if (page >= 0)
	{
		wxBookCtrlEvent clbkEvent(wxEVT_NOTEBOOK_PAGE_CLOSE, GetId(), GetSelection());
		clbkEvent.SetEventObject( this );

		ProcessWindowEvent( clbkEvent );

		if ( !clbkEvent.IsAllowed() )
		{
			event.Veto();
		}
	}
}