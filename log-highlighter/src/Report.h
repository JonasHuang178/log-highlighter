#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
//  Report - runs the report functions registered in config/CustomReports.h.
//
//  Plugin.cpp talks to reports only through this header, so CustomReports.h
//  stays included by exactly one translation unit.
// ---------------------------------------------------------------------------

// Number of reports registered in CUSTOM_REPORTS[].
int CustomReportCount();

// Menu title of report `index`. Returns L"" if the index is out of range.
const wchar_t* CustomReportTitle(int index);

// Shortcut letter of report `index`, or 0 when it has none.
char CustomReportShortcut(int index);

// Runs report `index` against the active document and shows the result.
// Safe to call with an out-of-range index (does nothing).
void RunCustomReport(int index);
