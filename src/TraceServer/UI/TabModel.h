#pragma once

#include <queue>
#include "../Utilities/Dequeue.h"

enum E_Column
{
    E_C_Icon = 0,
    E_C_Time,
    E_C_Msg,
    E_C_Fn,
    E_C_File,
	E_C_Thread,

    E_C_Count
};

class C_MainWindow;
class C_MsgFilter;
struct S_Message;

class C_TabModel : public wxDataViewVirtualListModel// public wxDataViewModel
{
	struct S_StoredMsg;

public:
    C_TabModel( C_MainWindow* inMainWindow, C_MsgFilter const& filter );
    ~C_TabModel();

    void Clear();
    void AddMsg(S_Message const& inMsg, char const* inThreadName);

	size_t GetMsgCount() const { return m_Msgs.size(); }

	void SetFilter( C_MsgFilter const* filter );
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

	bool FilterTest( S_StoredMsg const& msg ) const;

    struct S_StoredMsg
    {
        trace::E_TracedMessageType m_Type;
		bool m_Visible : 1;

        wxString m_Time;
        wxString m_Msg;
        wxString m_Fn;
        wxString m_File;
		wxString m_Thread;

        S_StoredMsg(){}

        S_StoredMsg(S_StoredMsg && other)
			: m_Type( other.m_Type )
			, m_Time( std::move( other.m_Time ) )
			, m_Msg( std::move( other.m_Msg ) )
			, m_Fn( std::move( other.m_Fn ) )
			, m_File( std::move( other.m_File ) )
			, m_Thread( std::move( other.m_Thread ) )
			, m_Visible( other.m_Visible )
        {}
    };

private:

    C_MainWindow* m_MainWindow;

	C_Dequeue< S_StoredMsg, 512 > m_Msgs;
	C_Dequeue< S_StoredMsg*, 512 > m_FilteredMsgs;
	
	C_MsgFilter const& m_CurrentMsgFilter;

};
