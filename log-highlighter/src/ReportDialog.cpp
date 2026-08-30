#include "ReportDialog.h"

static constexpr int     IDC_REPORT_EDIT = 201;
static const wchar_t     kClass[]        = L"LogHLReportWnd";

static constexpr int kDefaultWidth  = 720;
static constexpr int kDefaultHeight = 460;
static constexpr int kMinWidth      = 320;
static constexpr int kMinHeight     = 180;
static constexpr int kMargin        = 8;

// Per-window state stored in GWLP_USERDATA
struct ReportDlgState
{
    HWND   hwndEdit = nullptr;
    HFONT  hFont    = nullptr;
};

// Text to install, handed through CREATESTRUCT::lpCreateParams.
struct CreateParams
{
    const std::wstring* text;
};

static LRESULT CALLBACK ReportWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* s = reinterpret_cast<ReportDlgState*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* state = new ReportDlgState{};
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(state));

        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);

        RECT rc = {};
        GetClientRect(hwnd, &rc);

        state->hwndEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            kMargin, kMargin,
            rc.right - 2 * kMargin, rc.bottom - 2 * kMargin,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_REPORT_EDIT)),
            cs->hInstance, nullptr);

        if (state->hwndEdit)
        {
            // The default limit is 32767 characters and truncates silently.
            SendMessageW(state->hwndEdit, EM_SETLIMITTEXT, 0, 0);

            // Fixed pitch, or the aligned key columns fall apart.
            HDC hdc = GetDC(hwnd);
            const int height = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ReleaseDC(hwnd, hdc);

            state->hFont = CreateFontW(
                height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

            if (state->hFont)
                SendMessageW(state->hwndEdit, WM_SETFONT,
                             reinterpret_cast<WPARAM>(state->hFont), TRUE);

            auto* params = reinterpret_cast<CreateParams*>(cs->lpCreateParams);
            if (params && params->text)
                SetWindowTextW(state->hwndEdit, params->text->c_str());

            // Caret at the top with nothing selected.
            SendMessageW(state->hwndEdit, EM_SETSEL, 0, 0);
        }

        return 0;
    }

    case WM_SIZE:
        if (s && s->hwndEdit)
        {
            const int w = LOWORD(lp);
            const int h = HIWORD(lp);
            MoveWindow(s->hwndEdit, kMargin, kMargin,
                       w - 2 * kMargin, h - 2 * kMargin, TRUE);
        }
        return 0;

    case WM_GETMINMAXINFO:
    {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize = { kMinWidth, kMinHeight };
        return 0;
    }

    case WM_SETFOCUS:
        if (s && s->hwndEdit) SetFocus(s->hwndEdit);
        return 0;

    // A read-only EDIT is painted with the button face colour by default, which
    // reads as "disabled". Paint it like a document instead.
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        if (s && reinterpret_cast<HWND>(lp) == s->hwndEdit)
        {
            auto hdc = reinterpret_cast<HDC>(wp);
            SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        }
        break;

    case WM_COMMAND:
        // IsDialogMessage turns ESC into IDCANCEL.
        if (LOWORD(wp) == IDCANCEL)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (s)
        {
            if (s->hFont) DeleteObject(s->hFont);
            delete s;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        // Ends the nested modal loop in ShowReportDialog. The WM_QUIT is
        // consumed by that loop's GetMessage, so Notepad++ never sees it.
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------

void ShowReportDialog(HWND                hParent,
                      HINSTANCE           hInst,
                      const wchar_t*      reportTitle,
                      const std::wstring& text)
{
    // Register class once (ignore ERROR_CLASS_ALREADY_EXISTS)
    {
        WNDCLASSW wc     = {};
        wc.lpfnWndProc   = ReportWndProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = kClass;
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
    }

    std::wstring caption = L"log-highlighter";
    if (reportTitle && *reportTitle)
    {
        caption += L" - ";
        caption += reportTitle;
    }

    // Centre over parent
    RECT rc = {};
    GetWindowRect(hParent, &rc);
    const int x = (rc.left + rc.right  - kDefaultWidth)  / 2;
    const int y = (rc.top  + rc.bottom - kDefaultHeight) / 2;

    CreateParams params{ &text };

    HWND hwnd = CreateWindowExW(
        0,
        kClass,
        caption.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
            WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        x, y, kDefaultWidth, kDefaultHeight,
        hParent, nullptr, hInst, &params);

    if (!hwnd) return;

    // Modal: block the parent and run our own loop, so Notepad++'s accelerator
    // table never gets a chance to intercept keys meant for the edit control.
    EnableWindow(hParent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        // The window is already gone once ESC or the close box has run, but
        // messages posted before that still drain through here — IsDialogMessage
        // must not be handed a destroyed HWND.
        if (IsWindow(hwnd) && IsDialogMessageW(hwnd, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
}
