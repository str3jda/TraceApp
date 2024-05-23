#pragma once

#include <App/Tray.h>
#include <App/UI/FilterOptions.h>

#include <unordered_map>
#include <mutex>

class C_TabModel;
class C_ClientOverseer;
class ListView;

struct S_SettingParams
{
    bool			m_StayOnTop;
	bool			m_ResetOnProcInstance;
	bool			m_AutoScroll;
	FilterOptions 	m_MsgFilter;
	std::string		m_MQTTBroker;

	S_SettingParams()
		: m_StayOnTop( false )
		, m_ResetOnProcInstance( false )
		, m_AutoScroll( true )
    {
	}
};

class C_MainWindow : public wxFrame
{
	struct S_PersistentSettings;

public:

	C_MainWindow();
    ~C_MainWindow();

	void Init();

    wxIcon const& GetIcon(trace::TracedMessageType_t inType);

	void ToggleAutoScroll();
	void DeleteCurrentPageContent();

	void OnMessageGreetings( char const* inTitle, trace::TraceAppID_t _app );
	void OnMessageFarewell( trace::TraceAppID_t _app );
	void OnTraceMessage( trace::TraceAppID_t _app, trace::Message const& _msg, ThreadNameHandle _thread_name = InvalidThreadNameHandle );

	void LogSystemMessage( char const* text, trace::TracedMessageType_t type = trace::TMT_Information );

	wxString ResolveThreadName( ThreadNameHandle _handle ) const;

private:

	void ScrollToBottom( wxWindow* page );

    void            LoadConfig();
    void            SaveConfig();
    void            ApplyConfig( S_PersistentSettings const& p );

	void            ApplySettings( S_SettingParams const& s );

	void			CreateUI();
	ListView* 		CreateTab( char const* inTitle, wxDataViewModel* model, trace::TraceAppID_t _app );
	ListView* 		CreateSysTab( wxDataViewModel* model );
	wxWindow* 		CreateSettingsPanel( wxWindow* _parent );

public:
    void            OnTick(wxTimerEvent& inEvent);
    void            OnTabClose(wxBookCtrlEvent& event);
	void			OnClose(wxCloseEvent& event);
	void			OnKeyPressEvent(wxKeyEvent& event);

	static bool		LoadIco(wxIcon& outIco, DWORD id);

private:

	void 			OnFilterChanged( wxEvent& _ev );
	void 			OnFilterApply();

private:

	typedef uint16_t T_SettingsVersion;
	static T_SettingsVersion const E_LatestVersion	= 5;

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

	C_ClientOverseer* 	m_Overseer = nullptr;
    C_Tray				m_Tray;

	int32_t				m_ApplyPendingSettingsCooldown = 0;
	S_SettingParams		m_CurrentSetting;
	S_SettingParams		m_PendingSetting;
	MsgFilter*			m_MsgFilter;

	struct S_Tab
	{
		C_TabModel*		m_DataModel;
		ListView*		m_ListViewCtrl;
	};

    typedef std::unordered_map< trace::TraceAppID_t, S_Tab > T_ClientToTabMap;

    T_ClientToTabMap    m_TabMap;
	wxTimer				m_TickTimer;

    wxIcon				m_MsgIcons[ std::max( trace::AvailableTypes ) + 1 ];

	struct
	{
		wxAuiNotebook*	m_TabControl = nullptr;

		wxCheckBox*		m_StayOnTopChck;
		wxCheckBox*		m_ResetOnNewProcessInstanceChck;
		wxCheckBox*		m_AutoScrollChck;
		wxTextCtrl*		m_MessageFilterCntrl;
		wxCheckBox*		m_MessageCaseSensitiveChck;
		wxCheckBox*		m_MessageFilteredAreGreydOut;
		wxCheckBox*		m_TypeFilterChck_Info;
		wxCheckBox*		m_TypeFilterChck_Log;
		wxCheckBox*		m_TypeFilterChck_Warning;
		wxCheckBox*		m_TypeFilterChck_Error;
		wxSpinCtrl*		m_CountLimit;
		wxTextCtrl*		m_MQTTCntrl;
	}					m_UI;

	std::thread::id m_UiThreadId;
	std::mutex m_PendingSysMessagesMutex;
	std::vector< trace::Message > m_PendingSysMessages;

	wxDECLARE_EVENT_TABLE();
};
