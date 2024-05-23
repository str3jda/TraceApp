#pragma once

#pragma warning(push)
#pragma warning (disable : 4996)
#pragma warning (disable : 4191)
#pragma warning (disable : 4365)

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <wx/taskbar.h>
#include <wx/spinCtrl.h>
#include <wx/generic/statbmpg.h>
#include <wx/aui/auibook.h>
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/itemattr.h>
#include <wx/headerctrl.h>

#pragma warning(pop)

#include <tchar.h>
#include <time.h>

#ifdef SendMessage
#	undef SendMessage
#endif

#include <Trace/Trace.h>

using ThreadNameHandle = uint32_t;
static constexpr ThreadNameHandle InvalidThreadNameHandle = ~0u;
