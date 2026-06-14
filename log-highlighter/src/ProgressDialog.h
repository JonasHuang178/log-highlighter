#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
//  ProgressDialog — lightweight modeless progress window
//
//  All functions run on the UI thread. The caller pumps messages itself
//  (via PeekMessage) inside the parse loop so the window stays responsive.
//
//  Typical usage:
//    HWND hDlg = CreateProgressDialog(hParent, hInst);
//    EnableWindow(hParent, FALSE);                // block NPP input
//    for each chunk:
//        SetProgressLine(hDlg, cur, total);
//        MSG m; while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE))
//               { TranslateMessage(&m); DispatchMessage(&m); }
//        if (IsProgressCancelled(hDlg)) break;
//    EnableWindow(hParent, TRUE);
//    DestroyWindow(hDlg);
// ---------------------------------------------------------------------------

// Creates and shows the progress window. Returns its HWND (caller destroys).
HWND CreateProgressDialog(HWND hParent, HINSTANCE hInst);

// Updates the "Processing X / Y lines" text. Call from the UI thread.
void SetProgressLine(HWND hDlg, int current, int total);

// Returns true once the user has clicked Cancel.
bool IsProgressCancelled(HWND hDlg);
