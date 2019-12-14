#include "stdafx.h"
#include "mainwindow.h"
#include "TabModel.h"
#include "SettingsDialog.h"
#include "../Resource.h"
#include "FormatedRenderer.h"
#include "wxNotebookEx.h"
#include "../ClientOverseer.h"

#define SETTINGS_CFG_FILE "setings.trace"

// -------------------------------------------------------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(C_MainWindow, wxFrame)
	EVT_CLOSE(C_MainWindow::OnClose)
	EVT_NOTEBOOK_PAGE_CLOSE(C_MainWindow::OnTabClose)
wxEND_EVENT_TABLE()

// -------------------------------------------------------------------------------------------------------------------------

C_MainWindow::C_MainWindow()
    : wxFrame(nullptr, wxID_ANY, "Trace Server", wxPoint(50, 50), wxSize(950, 340), wxCAPTION|wxSYSTEM_MENU|wxRESIZE_BORDER|wxCLOSE_BOX|wxMAXIMIZE_BOX|wxCLIP_CHILDREN )
	, m_Tray( this )
	, m_TabToRefresh(nullptr)
	, m_MsgFilter( new C_MsgFilter() )
{  
	m_Overseer = new C_ClientOverseer();

	CreateUI();

	if ( !m_Overseer->Init( this ) )
	{
		wxMessageBox( "Trace server failed to initialize! Make sure there's no trace server already launched", "Fail", wxOK | wxCENTER | wxICON_ERROR );
		delete m_Overseer;
		m_Overseer = nullptr;
		return;
	}

    LoadConfig();
	
	char buf[128];
	sprintf(buf, "#%u", IDI_MSG_INFO );
	bool succ	=  m_MsgIcons[0].LoadFile( buf, wxBITMAP_TYPE_ICO_RESOURCE, 16, 16);
	sprintf( buf, "#%u", IDI_MSG_WARNING );
	succ		&= m_MsgIcons[1].LoadFile( buf, wxBITMAP_TYPE_ICO_RESOURCE, 16, 16);
	sprintf( buf, "#%u", IDI_MSG_ERROR );
	succ		&= m_MsgIcons[2].LoadFile( buf, wxBITMAP_TYPE_ICO_RESOURCE, 16, 16);

	m_TickTimer.Bind(wxEVT_TIMER, &C_MainWindow::OnTick, this);
	m_TickTimer.Start(50);
}

// -------------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::CreateUI()
{
	wxRect client = GetClientRect();

//	m_UI.m_Sizer		= new wxBoxSizer(wxHORIZONTAL);
// 	m_UI.m_Sizer->Add(this);
// 	SetSizer(m_UI.m_Sizer);

	m_UI.m_TabControl	= new wxNotebookEx(this, wxID_ANY, wxPoint(0,0), wxSize(client.GetWidth(), client.GetHeight()));

	wxAuiSimpleTabArt* tabArt = new wxAuiSimpleTabArt();
	tabArt->SetColour( wxColour( 0xd5d5d5 ) );
	m_UI.m_TabControl->SetArtProvider( tabArt );

// 	m_UI.m_Sizer->Add(m_UI.m_TabControl);
//	m_UI.m_TabControl->SetSizer(m_UI.m_Sizer);

//	m_UI.m_TabControl->SetBackgroundColour( wxColour( 0x262525 ) );
	

	HICON hico = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_TRACERSERVER));
	if (NULL != hico)
	{
		wxIcon ico;
		if (ico.CreateFromHICON(hico))
		{
			SetIcon( ico );
		}

		DestroyIcon(hico);
	}
}

// -------------------------------------------------------------------------------------------------------------------------

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
			LogSystemMessage( "Config applied" );
			return;
		}
	}

	LogSystemMessage( "Config load failed", trace::TMT_Warning );
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::SaveConfig()
{
	S_PersistentSettings p;
	p.m_Version = E_LatestVersion;
	
	GetSize( &p.m_SizeX, &p.m_SizeY );
	GetPosition( &p.m_PosX, &p.m_PosY );

	p.m_StayOnTop = ( GetWindowStyle() & wxSTAY_ON_TOP ) != 0;
	p.m_ResetOnProcInstance = m_CurrentSetting.m_ResetOnProcInstance;
	p.m_CountLimit = m_CurrentSetting.m_MsgFilter.m_MsgLimit;
	memset( p.m_MQTTBroker, 0, sizeof( p.m_MQTTBroker ) );
	strcpy_s( p.m_MQTTBroker, m_CurrentSetting.m_MQTTBroker.c_str() );

	bool ok = false;

	FILE* f = fopen( SETTINGS_CFG_FILE, "wb" );
	if ( f != nullptr )
	{
		ok = fwrite( &p, sizeof( S_PersistentSettings ), 1, f ) == 1;
		fclose( f );
	}
		
	if ( ok )
	{
		LogSystemMessage( "Config saved" );
	}
	else
	{
		LogSystemMessage( "Config save failed", trace::TMT_Error );
	}
}

// -------------------------------------------------------------------------------------------------------------------------

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
	m_CurrentSetting.m_MsgFilter.m_MsgLimit = p.m_CountLimit;
	m_CurrentSetting.m_MQTTBroker = p.m_MQTTBroker;

	S_FilterOptions filterOptions;
	filterOptions.m_MsgLimit = p.m_CountLimit;

	m_MsgFilter->Apply( filterOptions );
	m_Overseer->SetMQTTBroker( m_CurrentSetting.m_MQTTBroker.c_str() );
}

// -------------------------------------------------------------------------------------------------------------------------

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

			if ( !s.m_MsgFilter.m_Text.empty() )
			{
				title += " - ";

				static size_t const filterMaxLen = 16;

				size_t filterLen = s.m_MsgFilter.m_Text.length();
				title += ( filterLen > filterMaxLen ) ? s.m_MsgFilter.m_Text.substr( 0, filterMaxLen ) : s.m_MsgFilter.m_Text;
			}

			tab.m_ListViewCtrl->GetColumn( ( uint32_t )cIdx )->SetTitle( title );
		}
	}

	m_Overseer->SetMQTTBroker( s.m_MQTTBroker.c_str() );
	m_CurrentSetting = s;
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::OnTick(wxTimerEvent& inEvent)
{
    if (nullptr != m_Overseer )
    {
		m_Overseer->Tick();
    }
}

// -------------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------------

wxDataViewCtrl* C_MainWindow::CreateTab( char const* inTitle, wxDataViewModel* model )
{
	wxDataViewCtrl* listBox = new wxDataViewCtrl( m_UI.m_TabControl, wxID_ANY );
	listBox->SetRowHeight(17);
	listBox->SetBackgroundColour( wxColour( 0xE9F1F0 ) );

	//listBox->SetForegroundColour( wxColour( 0xffffff ) );
	listBox->SetAlternateRowColour( wxColour( 0xff ) );
	
	listBox->AppendColumn(new wxDataViewColumn("", new wxDataViewBitmapRenderer( "wxIcon" ), E_C_Icon, 22, wxALIGN_LEFT, 0));
	listBox->AppendColumn(new wxDataViewColumn("Time", new wxDataViewTextRenderer(), E_C_Time, 110, wxALIGN_LEFT, 0));
	listBox->AppendColumn(new wxDataViewColumn("Msg", new C_FormatedRenderer(), E_C_Msg, 650, wxALIGN_LEFT));
	listBox->AppendColumn(new wxDataViewColumn("Fn", new wxDataViewTextRenderer(), E_C_Fn, 220, wxALIGN_LEFT));
	listBox->AppendColumn(new wxDataViewColumn("File", new wxDataViewTextRenderer(), E_C_File, 200, wxALIGN_LEFT));
	listBox->AppendColumn(new wxDataViewColumn("Thread", new wxDataViewTextRenderer(), E_C_Thread, 70, wxALIGN_LEFT));
// 	
//	m_UI.m_Sizer->Add(listBox, 1, wxEXPAND);

	listBox->AssociateModel( model );

	m_UI.m_TabControl->AddPage( listBox, inTitle, true );
	return listBox;
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::OnMessageGreetings( char const* inTitle, T_SourceID id )
{
	char buf[512];

	T_ClientToTabMap::iterator it = m_TabMap.find( id );
	if ( it != m_TabMap.end() )
	{
		S_Tab& tab = it->second;

		if ( m_CurrentSetting.m_ResetOnProcInstance )
		{
			tab.m_DataModel->Clear();
		}
		
		if ( tab.m_ListViewCtrl == nullptr )
		{
			tab.m_ListViewCtrl = CreateTab( inTitle, tab.m_DataModel );
		}
		else
		{
			int idx = m_UI.m_TabControl->GetPageIndex( tab.m_ListViewCtrl );
			if ( idx >= 0 )
			{
				m_UI.m_TabControl->SetPageText( idx, inTitle );
			}
		}

		sprintf_s( buf, "'%s' re-connected", inTitle );
		LogSystemMessage( buf );
	}
	else
	{
		S_Tab tab;
		tab.m_DataModel = new C_TabModel( this, *m_MsgFilter );
		tab.m_ListViewCtrl = CreateTab( inTitle, tab.m_DataModel );

		m_TabMap.emplace( id, tab );

		if ( id != 0 )
		{
			sprintf_s( buf, "'%s' connected", inTitle );
			LogSystemMessage( buf );
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::OnMessageFarewell( T_SourceID id )
{
	T_ClientToTabMap::iterator it = m_TabMap.find( id );
	if ( it != m_TabMap.end() )
	{
		wxString title = m_UI.m_TabControl->GetPageText( m_UI.m_TabControl->GetPageIndex( it->second.m_ListViewCtrl ) );

		char buf[512];
		sprintf_s( buf, "'%s' disconnected", (char const*)title );
		LogSystemMessage( buf );
	}
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::OnTraceMessage( S_Message const& msg, char const* threadName )
{
	T_ClientToTabMap::iterator it = m_TabMap.find( msg.m_SourceID );
	if ( it != m_TabMap.end() )
	{
		S_Tab& tab = it->second;

		if ( tab.m_ListViewCtrl != nullptr )
		{ // Tab could have been closed -> ignore the rest of messages from that process instance
			tab.m_DataModel->AddMsg( msg, threadName );

			if ( m_CurrentSetting.m_AutoScroll )
			{
				ScrollToBottom( tab.m_ListViewCtrl );
			}
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::LogSystemMessage( char const* text, trace::E_TracedMessageType type )
{
	S_Message* msg = S_Message::Create( 0, type, 0, text, nullptr, nullptr, 0 );
	OnTraceMessage( *msg, nullptr );
	delete msg;
}

// -------------------------------------------------------------------------------------------------------------------------

wxIcon const& C_MainWindow::GetIcon( trace::E_TracedMessageType inType)
{
    switch (inType)
    {
    case trace::TMT_Information:  return m_MsgIcons[0];
    case trace::TMT_Warning:      return m_MsgIcons[1];
    case trace::TMT_Error:        return m_MsgIcons[2];
    }

	static wxIcon invalid;
    return invalid;
}

// -------------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::ShowSettingsDialog()
{
	S_SettingParams params = m_CurrentSetting;
	C_SettingsDialog dlg( params, GetIcon( trace::TMT_Information ), GetIcon( trace::TMT_Warning ), GetIcon( trace::TMT_Error ) );

    if (wxID_OK == dlg.ShowModal())
    {		
		ApplySettings( params );
		SaveConfig();
    }
}

// -------------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------------

void C_MainWindow::ToggleAutoScroll()
{
	m_CurrentSetting.m_AutoScroll = !m_CurrentSetting.m_AutoScroll;
}

// -------------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------------

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