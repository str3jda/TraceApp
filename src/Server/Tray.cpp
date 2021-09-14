#include "stdafx.h"
#include "Tray.h"
#include "Resource.h"
#include "UI/mainwindow.h"

// -------------------------------------------------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(C_Tray, wxTaskBarIcon) 
	EVT_MENU(E_TE_SHOW, C_Tray::OnShow) 
	EVT_MENU(E_TE_CLOSE, C_Tray::OnClose) 
	EVT_MENU(E_TE_RESET_POS, C_Tray::OnResetPos)
	EVT_TASKBAR_LEFT_DCLICK(OnDblClick)
END_EVENT_TABLE() 
// -------------------------------------------------------------------------------------------------------------------------

C_Tray::C_Tray(C_MainWindow* inMainWindow)
	: m_MainWindow(inMainWindow)
{
	if (wxTaskBarIcon::IsAvailable())
	{
		HICON hico = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_TRACERSERVER));
		if (NULL != hico)
		{
			wxIcon ico;
			if (ico.CreateFromHICON(hico))
			{
				SetIcon(ico, "Trace Server");
			}

			DestroyIcon(hico);
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------

C_Tray::~C_Tray()
{
}

// -------------------------------------------------------------------------------------------------------------------------

wxMenu* C_Tray::CreatePopupMenu()
{
	wxMenu* menu = new wxMenu();
	menu->Append(E_TE_SHOW, "Show", "Focuses window", false);
	menu->AppendSeparator();
	menu->Append(E_TE_CLOSE, "Close", "Close app", false);
	menu->Append(E_TE_RESET_POS, "Reset position", "Reset position", false);

	return menu;
}

// -------------------------------------------------------------------------------------------------------------------------

void C_Tray::OnClose(wxCommandEvent& /*event*/)
{
	m_MainWindow->Close(true);
}

// -------------------------------------------------------------------------------------------------------------------------

void C_Tray::OnShow(wxCommandEvent& /*event*/)
{
	m_MainWindow->Restore();
	m_MainWindow->Show(true);
}

// -------------------------------------------------------------------------------------------------------------------------

void C_Tray::OnResetPos(wxCommandEvent& /*event*/)
{
	m_MainWindow->Move(0, 0);
}

// -------------------------------------------------------------------------------------------------------------------------

void C_Tray::OnDblClick(wxTaskBarIconEvent& /*event*/)
{
	m_MainWindow->Restore();
	m_MainWindow->Show(true);
}

