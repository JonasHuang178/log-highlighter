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

// True when config/CustomReports.h sets REPORT_DEBUG_MODE. Lets Plugin.cpp act
// on the flag without including CustomReports.h, which must stay included by
// Report.cpp alone.
bool ReportDebugEnabled();

// Writes one engine breadcrumb line to the debug console. Exempt from the
// author output cap, and a no-op when debug mode is off or the console is
// closed. `text` is ASCII.
void ReportEngineLog(const char* text);

// Same, for text that has to carry a file name.
void ReportEngineLogW(const wchar_t* text);
