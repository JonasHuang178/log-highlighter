#pragma once
#include <windows.h>
#include <string>

// ---------------------------------------------------------------------------
//  ReportDialog - modal, read-only window that displays report output.
//
//  Modal on purpose. Notepad++ runs its accelerator table before dispatching
//  messages, so a modeless window would need NPPM_MODELESSDIALOG registration
//  and Ctrl+C could still be swallowed by Notepad++'s own Copy command. A modal
//  loop owns its input, so Ctrl+C, Ctrl+A and ESC all behave normally.
//
//  `text` must already use CRLF line endings — the edit control ignores a bare
//  LF and would render the whole report as one line.
//
//  Copying works two ways: selection plus the edit control's built-in
//  right-click Copy / Select All, and Ctrl+C. There is no Copy button.
// ---------------------------------------------------------------------------
void ShowReportDialog(HWND               hParent,
                      HINSTANCE          hInst,
                      const wchar_t*     reportTitle,
                      const std::wstring& text);
