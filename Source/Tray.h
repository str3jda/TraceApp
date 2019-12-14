//--------------------------------------------------------------------------------------------------------------------------
// Filename    : Tray.h
// Author      : Roman 'Str3jda' Truhlar
// Created     : 11.1.2014
// Description : 
//--------------------------------------------------------------------------------------------------------------------------
#ifndef TRACESERVER_TRAY_H
#define TRACESERVER_TRAY_H

class C_MainWindow;

class C_Tray : public wxTaskBarIcon
{
public:
	C_Tray(C_MainWindow* inMainWindow);
	~C_Tray();

	virtual wxMenu* CreatePopupMenu();

private:
	enum E_TrayEvent
	{
		E_TE_SHOW = 1,
		E_TE_CLOSE,
		E_TE_RESET_POS
	};

	void OnClose(wxCommandEvent& event);
	void OnShow(wxCommandEvent& event);
	void OnResetPos(wxCommandEvent& event);
	void OnDblClick(wxTaskBarIconEvent& event);

private:
	C_MainWindow*	m_MainWindow;

private:
	wxDECLARE_EVENT_TABLE();
};

#endif // #ifndef TRACESERVER_TRAY_H