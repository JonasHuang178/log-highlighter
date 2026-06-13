#pragma once
#include "../external/PluginInterface.h"

extern NppData    g_nppData;
extern HINSTANCE  g_hInstance;

// Returns the HWND of the currently active Scintilla view.
HWND GetCurrentScintilla();
