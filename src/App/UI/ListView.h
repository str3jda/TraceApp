
class ListView : public wxDataViewCtrl
{
public:

	bool NewMessagesCame = false;
	bool const SystemView = false;

	ListView( wxWindow* _parent, bool _system_view = false )
		: wxDataViewCtrl( 
			_parent, 
			wxID_ANY, 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxDV_MULTIPLE /*| wxDV_NO_HEADER*/ | wxBORDER_NONE )
		, SystemView( _system_view )
	{}

	void OnMotion(wxMouseEvent& _event);
	void OnContextMenu( wxDataViewEvent& _event );
	void OnSetCursor( wxSetCursorEvent& _event );

	void OnSelectionChanged( wxDataViewEvent& _event )
	{		
	}

	void OnKeyDown( wxKeyEvent& _event );
	void OnSize( wxSizeEvent& event );

private:

	int m_LastWidth = -1;
	wxDataViewItem m_LastMouseOverItem; 

	wxDECLARE_EVENT_TABLE();
};