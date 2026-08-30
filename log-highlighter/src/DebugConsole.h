#pragma once
#include <windows.h>
#include <cstddef>

// ---------------------------------------------------------------------------
//  DebugConsole - the console window used by REPORT_DEBUG_MODE.
//
//  Only allocated when debug mode is enabled, and only from the NPPN_READY
//  handler: AllocConsole from DllMain would run under the loader lock.
//
//  The close box is removed on purpose. A console allocated by a process
//  belongs to that process, and closing its window sends CTRL_CLOSE_EVENT,
//  whose default handling terminates the owner — Notepad++, with every unsaved
//  tab. Somebody dismissing what looks like a stray console would lose their
//  work, so the box is disabled and a control handler is installed behind it.
//  The window therefore stays for the session; minimising is the way to get it
//  out of the way.
// ---------------------------------------------------------------------------

// Allocates and prepares the console. Safe to call more than once.
void OpenDebugConsole();

// Releases the console, if one was allocated.
void CloseDebugConsole();

bool IsDebugConsoleOpen();

// Writes UTF-16 text verbatim. Nothing is appended — callers add line endings.
void DebugConsoleWrite(const wchar_t* text, size_t len);
