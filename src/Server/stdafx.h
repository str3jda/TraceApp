#pragma warning(push)
#pragma warning (disable : 4996)
#pragma warning (disable : 4191)
#pragma warning (disable : 4365)

#define _HAS_EXCEPTIONS 0
#define wxUSE_UNICODE 0

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <wx/taskbar.h>
#include <wx/spinCtrl.h>
#include <wx/generic/statbmpg.h>
#include <wx/aui/auibook.h>

#pragma warning(pop)

#include <tchar.h>
#include <time.h>

#include "Client/TraceClient.h"

typedef uint32_t T_SourceID;

#define BLACK_THEME false