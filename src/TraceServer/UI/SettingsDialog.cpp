#include "stdafx.h"
#include "SettingsDialog.h"

// -------------------------------------------------------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(C_SettingsDialog, wxDialog)
	EVT_BUTTON(wxID_OK, C_SettingsDialog::OnOk)
wxEND_EVENT_TABLE()

// -------------------------------------------------------------------------------------------------------------------------

C_SettingsDialog::C_SettingsDialog( S_SettingParams& outParams, wxIcon const& infoIcon, wxIcon const& warningIcon, wxIcon const& errorIcon )
    : wxDialog( nullptr, wxID_ANY, "Settings", wxDefaultPosition, wxSize( 500, 200 ) )
	, m_Params( outParams )
{
	// Type filter line
	new wxStaticText(this, wxID_ANY, "Type: ", wxPoint( 5, 7 ) );
	new wxGenericStaticBitmap( this, wxID_ANY, infoIcon, wxPoint( 80, 5 ) );
	m_TypeFilterChck_Info = new wxCheckBox(this, wxID_ANY, "", wxPoint( 100, 5 ) );
	m_TypeFilterChck_Info->SetValue( ( m_Params.m_MsgFilter.m_TypeFlags & trace::TMT_Information ) != 0 );

	new wxGenericStaticBitmap( this, wxID_ANY, warningIcon, wxPoint( 130, 5 ) );
	m_TypeFilterChck_Warning = new wxCheckBox(this, wxID_ANY, "", wxPoint( 150, 5 ) );
	m_TypeFilterChck_Warning->SetValue( ( m_Params.m_MsgFilter.m_TypeFlags & trace::TMT_Warning ) != 0 );

	new wxGenericStaticBitmap( this, wxID_ANY, errorIcon, wxPoint( 180, 5 ) );
	m_TypeFilterChck_Error = new wxCheckBox(this, wxID_ANY, "", wxPoint( 200, 5 ) );
	m_TypeFilterChck_Error->SetValue( ( m_Params.m_MsgFilter.m_TypeFlags & trace::TMT_Error ) != 0 );

	// Message Filter line
	new wxStaticText(this, wxID_ANY, "Message: ", wxPoint( 5, 34 ) );
	m_MessageFilterCntrl = new wxTextCtrl(this, wxID_ANY, m_Params.m_MsgFilter.m_Text.c_str(), wxPoint( 80, 32 ), wxSize( 300, wxDefaultSize.y ) );
	m_MessageFilterCntrl->Bind( wxEVT_KEY_UP, &C_SettingsDialog::OnKeyEnter, this );

	m_MessageCaseSensitiveChck = new wxCheckBox(this, wxID_ANY, "CS", wxPoint( 390, 34 ) );
	m_MessageCaseSensitiveChck->SetValue(m_Params.m_MsgFilter.m_CaseSensitive);
	m_MessageCaseSensitiveChck->SetToolTip( "Case sensitive" );

	m_MessageRegexChck = new wxCheckBox(this, wxID_ANY, "RegEx", wxPoint( 430, 34 ) );
	m_MessageRegexChck->SetValue( m_Params.m_MsgFilter.m_Regex );
	m_MessageRegexChck->SetToolTip( "Use regular expression" );

	// App settings line
	new wxStaticText(this, wxID_ANY, "Stay on top: ", wxPoint( 5, 60 ));
	m_StayOnTopChck = new wxCheckBox(this, wxID_ANY, "", wxPoint( 80, 61 ) );
	m_StayOnTopChck->SetValue(m_Params.m_StayOnTop);

	new wxStaticText( this, wxID_ANY, "Auto reset: ", wxPoint( 5, 82 ) );
	m_ResetOnNewProcessInstanceChck = new wxCheckBox( this, wxID_ANY, "", wxPoint( 80, 83 ) );
	m_ResetOnNewProcessInstanceChck->SetValue( m_Params.m_ResetOnProcInstance );
	m_ResetOnNewProcessInstanceChck->SetToolTip( "Automatically reset content on new process instance" );

	new wxStaticText( this, wxID_ANY, "Count limit: ", wxPoint( 5, 106 ) );
	m_CountLimit = new wxSpinCtrl( this, wxID_ANY, "XX", wxPoint( 82, 102 ) );
	m_CountLimit->SetRange( 0, 0xffff );
	m_CountLimit->SetValue( m_Params.m_MsgFilter.m_MsgLimit );

	new wxStaticText( this, wxID_ANY, "MQTT Broker: ", wxPoint( 5, 130 ) );
	m_MQTTCntrl = new wxTextCtrl(this, wxID_ANY, m_Params.m_MQTTBroker, wxPoint( 80, 128 ), wxSize( 180, wxDefaultSize.y ) );
	m_MQTTCntrl->Bind( wxEVT_KEY_UP, &C_SettingsDialog::OnKeyEnter, this );

	// Exit dialog line
	new wxButton(this, wxID_OK, "OK", wxPoint(348, 135), wxSize(60, 18)); 
	new wxButton(this, wxID_CANCEL, "Cancel", wxPoint(414, 135), wxSize(60, 18));

	m_MessageFilterCntrl->SetFocus();
	m_MessageFilterCntrl->SetInsertionPointEnd();
}

// -------------------------------------------------------------------------------------------------------------------------

void C_SettingsDialog::OnKeyEnter(wxKeyEvent& event)
{
	if ( event.GetKeyCode() == WXK_RETURN )
	{
		ApplyChanges();
		EndDialog(GetAffirmativeId());
	}
	else
	{
		event.Skip( true );
	}
}

// -------------------------------------------------------------------------------------------------------------------------

void C_SettingsDialog::OnOk(wxCommandEvent& event)
{
	ApplyChanges();
	EndDialog(GetAffirmativeId());
}

// -------------------------------------------------------------------------------------------------------------------------

void C_SettingsDialog::ApplyChanges()
{
	m_Params.m_StayOnTop = m_StayOnTopChck->GetValue();
	m_Params.m_ResetOnProcInstance = m_ResetOnNewProcessInstanceChck->GetValue();
	m_Params.m_MsgFilter.m_Text = m_MessageFilterCntrl->GetLineText(0);
	m_Params.m_MsgFilter.m_CaseSensitive = m_MessageCaseSensitiveChck->GetValue();
	m_Params.m_MsgFilter.m_Regex = m_MessageRegexChck->GetValue();
	m_Params.m_MQTTBroker = m_MQTTCntrl->GetLineText( 0 );

	uint32_t typeFlags = 0;

	if ( m_TypeFilterChck_Info->GetValue() )
	{
		typeFlags |= trace::TMT_Information;
	}

	if ( m_TypeFilterChck_Warning->GetValue() )
	{
		typeFlags |= trace::TMT_Warning;
	}

	if ( m_TypeFilterChck_Error->GetValue() )
	{
		typeFlags |= trace::TMT_Error;
	}

	m_Params.m_MsgFilter.m_TypeFlags = typeFlags;
	m_Params.m_MsgFilter.m_MsgLimit = m_CountLimit->GetValue();
}
