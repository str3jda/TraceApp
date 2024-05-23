#pragma once

#include <App/UI/UiMessage.h>
#include <App/Utilities/Dequeue.h>
#include <queue>

enum E_Column
{
    E_C_Icon = 0,
    E_C_Msg,
	E_C_LocalTime,
    E_C_Time,
    E_C_Fn,
    E_C_File,
	E_C_Thread,

    E_C_Count
};

class C_MainWindow;
class MsgFilter;

class C_TabModel : public wxDataViewVirtualListModel// public wxDataViewModel
{
public:
    C_TabModel( C_MainWindow* inMainWindow, MsgFilter const& filter );
    ~C_TabModel();

    void Clear();
    void AddMsg( trace::Message const& _msg, ThreadNameHandle inThreadName );

	size_t GetMsgCount() const { return m_Msgs.size(); }

	void SetFilter( MsgFilter const* filter );
	void OnFilterChanged();

	//
	// wxDataViewVirtualListModel impl
	//

	void GetValueByRow( wxVariant &variant, unsigned row, unsigned col ) const override;
	bool SetValueByRow( const wxVariant &variant, unsigned row, unsigned col ) override;

	bool GetAttrByRow( unsigned row, unsigned col, wxDataViewItemAttr& attr ) const override;

	unsigned int GetColumnCount() const override { return E_C_Count; }
	wxString GetColumnType(unsigned int col) const override;

private:

    C_MainWindow* m_MainWindow;

	C_Dequeue< UiMessage, 512 > m_Msgs;
	C_Dequeue< UiMessage*, 512 > m_FilteredMsgs;
	
	MsgFilter const& m_CurrentMsgFilter;

};
