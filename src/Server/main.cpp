#include "stdafx.h"
#include "UI/mainwindow.h"
#include "Utilities/StackWalker.h"

#include <csignal>

class C_TraceServerApp : public wxApp
{
public:

	virtual bool OnInit();
	virtual int	OnExit();

	void OnKeyPressEvent(wxKeyEvent& event);
	
private:
	C_MainWindow* m_Wnd;
	bool m_Browsing = true;

	wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(C_TraceServerApp, wxApp)
	EVT_KEY_UP(C_TraceServerApp::OnKeyPressEvent)
wxEND_EVENT_TABLE()

// -------------------------------------------------------------------------------------------------------------------------
wxIMPLEMENT_APP(C_TraceServerApp);
// -------------------------------------------------------------------------------------------------------------------------

class C_AssertStackWalker : public StackWalker
{
public:
	C_AssertStackWalker(FILE* f)
		: m_File(f)
	{
	}
	
	virtual void OnCallstackEntry(CallstackEntryType eType, int frameIdx, CallstackEntry &entry)
	{
		if (frameIdx >= 0 && '\0' != entry.lineFileName[0])
		{ // first is not interesting and if filename is not present, than i don't care (some system libs or whatever)
			StackWalker::OnCallstackEntry(eType, frameIdx, entry);
		}
	}

	virtual void OnOutput(LPCSTR szText)
	{
		char line[1024];
		strcpy_s(line, "\t");
		strcat_s(line, szText);

		fputs( line, m_File );
	}

private:
	FILE* m_File;
};

// -------------------------------------------------------------------------------------------------------------------------

long OnCrash( _EXCEPTION_POINTERS* exceptionInfo )
{
	time_t t;
	time( &t );

	struct tm* ti = localtime( &t );

	char file_path[256];
	sprintf_s( file_path, "crash %d.%d.%d %d.%d.%d.txt", ti->tm_mday, ti->tm_mon, 1900 + ti->tm_year, ti->tm_hour, ti->tm_min, ti->tm_sec );

	FILE* f = fopen( file_path, "w" );
	fprintf( f, "REASON: %d\n", exceptionInfo->ExceptionRecord->ExceptionCode );

	fputs( "CALLSTACK:\n", f );
	C_AssertStackWalker sw( f );
	sw.ShowCallstack( GetCurrentThread(), exceptionInfo->ContextRecord );

	fclose( f );
	return EXCEPTION_EXECUTE_HANDLER;
}

// -------------------------------------------------------------------------------------------------------------------------

void SignalHandler( int signal )
{
	//volatile std::sig_atomic_t.

	time_t t;
	time( &t );

	struct tm* ti = localtime( &t );

	char file_path[ 256 ];
	if ( signal != 0 )
	{
		sprintf_s( file_path, "crash %d.%d.%d %d.%d.%d.txt", ti->tm_mday, ti->tm_mon, 1900 + ti->tm_year, ti->tm_hour, ti->tm_min, ti->tm_sec );
	}
	else
	{
		sprintf_s( file_path, "clean_exit %d.%d.%d %d.%d.%d.txt", ti->tm_mday, ti->tm_mon, 1900 + ti->tm_year, ti->tm_hour, ti->tm_min, ti->tm_sec );
	}

	FILE* f = fopen( file_path, "w" );
	fprintf( f, "REASON: signal:%d\n", signal );

	fputs( "CALLSTACK:\n", f );
	C_AssertStackWalker sw( f );
	sw.ShowCallstack();

	fclose( f );
}

// -------------------------------------------------------------------------------------------------------------------------

void AtexitHandler()
{
	SignalHandler( 0 );
}

// -------------------------------------------------------------------------------------------------------------------------

bool C_TraceServerApp::OnInit()
{
	std::signal( SIGSEGV, &SignalHandler );
	std::signal( SIGFPE, &SignalHandler );	
	atexit( &AtexitHandler );

	if (!wxApp::OnInit())
	{
		return false;
	}

	// create main window
	m_Wnd = new C_MainWindow();
	m_Wnd->Show(true);

	return true;
}

// -------------------------------------------------------------------------------------------------------------------------

int C_TraceServerApp::OnExit()
{
	m_Wnd = nullptr;

	int ret = wxApp::OnExit();

	return ret;
}

// -------------------------------------------------------------------------------------------------------------------------

void C_TraceServerApp::OnKeyPressEvent(wxKeyEvent& event)
{
	if ( m_Browsing )
	{
		switch ( event.GetKeyCode() )
		{
			case WXK_F11:
				m_Browsing = false;
				m_Wnd->ShowSettingsDialog();
				m_Browsing = true;
				break;

			case WXK_INSERT:
				m_Wnd->ToggleAutoScroll();
				break;

			case WXK_DELETE:
				m_Wnd->DeleteCurrentPageContent();
				break;
		}
	}
}