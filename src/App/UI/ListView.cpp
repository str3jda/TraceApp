#include <App/stdafx.h>
#include <App/UI/ListView.h>
#include <App/UI/TabModel.h>
#include <App/UI/FormatedRenderer.h>
#include <bit>

wxBEGIN_EVENT_TABLE( ListView, wxDataViewCtrl )
	EVT_KEY_DOWN( ListView::OnKeyDown )
	EVT_DATAVIEW_SELECTION_CHANGED( wxID_ANY, ListView::OnSelectionChanged )
	EVT_DATAVIEW_ITEM_CONTEXT_MENU( wxID_ANY, ListView::OnContextMenu )
	//EVT_MOTION( ListView::OnMotion )
	EVT_SIZE ( ListView::OnSize )
	EVT_SET_CURSOR( ListView::OnSetCursor )
wxEND_EVENT_TABLE()

template < size_t TMarkSize >
static bool test_mark( char const* _str, char const ( &_mark )[TMarkSize] ) 
{ 
	if      constexpr ( TMarkSize == 2 ) return _str[0] == _mark[0];
	else if constexpr ( TMarkSize == 3 ) return *std::bit_cast< uint16_t const* >( _str ) == *std::bit_cast< uint16_t const* >( &_mark[0] );
	else static_assert( false, "TMarkSize is unsupported" );
}

template < size_t TMarkSize >
char const* find_block( char const* _text, char const* _text_end, char const ( &_mark )[TMarkSize] )
{
	char const* x = _text;
	
	// It can read behind the memory, but it shouldn't matter. There's always '\0', so it
	// will never actually find anything there
	while ( ( x < _text_end ) && !test_mark( x, _mark ) )
		x++;

	return x;
}

void ListView::OnSetCursor( wxSetCursorEvent& _event )
{
	wxDataViewItem item;
	wxDataViewColumn* column = nullptr;
	HitTest( ScreenToClient( wxGetMousePosition() ), item, column );

	if ( item.IsOk() && ( m_LastMouseOverItem != item ) )
	{
		m_LastMouseOverItem = item;

		wxVariant value_variant;
		GetModel()->GetValue( value_variant, item, E_C_Msg );

		UiMessage const& msg = *reinterpret_cast< UiMessage const* >( value_variant.GetVoidPtr() );
		
		wxString msg_sanatized = C_FormatedRenderer::StripFormatting( msg.Msg );

		wxString tip;
		tip.sprintf( "%s\nTime: %u %s\nFunction: %s\nFile: %s (%u)",
			msg_sanatized.c_str(),
			msg.LocalTime, msg.GTime.Format( "%d.%m.%y" ).c_str(),
			msg.Fn.c_str(),
			msg.File.c_str(), msg.FileLine );
		
		// Need this reset, otherwise delay-to-show-up won't apply if already tooltip-ing
		SetToolTip( nullptr );
		SetToolTip( tip );
	}
}

void ListView::OnKeyDown( wxKeyEvent& _event )
{
	if ( ( _event.GetKeyCode() == 'C' ) && ( _event.GetModifiers() == wxMOD_CONTROL ) )
	{
		if (wxTheClipboard->Open())
		{
			wxTextDataObject* data = new wxTextDataObject();

			wxDataViewItemArray selections;
			GetSelections( selections );

			wxString text;
			for ( wxDataViewItem item : selections )
			{
				if ( item.IsOk() )
				{	
					wxVariant value_variant;
					GetModel()->GetValue( value_variant, item, E_C_Msg );

					UiMessage const& msg = *reinterpret_cast< UiMessage const* >( value_variant.GetVoidPtr() );
					
					GetModel()->GetValue( value_variant, item, E_C_Thread );
					wxString resolved_thread_name = value_variant.GetString();

					wxString row;
					row.sprintf("%s;%u;%s;%s;%s;%s\r\n",
						msg.Msg.c_str(),
						msg.LocalTime,
						msg.GTime.FormatISOCombined( ' ' ).c_str(),
						msg.Fn.c_str(),
						msg.File.c_str(),
						resolved_thread_name.c_str());

					text += row;
				}
			}

			data->SetText( text );
			wxTheClipboard->SetData( data );

			wxTheClipboard->Flush();
			wxTheClipboard->Close();
		}
	}
}

///////////////////////////////////////////////////////////////

void ListView::OnContextMenu( wxDataViewEvent& _event )
{
	wxDataViewItem item = _event.GetItem();
	wxVariant value_variant;
	GetModel()->GetValue( value_variant, item, E_C_Msg );

	UiMessage const& msg = *reinterpret_cast< UiMessage const* >( value_variant.GetVoidPtr() );
	SetToolTip( msg.Msg );

	/*
	wxDataViewItem item;
	wxDataViewColumn* column = nullptr;
	HitTest(ScreenToClient(wxGetMousePosition()), item, column);

	if ( item.IsOk() )
	{
		unsigned colIdx = column ? column->GetModelColumn() : 0;

		wxVariant value;
		GetModel()->GetValue(value, item, colIdx);
		//wxLogMessage("Item under mouse is \"%s\"", value.GetString());
	}
	else
	{
		//wxLogMessage("No item under mouse");
	}
	*/
}

void ListView::OnMotion(wxMouseEvent& _event)
{
}

///////////////////////////////////////////////////////////////

void ListView::OnSize( wxSizeEvent& event )
{
	int w;
	GetClientSize( &w, nullptr );

	if ( m_LastWidth != w )
	{
		m_LastWidth = w;

		wxDataViewColumn* msg_col = nullptr;
		int used_width = 0;

		for ( unsigned i = 0, n = GetColumnCount(); i < n; ++i )
		{
			wxDataViewColumn* col = GetColumnAt( i );
			if ( i == E_C_Msg )
				msg_col = col;
			else
				used_width += col->GetWidth();
		}

		if ( msg_col != nullptr )
			msg_col->SetWidth( std::max( m_LastWidth - used_width, msg_col->GetMinWidth() ) );
	}

	event.Skip();
}
