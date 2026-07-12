## 1. Add SetProgressApplying() to ProgressDialog

- [x] 1.1 In `src/ProgressDialog.h`, declare `void SetProgressApplying(HWND hDlg)`
- [x] 1.2 In `src/ProgressDialog.cpp`, implement `SetProgressApplying`: set label text to `L"Applying highlights…"` and hide the Cancel button (`ShowWindow(GetDlgItem / find IDC_CANCEL child, SW_HIDE)`), then call `UpdateWindow(hDlg)` to force an immediate repaint

## 2. Update ParseLog() in Plugin.cpp

- [x] 2.1 After `IsProgressCancelled` check and before `ClearAllHighlights`, call `SetProgressApplying(hDlg)`
- [x] 2.2 Move `EnableWindow(g_nppData._nppHandle, TRUE)`, `SetForegroundWindow`, and `DestroyWindow(hDlg)` to after `ApplyHighlights()` and `g_overviewPanel.Update()` complete (i.e., at the very end of the non-cancelled path)
