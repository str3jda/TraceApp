#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "SettingsDialog.h"
#include "../Tray.h"

#include <unordered_map>

class C_TabModel;
class C_MsgFilter;
class C_ClientOverseer;

struct S_Message;

class C_MainWindow : public wxFrame
{
	struct S_PersistentSettings;

public:

	C_MainWindow();
    ~C_MainWindow();

    wxIcon const& GetIcon(trace::E_TracedMessageType inType);

	void ShowSettingsDialog();
	void ToggleAutoScroll();
	void DeleteCurrentPageContent();

	void OnMessageGreetings( char const* inTitle, T_SourceID id );
	void OnMessageFarewell( T_SourceID id );
	void OnTraceMessage( S_Message const& msg, char const* threadName );

	void LogSystemMessage( char const* text, trace::E_TracedMessageType type = trace::TMT_Information );

private:

	wxDataViewCtrl* CreateTab( char const* inTitle, wxDataViewModel* model );

	void ScrollToBottom( wxWindow* page );

    void            LoadConfig();
    void            SaveConfig();
    void            ApplyConfig( S_PersistentSettings const& p );

	void            ApplySettings( S_SettingParams const& s );

	void			CreateUI();

public:
    void            OnTick(wxTimerEvent& inEvent);
    void            OnTabClose(wxBookCtrlEvent& event);
	void			OnClose(wxCloseEvent& event);
	void			OnKeyPressEvent(wxKeyEvent& event);

	static bool		LoadIco(wxIcon& outIco, DWORD id);

private:

	typedef uint16_t T_SettingsVersion;
	static T_SettingsVersion const E_LatestVersion	= 4;

	struct S_PersistentSettings
	{
		T_SettingsVersion	m_Version;
		bool				m_StayOnTop;
		bool				m_ResetOnProcInstance;
		unsigned short		m_CountLimit;
		int32_t				m_SizeX;
		int32_t				m_SizeY;
		int32_t				m_PosX;
		int32_t				m_PosY;
		char				m_MQTTBroker[64];
	};

	C_ClientOverseer* m_Overseer;
    C_Tray				m_Tray;

	S_SettingParams		m_CurrentSetting;
	C_MsgFilter*		m_MsgFilter;

	struct S_Tab
	{
		C_TabModel*		m_DataModel;
		wxDataViewCtrl* m_ListViewCtrl;
	};

    typedef std::unordered_map< T_SourceID, S_Tab > T_ClientToTabMap;

    T_ClientToTabMap    m_TabMap;
	wxTimer				m_TickTimer;

    wxIcon				m_MsgIcons[3];

	C_TabModel*			m_TabToRefresh;

	struct
	{
//		wxSizer*		m_Sizer;
		wxAuiNotebook*	m_TabControl;
	}					m_UI;

	wxDECLARE_EVENT_TABLE();
};

#endif // MAINWINDOW_H
