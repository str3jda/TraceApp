#include <App/stdafx.h>
#include <App/UI/MainWindow.h>
#include <App/UI/TabModel.h>
#include <App/UI/FormatedRenderer.h>
#include <App/UI/ListView.h>
#include <App/UI/MyAuiTabArt.h>
#include <App/ClientOverseer.h>
#include <App/Utilities/assert.h>
#include <App/Resource.h>
#include <App/Theme.h>
#include <Trace/Message.h>

#include <bit>
#include "wxNotebookEx.h"

#define SETTINGS_CFG_FILE "setings.trace"

///////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(C_MainWindow, wxFrame)
	EVT_CLOSE(C_MainWindow::OnClose)
	EVT_NOTEBOOK_PAGE_CLOSE(C_MainWindow::OnTabClose)
wxEND_EVENT_TABLE()

///////////////////////////////////////////////////////////////

C_MainWindow::C_MainWindow()
    : wxFrame(nullptr, wxID_ANY, "Trace App " TRACE_VERSION, wxPoint(50, 50), wxSize(950, 340), wxCAPTION|wxSYSTEM_MENU|wxRESIZE_BORDER|wxCLOSE_BOX|wxMAXIMIZE_BOX|wxCLIP_CHILDREN|wxTAB_TRAVERSAL )
	, m_Tray( this )
	, m_MsgFilter( new MsgFilter() )
	, m_UiThreadId( std::this_thread::get_id() )
{  
}

C_MainWindow::~C_MainWindow()
{
	m_TickTimer.Stop();

    if (nullptr != m_Overseer )
    {
		m_Overseer->Deinit();
        delete m_Overseer;
    }

	delete m_MsgFilter;

	m_TabMap.clear();
}

void C_MainWindow::Init()
{
	auto set_ico = [this]( uint32_t _res_id, trace::TracedMessageType_t _type )
	{
		char buf[128];
		sprintf(buf, "#%u", _res_id );

		m_MsgIcons[ static_cast< size_t >( _type ) ].LoadFile( buf, wxBITMAP_TYPE_ICO_RESOURCE, 16, 16);
	};

	set_ico(IDI_MSG_INFO, trace::TMT_Information);
	set_ico(IDI_MSG_LOG, trace::TMT_Log);
	set_ico(IDI_MSG_WARNING, trace::TMT_Warning);
	set_ico(IDI_MSG_ERROR, trace::TMT_Error);

	LoadConfig();

	if ( m_CurrentSetting.m_MQTTBroker.empty() )
		m_CurrentSetting.m_MQTTBroker = "localhost";

	CreateUI();

	m_Overseer = new C_ClientOverseer();
	if ( !m_Overseer->Init( this, m_CurrentSetting.m_MQTTBroker.c_str() ) )
	{
		wxMessageBox( "Trace server failed to initialize! Make sure there's no trace server already launched", "Fail", wxOK | wxCENTER | wxICON_ERROR );
		delete m_Overseer;
		m_Overseer = nullptr;
		return;
	}

	m_Overseer->StartListening();

	wxToolTip::SetDelay( 1000 );
	wxToolTip::SetAutoPop( 5000 );
	wxToolTip::SetReshow( 1000 );

	m_TickTimer.Bind(wxEVT_TIMER, &C_MainWindow::OnTick, this);
	m_TickTimer.Start(50);	
}

void C_MainWindow::CreateUI()
{
	m_UI.m_TabControl = new wxNotebookEx(this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
	m_UI.m_TabControl->SetTabCtrlHeight( 20 );

	wxIcon sys_tab_ico; 
	LoadIco( sys_tab_ico, IDI_SYS_TAB );

	wxIcon sys_tab_act_ico; 
	LoadIco( sys_tab_act_ico, IDI_SYS_TAB_ACTIVE );	

	wxAuiTabArt* tabArt = new MyAuiTabArt( sys_tab_ico, sys_tab_act_ico );
	tabArt->SetColour( wxColour( theme::WINDOW_BACKGROUND ) );
	tabArt->SetActiveColour( wxColour( theme::TAB_ACTIVE_BACKGROUND ) );
	
	m_UI.m_TabControl->SetArtProvider( tabArt );

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add( m_UI.m_TabControl, 1, wxEXPAND, 0 );
	SetSizer(sizer);
	
	if ( wxIcon ico; LoadIco( ico, IDI_TRACERSERVER ) )
		SetIcon( ico );

	S_Tab tab;
	tab.m_DataModel = new C_TabModel( this, *m_MsgFilter );
	tab.m_ListViewCtrl = CreateSysTab( tab.m_DataModel );

	m_TabMap.emplace( 0, tab );
}

void C_MainWindow::LoadConfig()
{
	S_PersistentSettings p;
	DWORD size = sizeof( S_PersistentSettings );

	FILE* f = fopen( SETTINGS_CFG_FILE, "rb" );
	if ( f != nullptr )
	{
		fread( &p, sizeof( S_PersistentSettings ), 1, f );
		fclose( f );

		if ( p.m_Version == E_LatestVersion )
		{
			ApplyConfig( p );
			return;
		}
	}

	LogSystemMessage( "Config load failed", trace::TMT_Warning );
}

void C_MainWindow::SaveConfig()
{
	S_PersistentSettings p;
	p.m_Version = E_LatestVersion;
	
	GetSize( &p.m_SizeX, &p.m_SizeY );
	GetPosition( &p.m_PosX, &p.m_PosY );

	p.m_StayOnTop = ( GetWindowStyle() & wxSTAY_ON_TOP ) != 0;
	p.m_ResetOnProcInstance = m_CurrentSetting.m_ResetOnProcInstance;
	p.m_CountLimit = m_CurrentSetting.m_MsgFilter.MsgLimit;
	memset( p.m_MQTTBroker, 0, sizeof( p.m_MQTTBroker ) );
	strcpy_s( p.m_MQTTBroker, m_CurrentSetting.m_MQTTBroker.c_str() );

	bool ok = false;

	FILE* f = fopen( SETTINGS_CFG_FILE, "wb" );
	if ( f != nullptr )
	{
		ok = fwrite( &p, sizeof( S_PersistentSettings ), 1, f ) == 1;
		fclose( f );
	}
		
	if ( !ok )
	{
		LogSystemMessage( "Config save failed", trace::TMT_Error );
	}
}

void C_MainWindow::ApplyConfig( S_PersistentSettings const& p )
{
    Move(p.m_PosX, p.m_PosY);
    SetSize(p.m_SizeX, p.m_SizeY);

    if (p.m_StayOnTop)
    {
        SetWindowStyle(GetWindowStyle() | wxSTAY_ON_TOP);
    }
    else
    {
        SetWindowStyle(GetWindowStyle() & ~wxSTAY_ON_TOP);
    }

	m_CurrentSetting.m_StayOnTop = p.m_StayOnTop;
	m_CurrentSetting.m_ResetOnProcInstance = p.m_ResetOnProcInstance;
	m_CurrentSetting.m_MsgFilter.MsgLimit = p.m_CountLimit;
	m_CurrentSetting.m_MQTTBroker = p.m_MQTTBroker;

	FilterOptions filterOptions;
	filterOptions.MsgLimit = p.m_CountLimit;

	m_MsgFilter->Apply( filterOptions );
	if ( m_Overseer != nullptr )
		m_Overseer->SetMQTTBroker( m_CurrentSetting.m_MQTTBroker.c_str() );
}

void C_MainWindow::ApplySettings( S_SettingParams const& s )
{
	if ( s.m_StayOnTop )
	{
		SetWindowStyle(GetWindowStyle() | wxSTAY_ON_TOP);
	}
	else
	{
		SetWindowStyle(GetWindowStyle() & ~wxSTAY_ON_TOP);
	}

	if ( m_MsgFilter->Apply( s.m_MsgFilter ) )
	{
		for ( T_ClientToTabMap::value_type const& p : m_TabMap )
		{
			S_Tab const& tab = p.second;
			tab.m_DataModel->OnFilterChanged();

			int cIdx = tab.m_ListViewCtrl->GetModelColumnIndex( E_C_Msg );

			std::string title = "Msg";

			if ( !s.m_MsgFilter.Text.empty() )
			{
				title += " - ";

				static size_t const filterMaxLen = 16;

				size_t filterLen = s.m_MsgFilter.Text.length();
				title += ( filterLen > filterMaxLen ) ? s.m_MsgFilter.Text.substr( 0, filterMaxLen ) : s.m_MsgFilter.Text;
			}

			tab.m_ListViewCtrl->GetColumn( ( uint32_t )cIdx )->SetTitle( title );
		}
	}

	m_Overseer->SetMQTTBroker( s.m_MQTTBroker.c_str() );
	m_CurrentSetting = s;
}

void C_MainWindow::OnTick(wxTimerEvent& inEvent)
{
    if (nullptr != m_Overseer )
    {
		m_PendingSysMessagesMutex.lock();
		if ( !m_PendingSysMessages.empty() )
		{
			for ( trace::Message& msg : m_PendingSysMessages )
			{
				OnTraceMessage( 0, msg );
			}

			m_PendingSysMessages.clear();
		}

		m_PendingSysMessagesMutex.unlock();

		m_Overseer->Tick();

		if ( m_CurrentSetting.m_AutoScroll )
		{
			int pageIdx = m_UI.m_TabControl->GetSelection();
			wxWindow* page = m_UI.m_TabControl->GetPage( pageIdx );

			ListView* list = std::bit_cast< ListView* >( page->GetClientData() );

			if ( list->NewMessagesCame )
			{
				ScrollToBottom( list );
				list->NewMessagesCame = false;
			}
		}
    }

	if ( m_ApplyPendingSettingsCooldown > 0 )
	{
		if ( --m_ApplyPendingSettingsCooldown == 0 )
		{
			ApplySettings( m_PendingSetting );
			m_PendingSetting = {};

			SaveConfig();
		}
	}
}

void C_MainWindow::OnTabClose(wxBookCtrlEvent& event)
{
	int tabIdx = event.GetSelection();
	if (0 == tabIdx)
	{
		event.Veto();
	}
	else
	{
		wxWindow* page = m_UI.m_TabControl->GetPage( tabIdx );
		
		for ( auto& p : m_TabMap )
		{
			S_Tab& tab = p.second;
			if ( tab.m_ListViewCtrl == page )
			{
				tab.m_ListViewCtrl = nullptr;
				break;
			}
		}
	}
}

ListView* C_MainWindow::CreateTab( char const* inTitle, wxDataViewModel* model, trace::TraceAppID_t _app )
{
	ListView* listBox = new ListView( m_UI.m_TabControl );
	listBox->SetClientData( listBox );
	listBox->SetRowHeight(17);
	listBox->SetBackgroundColour( wxColour( theme::WINDOW_BACKGROUND ) );
	
	listBox->AppendColumn( new wxDataViewColumn( "", new wxDataViewBitmapRenderer( "wxIcon" ), E_C_Icon, 22, wxALIGN_LEFT, 0 ) );
	listBox->AppendColumn( new wxDataViewColumn( "Msg", new C_FormatedRenderer(), E_C_Msg, 650, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ) );
	listBox->AppendColumn( new wxDataViewColumn( "Time", new wxDataViewTextRenderer(), E_C_LocalTime, 80, wxALIGN_LEFT, 0 ) );
	listBox->AppendColumn( new wxDataViewColumn( "Date", new wxDataViewTextRenderer(), E_C_Time, 60, wxALIGN_LEFT, 0 ) );
	listBox->AppendColumn( new wxDataViewColumn( "Fn", new wxDataViewTextRenderer(), E_C_Fn, 180, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ) );
	listBox->AppendColumn( new wxDataViewColumn( "File", new wxDataViewTextRenderer(), E_C_File, 170, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ) );
	listBox->AppendColumn( new wxDataViewColumn( "Thread", new wxDataViewTextRenderer(), E_C_Thread, 75, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ) );
	listBox->AppendColumn( new wxDataViewColumn( "Frame", new wxDataViewTextRenderer(), E_C_Frame, 45, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE ) );
	
	listBox->AssociateModel( model );
	if ( wxHeaderCtrl* header = listBox->GenericGetHeader(); header != nullptr )
		header->SetMaxSize( wxSize( header->GetMaxWidth(), 17 ) );

	m_UI.m_TabControl->AddPage( listBox, inTitle, true );
	return listBox;
}

ListView* C_MainWindow::CreateSysTab( wxDataViewModel* model )
{
	wxWindow* main_panel = new wxWindow( this, wxID_ANY );

	ListView* listBox = new ListView( main_panel, true );
	listBox->SetRowHeight(17);
	listBox->SetBackgroundColour( wxColour( theme::WINDOW_BACKGROUND ) );
	listBox->AssociateModel( model );

	wxDataViewColumn* msg_col = new wxDataViewColumn( "Msg", new C_FormatedRenderer(), E_C_Msg, 80, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE );
	msg_col->SetMinWidth( 150 );

	listBox->AppendColumn( new wxDataViewColumn( "", new wxDataViewBitmapRenderer( "wxIcon" ), E_C_Icon, 22, wxALIGN_LEFT, 0 ) );
	listBox->AppendColumn( msg_col );
	listBox->AppendColumn( new wxDataViewColumn( "Time", new wxDataViewTextRenderer(), E_C_LocalTime, 80, wxALIGN_LEFT, 0 ) );
	listBox->AppendColumn( new wxDataViewColumn( "Date", new wxDataViewTextRenderer(), E_C_Time, 60, wxALIGN_LEFT, 0 ) );

	if ( wxHeaderCtrl* header = listBox->GenericGetHeader(); header != nullptr )
		header->SetMaxSize( wxSize( header->GetMaxWidth(), 17 ) );

	wxWindow* settings_panel = CreateSettingsPanel( main_panel );

	wxSizer* main_sizer = new wxBoxSizer( wxHORIZONTAL );
	wxSizer* left_sizer = new wxBoxSizer( wxVERTICAL );
	wxSizer* right_sizer = new wxBoxSizer( wxVERTICAL );

	left_sizer->Add(listBox, 1, wxEXPAND);
	right_sizer->Add(settings_panel, 1, wxEXPAND);

	main_sizer->Add(left_sizer, 2, wxEXPAND);
	main_sizer->Add(right_sizer, 1, wxEXPAND);

	main_panel->SetSizerAndFit( main_sizer );	
	main_panel->SetClientData( listBox );

	m_UI.m_TabControl->AddPage( main_panel, "", true );
	return listBox;
}

wxWindow* C_MainWindow::CreateSettingsPanel( wxWindow* _parent ) 
{
	wxWindow* panel = new wxWindow( _parent, wxID_ANY );
		
	panel->SetBackgroundColour( theme::WINDOW_BACKGROUND );
	panel->SetForegroundColour( *wxWHITE );

	wxBoxSizer* main_sizer = new wxBoxSizer( wxVERTICAL );
	wxFlexGridSizer* grid_sizer = new wxFlexGridSizer(2, 4, 8);
	grid_sizer->AddGrowableCol( 1 );

	main_sizer->Add( grid_sizer );
	panel->SetSizer( main_sizer );

	auto add_prop = [=]( char const* _prop_name, std::invocable< wxBoxSizer*, wxWindow* > auto _value )
	{
		grid_sizer->Add( new wxStaticText( panel, wxID_ANY, _prop_name ), wxSizerFlags().Border(wxTOP|wxLEFT) );

		wxBoxSizer* local_sizer = new wxBoxSizer( wxHORIZONTAL );
		grid_sizer->Add( local_sizer, wxSizerFlags().Border(wxTOP|wxLEFT) );

		_value( local_sizer, panel );
	};

	// Message type filter
	add_prop( "Type: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( new wxGenericStaticBitmap( _parent, wxID_ANY, GetIcon( trace::TMT_Information ) ) );
		_sizer->Add( m_UI.m_TypeFilterChck_Info = new wxCheckBox( _parent, wxID_ANY, "" ), wxSizerFlags().Border(wxLEFT) );
		m_UI.m_TypeFilterChck_Info->SetValue( ( m_CurrentSetting.m_MsgFilter.TypeFlags & ( 1 << trace::TMT_Information ) ) != 0 );
		m_UI.m_TypeFilterChck_Info->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );

		_sizer->AddSpacer( 12 );

		_sizer->Add( new wxGenericStaticBitmap( _parent, wxID_ANY, GetIcon( trace::TMT_Log ) ) );
		_sizer->Add( m_UI.m_TypeFilterChck_Log = new wxCheckBox( _parent, wxID_ANY, "" ), wxSizerFlags().Border(wxLEFT) );
		m_UI.m_TypeFilterChck_Log->SetValue( ( m_CurrentSetting.m_MsgFilter.TypeFlags & ( 1 << trace::TMT_Log ) ) != 0 );
		m_UI.m_TypeFilterChck_Log->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );

		_sizer->AddSpacer( 12 );

		_sizer->Add( new wxGenericStaticBitmap( _parent, wxID_ANY, GetIcon( trace::TMT_Warning ) ) );
		_sizer->Add( m_UI.m_TypeFilterChck_Warning = new wxCheckBox( _parent, wxID_ANY, "" ), wxSizerFlags().Border(wxLEFT) );
		m_UI.m_TypeFilterChck_Warning->SetValue( ( m_CurrentSetting.m_MsgFilter.TypeFlags & ( 1 << trace::TMT_Warning ) ) != 0 );
		m_UI.m_TypeFilterChck_Warning->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );

		_sizer->AddSpacer( 12 );

		_sizer->Add( new wxGenericStaticBitmap( _parent, wxID_ANY, GetIcon( trace::TMT_Error ) ) );
		_sizer->Add( m_UI.m_TypeFilterChck_Error = new wxCheckBox( _parent, wxID_ANY, "" ), wxSizerFlags().Border(wxLEFT) );
		m_UI.m_TypeFilterChck_Error->SetValue( ( m_CurrentSetting.m_MsgFilter.TypeFlags & ( 1 << trace::TMT_Error ) ) != 0 );
		m_UI.m_TypeFilterChck_Error->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
		
		_sizer->AddSpacer( 4 );
	});

	// Message text filter
	add_prop( "Message: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_MessageFilterCntrl = new wxTextCtrl( _parent, wxID_ANY, m_CurrentSetting.m_MsgFilter.Text.c_str() ), wxSizerFlags().Border(wxLEFT) );

		m_UI.m_MessageFilterCntrl->Bind( wxEVT_TEXT, &C_MainWindow::OnFilterChanged, this );
		m_UI.m_MessageFilterCntrl->SetFocus();
		m_UI.m_MessageFilterCntrl->SetInsertionPointEnd();

		_sizer->Add( m_UI.m_MessageCaseSensitiveChck = new wxCheckBox( _parent, wxID_ANY, "Case" ), wxSizerFlags().Border(wxLEFT) );
		m_UI.m_MessageCaseSensitiveChck->SetValue( m_CurrentSetting.m_MsgFilter.CaseSensitive );
		m_UI.m_MessageCaseSensitiveChck->SetToolTip( "Case sensitive" );
		m_UI.m_MessageCaseSensitiveChck->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	});

	// Message text filter
	add_prop( "Grey out: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_MessageFilteredAreGreydOut = new wxCheckBox(_parent, wxID_ANY, "" ) );
		m_UI.m_MessageFilteredAreGreydOut->SetValue( m_CurrentSetting.m_MsgFilter.GreyOutFilteredOut );
		m_UI.m_MessageFilteredAreGreydOut->SetToolTip( "Grey out filtered messages instead of hiding them" );
		m_UI.m_MessageFilteredAreGreydOut->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	} );

	// Stay App on top
	add_prop( "Stay on top: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_StayOnTopChck = new wxCheckBox( _parent, wxID_ANY, "" ) );
		m_UI.m_StayOnTopChck->SetValue( m_CurrentSetting.m_StayOnTop );
		m_UI.m_StayOnTopChck->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	} );
	
	// Reset tab content whenever new app say hi
	add_prop( "Auto reset: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_ResetOnNewProcessInstanceChck = new wxCheckBox( _parent, wxID_ANY, "" ) );
		m_UI.m_ResetOnNewProcessInstanceChck->SetValue( m_CurrentSetting.m_ResetOnProcInstance );
		m_UI.m_ResetOnNewProcessInstanceChck->SetToolTip( "Automatically reset tab content whenever app say hi again" );
		m_UI.m_ResetOnNewProcessInstanceChck->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	} );

	// Reset tab content whenever new app say hi
	add_prop( "Auto scroll: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_AutoScrollChck = new wxCheckBox( _parent, wxID_ANY, "(shortcut: Insert)" ) );
		m_UI.m_AutoScrollChck->SetValue( m_CurrentSetting.m_AutoScroll );
		m_UI.m_AutoScrollChck->SetToolTip( "Automatically scrolls to the last message" );
		m_UI.m_AutoScrollChck->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	} );

	// Reset tab content whenever new app say hi
	add_prop( "Message limit: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_CountLimit = new wxSpinCtrl( _parent, wxID_ANY, "" ) );
		m_UI.m_CountLimit->SetRange( 0, 0xffff );
		m_UI.m_CountLimit->SetValue( m_CurrentSetting.m_MsgFilter.MsgLimit );
		m_UI.m_CountLimit->SetToolTip( "Keep only this number of messages per tab (removing old ones)" );
		m_UI.m_CountLimit->SetForegroundColour( *wxBLACK );
		m_UI.m_CountLimit->Bind( wxEVT_CHECKBOX, &C_MainWindow::OnFilterChanged, this );
	} );

	// Reset tab content whenever new app say hi
	add_prop( "MQTT Broker: ", [this]( wxBoxSizer* _sizer, wxWindow* _parent )
	{
		_sizer->Add( m_UI.m_MQTTCntrl = new wxTextCtrl( _parent, wxID_ANY, m_CurrentSetting.m_MQTTBroker ) );
		m_UI.m_MQTTCntrl->SetValue( m_CurrentSetting.m_MQTTBroker );
		m_UI.m_MQTTCntrl->Bind( wxEVT_TEXT, &C_MainWindow::OnFilterChanged, this );
	} );
	
	// Legend
	main_sizer->Add( new wxStaticLine( panel, wxID_ANY, wxDefaultPosition, wxSize( 280, 1 ) ), wxSizerFlags(0).Border( wxNORTH|wxSOUTH ) );
	main_sizer->Add( new wxStaticText( panel, wxID_ANY, "Message formatting:" ) );

	wxRichTextCtrl* legend = new wxRichTextCtrl(
		panel, wxID_ANY, "", wxDefaultPosition, wxSize( 295, 195 ), wxBORDER_NONE | wxRE_READONLY );

	legend->SetBackgroundColour( theme::WINDOW_BACKGROUND );

	main_sizer->Add( legend );

	wxArrayInt tabs;
	tabs.Add( 230 );
	tabs.Add( 350 );

	wxTextAttr attr;
	attr.SetFlags( wxTEXT_ATTR_TABS );
	attr.SetTabs( tabs );
	attr.SetTextColour( *wxWHITE );

	legend->SetDefaultStyle( attr );

	legend->BeginSuppressUndo();

	{
		legend->WriteText(wxT("Hi "));
		legend->BeginTextColour(wxColour(255, 0, 0));
		legend->WriteText(wxT("red"));
		legend->EndTextColour();
		legend->WriteText(wxT("!"));

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{c:f00|red}}!"));
		legend->EndItalic();
	}
	legend->Newline();
	{
		legend->WriteText(wxT("Hi "));
		legend->BeginTextColour(wxColour(0xfc, 0x80, 0x50));
		legend->WriteText(wxT("orange"));
		legend->EndTextColour();
		legend->WriteText(wxT("!"));

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{c:fc8050|orange}}!"));
		legend->EndItalic();
	}
	legend->Newline();
	{
		legend->WriteText(wxT("Hi "));
		legend->BeginUnderline();
		legend->WriteText(wxT("background"));
		legend->EndUnderline();
		legend->WriteText(wxT(" (can't show in here) !"));
		legend->Newline();

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{g:0f0|background}}!"));
		legend->EndItalic();
	}
	legend->Newline();
	{
		legend->WriteText(wxT("Hi "));
		legend->BeginBold();
		legend->WriteText(wxT("bold"));
		legend->EndBold();
		legend->WriteText(wxT("!"));

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{b|bold}}!"));
		legend->EndItalic();
	}
	legend->Newline();
	{
		legend->WriteText(wxT("Hi "));
		legend->BeginItalic();
		legend->WriteText(wxT("italic"));
		legend->EndItalic();
		legend->WriteText(wxT("!"));

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{i|italic}}!"));
		legend->EndItalic();
	}
	legend->Newline();
	{
		legend->WriteText(wxT("Hi "));
		legend->BeginUnderline();
		legend->WriteText(wxT("underlined"));
		legend->EndUnderline();
		legend->WriteText(wxT("!"));

		legend->BeginItalic();
		legend->WriteText(wxT("\tHi {{u|underlined}}!"));
		legend->EndItalic();
	}
	legend->Newline();

	legend->EndSuppressUndo();	

	return panel;
}

void C_MainWindow::OnFilterChanged( wxEvent& _ev )
{
	m_ApplyPendingSettingsCooldown = _ev.GetEventObject()->IsKindOf(wxCLASSINFO(wxTextCtrl)) ? 10 : 1;

	uint32_t typeFlags = 0;

	if ( m_UI.m_TypeFilterChck_Info->GetValue() )
		typeFlags |= 1 << trace::TMT_Information;

	if ( m_UI.m_TypeFilterChck_Log->GetValue() )
		typeFlags |= 1 << trace::TMT_Log;

	if ( m_UI.m_TypeFilterChck_Warning->GetValue() )
		typeFlags |= 1 << trace::TMT_Warning;

	if ( m_UI.m_TypeFilterChck_Error->GetValue() )
		typeFlags |= 1 << trace::TMT_Error;

	m_PendingSetting.m_MsgFilter.TypeFlags = typeFlags;
	m_PendingSetting.m_MsgFilter.Text = m_UI.m_MessageFilterCntrl->GetLineText(0);
	m_PendingSetting.m_MsgFilter.CaseSensitive = m_UI.m_MessageCaseSensitiveChck->GetValue();
	m_PendingSetting.m_MsgFilter.GreyOutFilteredOut = m_UI.m_MessageFilteredAreGreydOut->GetValue();
	m_PendingSetting.m_AutoScroll = m_UI.m_AutoScrollChck->GetValue();

	m_PendingSetting.m_StayOnTop = m_UI.m_StayOnTopChck->GetValue();
	m_PendingSetting.m_ResetOnProcInstance = m_UI.m_ResetOnNewProcessInstanceChck->GetValue();
	m_PendingSetting.m_MsgFilter.MsgLimit = m_UI.m_CountLimit->GetValue();

	m_PendingSetting.m_MQTTBroker = m_UI.m_MQTTCntrl->GetLineText( 0 );
}

///////////////////////////////////////////////////////////////

void C_MainWindow::OnMessageGreetings( char const* inTitle, trace::TraceAppID_t _app )
{
	char buf[512];

	T_ClientToTabMap::iterator it = m_TabMap.find( _app );
	if ( it != m_TabMap.end() )
	{
		S_Tab& tab = it->second;

		if ( m_CurrentSetting.m_ResetOnProcInstance )
			tab.m_DataModel->Clear();
		
		if ( tab.m_ListViewCtrl == nullptr )
			tab.m_ListViewCtrl = CreateTab( inTitle, tab.m_DataModel, _app );
		else
		{
			int idx = m_UI.m_TabControl->GetPageIndex( tab.m_ListViewCtrl );
			if ( idx >= 0 )
				m_UI.m_TabControl->SetPageText( idx, inTitle );
		}

		sprintf_s( buf, "'%s' re-connected", inTitle );
		LogSystemMessage( buf );
	}
	else
	{
		S_Tab tab;
		tab.m_DataModel = new C_TabModel( this, *m_MsgFilter );
		tab.m_ListViewCtrl = CreateTab( inTitle, tab.m_DataModel, _app );

		m_TabMap.emplace( _app, tab );

		if ( _app != 0 )
		{
			sprintf_s( buf, "'%s' connected", inTitle );
			LogSystemMessage( buf );
		}
	}
}

void C_MainWindow::OnMessageFarewell( trace::TraceAppID_t _app )
{
	T_ClientToTabMap::iterator it = m_TabMap.find( _app );
	if ( it != m_TabMap.end() )
	{
		wxString title = m_UI.m_TabControl->GetPageText( m_UI.m_TabControl->GetPageIndex( it->second.m_ListViewCtrl ) );

		char buf[512];
		sprintf_s( buf, "'%s' disconnected", (char const*)title );
		LogSystemMessage( buf );
	}
}

void C_MainWindow::OnTraceMessage( trace::TraceAppID_t _app, trace::Message const& _msg, ThreadNameHandle _thread_name )
{
	if ( T_ClientToTabMap::iterator it = m_TabMap.find( _app ); it != m_TabMap.end() )
	{
		if ( S_Tab& tab = it->second; tab.m_ListViewCtrl != nullptr )
		{ // Tab could have been closed -> ignore the rest of messages from that process instance
			tab.m_DataModel->AddMsg( _msg, _thread_name );
			tab.m_ListViewCtrl->NewMessagesCame = true;
		}
	}
}

void C_MainWindow::LogSystemMessage( char const* text, trace::TracedMessageType_t type )
{
	trace::Message msg( type, text );

	if ( ( m_Overseer == nullptr ) || ( m_UiThreadId != std::this_thread::get_id() ) )
	{
		m_PendingSysMessagesMutex.lock();
		m_PendingSysMessages.push_back( std::move( msg ) );
		m_PendingSysMessagesMutex.unlock();
	}
	else
	{
		OnTraceMessage( 0, std::move( msg ) );
	}
}

wxString C_MainWindow::ResolveThreadName( ThreadNameHandle _handle ) const
{
	return m_Overseer->ResolveThreadName( _handle );
}

wxIcon const& C_MainWindow::GetIcon( trace::TracedMessageType_t inType)
{
    switch (inType)
    {
    case trace::TMT_Information:  return m_MsgIcons[ static_cast< size_t >( trace::TMT_Information ) ];
	case trace::TMT_Log:  		  return m_MsgIcons[ static_cast< size_t >( trace::TMT_Log ) ];
    case trace::TMT_Warning:      return m_MsgIcons[ static_cast< size_t >( trace::TMT_Warning ) ];
    case trace::TMT_Error:        return m_MsgIcons[ static_cast< size_t >( trace::TMT_Error ) ];
    }

	static wxIcon invalid;
    return invalid;
}

void C_MainWindow::OnClose(wxCloseEvent& event)
{
	if (event.CanVeto())
	{
		Hide();
		
		event.Veto(true);
	}
	else
	{
		SaveConfig();
		Destroy();
	}
}

void C_MainWindow::DeleteCurrentPageContent()
{
	int pageIdx = m_UI.m_TabControl->GetSelection();
	if ( pageIdx > 0 )
	{
		wxWindow* page = m_UI.m_TabControl->GetPage( pageIdx );

		for ( auto& p : m_TabMap )
		{
			S_Tab& tab = p.second;
			if ( tab.m_ListViewCtrl == page )
			{
				tab.m_DataModel->Clear();
				break;
			}
		}
	}
}

void C_MainWindow::ToggleAutoScroll()
{
	m_CurrentSetting.m_AutoScroll = !m_CurrentSetting.m_AutoScroll;
	if ( m_UI.m_AutoScrollChck != nullptr )
		m_UI.m_AutoScrollChck->SetValue( m_CurrentSetting.m_AutoScroll );
}

void C_MainWindow::ScrollToBottom( wxWindow* page )
{
	wxDataViewCtrl* listBox = static_cast<wxDataViewCtrl*>(page);
	C_TabModel* myModel = static_cast<C_TabModel*>(listBox->GetModel());

	if (0 < myModel->GetMsgCount())
	{
		listBox->Freeze();
		listBox->EnsureVisible(wxDataViewItem(reinterpret_cast<void*>(myModel->GetMsgCount())));
		listBox->Thaw();
	}
}

bool C_MainWindow::LoadIco(wxIcon& outIco, DWORD id)
{
	bool succ = false;
	HICON hico = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(id));
	if (NULL != hico)
	{
		succ = outIco.CreateFromHICON(hico);
		DestroyIcon(hico);
	}

	return succ;
}