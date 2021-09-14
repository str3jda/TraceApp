#pragma once

#include "FilterOptions.h"

class C_TabModel;

struct S_SettingParams
{
    bool			m_StayOnTop;
	bool			m_ResetOnProcInstance;
	bool			m_AutoScroll;
	S_FilterOptions m_MsgFilter;
	std::string		m_MQTTBroker;

	S_SettingParams()
		: m_StayOnTop( false )
		, m_ResetOnProcInstance( false )
		, m_AutoScroll( true )
    {
	}
};

class C_SettingsDialog : public wxDialog
{
public:
	C_SettingsDialog( S_SettingParams& outParams, wxIcon const& infoIcon, wxIcon const& warningIcon, wxIcon const& errorIcon );

private:

	void OnOk(wxCommandEvent& event);
	void OnKeyEnter(wxKeyEvent& event);

	void ApplyChanges();

private:
    
	S_SettingParams&	m_Params;

	wxCheckBox*			m_StayOnTopChck;
	wxCheckBox*			m_ResetOnNewProcessInstanceChck;

	wxTextCtrl*			m_MessageFilterCntrl;
	wxCheckBox*			m_MessageCaseSensitiveChck;
	wxCheckBox*			m_MessageRegexChck;

	wxCheckBox*			m_TypeFilterChck_Info;
	wxCheckBox*			m_TypeFilterChck_Warning;
	wxCheckBox*			m_TypeFilterChck_Error;

	wxSpinCtrl*			m_CountLimit;

	wxTextCtrl*			m_MQTTCntrl;

	wxDECLARE_EVENT_TABLE();
};