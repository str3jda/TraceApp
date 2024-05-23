#include <App/stdafx.h>
#include <App/UI/TabModel.h>
#include <App/UI/MainWindow.h>
#include <App/UI/FilterOptions.h>
#include <Trace/Message.h>

C_TabModel::C_TabModel( C_MainWindow* inMainWindow, MsgFilter const& filter )
    : m_MainWindow( inMainWindow )
	, m_CurrentMsgFilter( filter )
{
}

C_TabModel::~C_TabModel() = default;

// -------------------------------------------------------------------------------------------------------------------------

void C_TabModel::Clear()
{
    if (0 < m_Msgs.size())
    {
		m_Msgs.clear();

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

void C_TabModel::AddMsg(trace::Message const& _msg, ThreadNameHandle inThreadName)
{
	UiMessage msg;

    msg.Type = _msg.Type;
    msg.Msg = _msg.Msg;
	msg.Thread = inThreadName;
	msg.GTime = wxDateTime::Now();
	msg.LocalTime = _msg.LocalTime;
	msg.Fn = _msg.Function;
	msg.File = _msg.File;
	msg.FileLine = _msg.Line;

	uint16_t msgCountLimit = m_CurrentMsgFilter.GetMsgCountLimit();
	if ( m_Msgs.size() >= msgCountLimit )
	{
		if ( m_Msgs.front().Flags & E_MF_Visible )
		{
			m_FilteredMsgs.pop_front();
			RowDeleted( 0 );
		}

		m_Msgs.pop_front();
	}

	msg.Flags = m_CurrentMsgFilter.FilterMsgText( msg.Msg, msg.Type );

    m_Msgs.push_back( std::move( msg ) );

	if ( msg.Flags & E_MF_Visible )
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
		if ( m_Msgs.front().Flags & E_MF_Visible )
		{
			RowDeleted( 0 );
		}

		m_Msgs.pop_front();
	}

	m_FilteredMsgs.clear();

	size_t j = 0;
	for ( UiMessage& msg : m_Msgs )
	{
		MessageFlags_t flags = m_CurrentMsgFilter.FilterMsgText( msg.Msg, msg.Type );
		bool isVisible = ( ( flags & E_MF_Visible ) != 0 );

		if ( isVisible && ( ( msg.Flags & E_MF_Visible ) == 0 ) )
			RowInserted( ( uint32_t )j );
		else if ( !isVisible && ( ( msg.Flags & E_MF_Visible ) != 0 ) )
			RowDeleted( ( uint32_t )j );

		msg.Flags = flags;

		if ( isVisible )
		{
			m_FilteredMsgs.push_back( &msg );
			++j;
		}
	}
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
	UiMessage const& msg = *m_FilteredMsgs[row];

	switch (col)
	{
	case E_C_Icon:
		variant << m_MainWindow->GetIcon( msg.Type );
		break;
	
	case E_C_Msg: 
		variant = (void*)&msg;
		break;

	case E_C_LocalTime: 
	{
		wxString lt_str;
		if ( msg.LocalTime != 0 )
			lt_str.sprintf( "%u", msg.LocalTime );
		else
			lt_str = msg.GTime.FormatTime();

		variant = std::move( lt_str );
		break;
	}
	case E_C_Time:
		variant = msg.GTime.Format( "%d.%m.%y" );
		break;

	case E_C_Fn: 
		variant = msg.Fn;
		break;

	case E_C_File: 
	{
		wxString file_str;
	 	if ( !msg.File.empty() )
        	file_str.sprintf( "%s (%u)", msg.File.c_str(), msg.FileLine );

		variant = std::move ( file_str );
		break;
	}
	case E_C_Thread: 
		variant = m_MainWindow->ResolveThreadName( msg.Thread );
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
		UiMessage const& msg = *m_FilteredMsgs[row];

		switch ( msg.Type )
		{
		case trace::TMT_Information:
#if BLACK_THEME
			attr.SetColour( wxColour( 0xffffff ) );
#else
			attr.SetColour( wxColour( 0x101010 ) );
#endif
			return true;

		case trace::TMT_Log:
#if BLACK_THEME
			attr.SetColour( wxColour( 0xffffff ) );
#else
			attr.SetColour( wxColour( 0x101010 ) );
#endif
			return true;

		case trace::TMT_Warning:
#if BLACK_THEME
			attr.SetColour( wxColour( 0x81C0C5 ) );
#else
			attr.SetColour( wxColour( 0x92CDE7 ) );
#endif
			return true;

		case trace::TMT_Error:
#if BLACK_THEME
			attr.SetColour( wxColour( 0x3333ff ) );
#else
			attr.SetColour( wxColour( 0x3333ff ) );
#endif
			return true;


		}
	}

	return false;
}

