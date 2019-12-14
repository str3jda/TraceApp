#include "stdafx.h"
#include "TabModel.h"
#include "mainwindow.h"
#include "FilterOptions.h"
#include "../Messages/Message.h"

C_TabModel::C_TabModel( C_MainWindow* inMainWindow, C_MsgFilter const& filter )
    : m_MainWindow( inMainWindow )
	, m_CurrentMsgFilter( filter )
{
}

// -------------------------------------------------------------------------------------------------------------------------

C_TabModel::~C_TabModel()
{
}

// -------------------------------------------------------------------------------------------------------------------------

void C_TabModel::Clear()
{
    if (0 < m_Msgs.size())
    {
		while ( !m_Msgs.empty() )
		{
			m_Msgs.pop_front();
		}

// 		for ( size_t i = m_FilteredMsgs.size(); i-- > 0; )
// 		{
// 			m_FilteredMsgs.pop_front();
// 			RowDeleted( 0 );	
// 		}

		m_FilteredMsgs.clear();
		Reset( 0 );

		// notify control
		Cleared();
    }
}

// -------------------------------------------------------------------------------------------------------------------------

void C_TabModel::AddMsg(S_Message const& inMsg, char const* inThreadName)
{
	S_StoredMsg msg;

    msg.m_Type = inMsg.m_Type;
    msg.m_File = inMsg.m_File;
    msg.m_Msg = inMsg.m_Msg;
	msg.m_Thread = inThreadName;

    struct tm * timeinfo;
    timeinfo = localtime(reinterpret_cast<time_t const*>(&inMsg.m_Time));
    msg.m_Time.sprintf("%02d:%02d:%02d %02d.%02d.%04d", 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);

    if ('\0' != inMsg.m_Fn[0])
    {
        msg.m_Fn.sprintf("%s (%u)", inMsg.m_Fn, inMsg.m_Line);
    }

	uint16_t msgCountLimit = m_CurrentMsgFilter.GetMsgCountLimit();
	if ( m_Msgs.size() >= msgCountLimit )
	{
		if ( m_Msgs.front().m_Visible )
		{
			m_FilteredMsgs.pop_front();
			RowDeleted( 0 );
		}

		m_Msgs.pop_front();
	}

	msg.m_Visible = FilterTest( msg );

    m_Msgs.push_back( std::move( msg ) );

	if ( msg.m_Visible )
	{
		m_FilteredMsgs.push_back( &m_Msgs.back() );
		RowAppended();
	}
}

// -------------------------------------------------------------------------------------------------------------------------

void C_TabModel::OnFilterChanged()
{
	uint16_t msgCountLimit = m_CurrentMsgFilter.GetMsgCountLimit();
	while ( m_Msgs.size() > msgCountLimit )
	{
		if ( m_Msgs.front().m_Visible )
		{
			RowDeleted( 0 );
		}

		m_Msgs.pop_front();
	}

	m_FilteredMsgs.clear();

	size_t j = 0;
	for ( S_StoredMsg& msg : m_Msgs )
	{
		bool vis = FilterTest( msg );

		if ( vis && !msg.m_Visible )
		{
			RowInserted( ( uint32_t )j );
			msg.m_Visible = true;
		}
		else if ( !vis && msg.m_Visible )
		{
			RowDeleted( ( uint32_t )j );
			msg.m_Visible = false;
		}

		if ( vis )
		{
			m_FilteredMsgs.push_back( &msg );
			++j;
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------

bool C_TabModel::FilterTest( S_StoredMsg const& msg ) const
{
	return m_CurrentMsgFilter.FilterMsgText( msg.m_Msg, msg.m_Type );
}

// -------------------------------------------------------------------------------------------------------------------------

wxString C_TabModel::GetColumnType(unsigned int col) const
{
	// it's never called anyway...
	return wxString("string");
}

// -------------------------------------------------------------------------------------------------------------------------

void C_TabModel::GetValueByRow( wxVariant &variant, unsigned row, unsigned col ) const
{
	S_StoredMsg const& msg = *m_FilteredMsgs[row];

	switch (col)
	{
	case E_C_Icon:
		variant << m_MainWindow->GetIcon(msg.m_Type);
		break;

	case E_C_Time:
		variant = msg.m_Time;
		break;
	
	case E_C_Msg: 
		variant = msg.m_Msg;
		break;

	case E_C_Fn: 
		variant = msg.m_Fn;
		break;

	case E_C_File: 
		variant = msg.m_File;
		break;

	case E_C_Thread: 
		variant = msg.m_Thread;
		break;
	}
}

bool C_TabModel::SetValueByRow( const wxVariant &variant, unsigned row, unsigned col )
{
	return true;
}

bool C_TabModel::GetAttrByRow( unsigned row, unsigned col, wxDataViewItemAttr& attr ) const
{
	if ( col != 0 )
	{
		S_StoredMsg const& msg = *m_FilteredMsgs[row];

		switch ( msg.m_Type )
		{
		case trace::TMT_Information:
#if BLACK_THEME
			attr.SetBackgroundColour( *wxBLACK );
			attr.SetColour( wxColour( 0x6BB7BD ) );
#else
			//attr.SetBackgroundColour( wxColour( 0x50B4E0 ) );	
			attr.SetColour( wxColour( 0x101010 ) );
#endif
			return true;

		case trace::TMT_Warning:
#if BLACK_THEME
			attr.SetBackgroundColour( wxColour( 0x22BDF7 ) );
			attr.SetColour( wxColour( 0x1 ) );
#else
			//attr.SetBackgroundColour( wxColour( 0x50B4E0 ) );		
			attr.SetBackgroundColour( wxColour( 0x92CDE7 ) );
#endif
			return true;

		case trace::TMT_Error:
#if BLACK_THEME
			attr.SetBackgroundColour( wxColour( 0x3a3dfc ) );
			attr.SetColour( wxColour( 0xffffff ) );
#else
// 			attr.SetBackgroundColour( wxColour( 0x3a3dfc ) );
// 			attr.SetColour( wxColour( 0xffffff ) );
			attr.SetBackgroundColour( wxColour( 0x9aa9f3 ) );
#endif
			return true;


		}
	}

	return false;
}

