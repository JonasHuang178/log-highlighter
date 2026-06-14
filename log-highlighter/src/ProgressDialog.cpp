#include "ProgressDialog.h"

static constexpr int      IDC_TEXT   = 101;
static constexpr int      IDC_CANCEL = 102;
static const wchar_t      kClass[]   = L"LogHLProgressWnd";

// Per-window state stored in GWLP_USERDATA
struct DlgState
{
    HWND hwndText  = nullptr;
    bool cancelled = false;
};

static LRESULT CALLBACK ProgressWndProc(HWND hwnd, UINT msg,
                                         WPARAM wp, LPARAM lp)
{
    DlgState* s = reinterpret_cast<DlgState*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* s2 = new DlgState{};
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(s2));

        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);

        s2->hwndText = CreateWindowW(
            L"STATIC", L"Preparing...",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 28, 290, 20,
            hwnd, reinterpret_cast<HMENU>(IDC_TEXT),
            cs->hInstance, nullptr);

        CreateWindowW(
            L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            110, 72, 90, 28,
            hwnd, reinterpret_cast<HMENU>(IDC_CANCEL),
            cs->hInstance, nullptr);

        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        // Lock window size — no resize handle, fixed dimensions.
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize = { 320, 140 };
        mmi->ptMaxTrackSize = { 320, 140 };
        return 0;
    }
    case WM_COMMAND:
        if (s && (LOWORD(wp) == IDC_CANCEL || LOWORD(wp) == IDCANCEL))
            s->cancelled = true;
        return 0;

    case WM_CLOSE:
        // Treat X button as Cancel (don't destroy — caller destroys)
        if (s) s->cancelled = true;
        return 0;

    case WM_DESTROY:
        delete s;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------

HWND CreateProgressDialog(HWND hParent, HINSTANCE hInst)
{
    // Register class once (ignore ERROR_CLASS_ALREADY_EXISTS)
    {
        WNDCLASSW wc         = {};
        wc.lpfnWndProc       = ProgressWndProc;
        wc.hInstance         = hInst;
        wc.lpszClassName     = kClass;
        wc.hbrBackground     = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.hCursor           = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
    }

    // Centre over parent
    RECT rc = {};
    GetWindowRect(hParent, &rc);
    const int W = 320, H = 140;
    const int x = (rc.left + rc.right  - W) / 2;
    const int y = (rc.top  + rc.bottom - H) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClass,
        L"log-highlighter",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, W, H,
        hParent, nullptr, hInst, nullptr);

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    return hwnd;
}

void SetProgressLine(HWND hDlg, int current, int total)
{
    if (!hDlg) return;
    auto* s = reinterpret_cast<DlgState*>(
        GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    if (!s || !s->hwndText) return;

    wchar_t buf[64];
    wsprintfW(buf, L"Processing: %d / %d lines", current, total > 0 ? total : current);
    SetWindowTextW(s->hwndText, buf);
}

bool IsProgressCancelled(HWND hDlg)
{
    if (!hDlg) return false;
    auto* s = reinterpret_cast<DlgState*>(
        GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    return s && s->cancelled;
}
