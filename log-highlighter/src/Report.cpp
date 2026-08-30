#include "Report.h"
#include "Plugin.h"
#include "Parser.h"
#include "ProgressDialog.h"
#include "ReportDialog.h"
#include "../external/Scintilla.h"

// The one and only inclusion of the report authors' file. It defines functions,
// not just data, so including it a second time would create a second private
// copy of every report.
#include "../config/CustomReports.h"

#include <algorithm>
#include <string>

static constexpr int kReportCount =
    static_cast<int>(sizeof(CUSTOM_REPORTS) / sizeof(CUSTOM_REPORTS[0]));

static bool g_reportInProgress = false;   // re-entrancy guard

// ---------------------------------------------------------------------------
// Registration accessors
// ---------------------------------------------------------------------------
int CustomReportCount() { return kReportCount; }

const wchar_t* CustomReportTitle(int index)
{
    if (index < 0 || index >= kReportCount) return L"";
    return CUSTOM_REPORTS[index].title;
}

char CustomReportShortcut(int index)
{
    if (index < 0 || index >= kReportCount) return 0;
    return CUSTOM_REPORTS[index].shortcut;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Progress sink handed to the iterators in ReportApi.h. Called every
// REPORT_TICK_INTERVAL lines from inside ctx.Lines() / ctx.FindAll(), which is
// what keeps the dialog painted and Cancel clickable without the report author
// doing anything.
static bool ReportTick(void* self, int lineNo, int totalLines)
{
    HWND hDlg = static_cast<HWND>(self);

    SetProgressLine(hDlg, lineNo, totalLines);

    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return !IsProgressCancelled(hDlg);
}

// Full path of the active document. Returns an empty string when Notepad++
// does not fill the buffer, so a wrong message id degrades to a placeholder
// rather than showing garbage.
static std::wstring CurrentFilePath()
{
    wchar_t buf[MAX_PATH * 2] = { 0 };

    ::SendMessage(g_nppData._nppHandle,
                  NPPM_GETFULLCURRENTPATH,
                  static_cast<WPARAM>(MAX_PATH * 2 - 1),
                  reinterpret_cast<LPARAM>(buf));

    buf[MAX_PATH * 2 - 1] = L'\0';
    return std::wstring(buf);
}

static std::wstring FileNameOf(const std::wstring& path)
{
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return path;
    return path.substr(pos + 1);
}

// 45210 -> "45,210"
static std::string GroupDigits(long long value)
{
    std::string digits = std::to_string(value);
    std::string out;
    int count = 0;

    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i)
    {
        out += digits[static_cast<size_t>(i)];
        if (++count % 3 == 0 && i > 0) out += ',';
    }

    std::reverse(out.begin(), out.end());
    return out;
}

static std::wstring ToWide(const std::string& s, UINT codePage)
{
    if (s.empty()) return {};

    const int n = ::MultiByteToWideChar(codePage, 0, s.data(),
                                        static_cast<int>(s.size()),
                                        nullptr, 0);
    if (n <= 0) return {};

    std::wstring w(static_cast<size_t>(n), L'\0');
    ::MultiByteToWideChar(codePage, 0, s.data(), static_cast<int>(s.size()),
                          &w[0], n);
    return w;
}

// The edit control needs CRLF; ReportBuilder emits LF.
static std::wstring ToCrLf(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size() + s.size() / 16 + 16);

    for (wchar_t c : s)
    {
        if (c == L'\n') out += L'\r';
        out += c;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Command: Custom Report  (Ctrl+Alt+E for CUSTOM_REPORTS[0] by default)
// ---------------------------------------------------------------------------
void RunCustomReport(int index)
{
    if (index < 0 || index >= kReportCount) return;
    if (g_reportInProgress) return;
    g_reportInProgress = true;

    HWND hSci = GetCurrentScintilla();
    if (!hSci) { g_reportInProgress = false; return; }

    BufferState& buf = CurrentBuffer();
    InvalidateIfStale(buf);

    const std::wstring path     = CurrentFilePath();
    const std::wstring fileName = path.empty() ? L"(untitled)" : FileNameOf(path);
    const wchar_t*     title    = CUSTOM_REPORTS[index].title;

    // Cache hit — same report, same buffer, no edit since it was rendered.
    if (buf.reportIndex == index && !buf.reportText.empty())
    {
        g_reportInProgress = false;
        ShowReportDialog(g_nppData._nppHandle, g_hInstance, title, buf.reportText);
        return;
    }

    std::vector<char> snapshot = SnapshotDocument(hSci);

    const int totalLines = static_cast<int>(
        ::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0));

    // Show progress and block the Notepad++ window so the command cannot be
    // re-entered while the report pumps messages.
    HWND hDlg = CreateProgressDialog(g_nppData._nppHandle, g_hInstance);
    ::EnableWindow(g_nppData._nppHandle, FALSE);
    SetProgressLine(hDlg, 0, totalLines);

    ProgressSink sink;
    sink.tick = &ReportTick;
    sink.self = hDlg;

    ReportContext ctx;
    ctx.text      = snapshot.empty() ? "" : snapshot.data();
    ctx.length    = snapshot.size();
    ctx.lineCount = totalLines;
    ctx.fileName  = fileName.c_str();
    ctx.filePath  = path.c_str();
    ctx.sink      = &sink;

    ReportBuilder out;

    // Called directly and deliberately: there is no crash guard, so a stray
    // pointer in a report function takes down Notepad++. See design.md
    // Decision 9 — the helper contract in ReportApi.h is the mitigation.
    CUSTOM_REPORTS[index].fn(ctx, out);

    const bool cancelled = IsProgressCancelled(hDlg);

    ::EnableWindow(g_nppData._nppHandle, TRUE);
    ::SetForegroundWindow(g_nppData._nppHandle);
    ::DestroyWindow(hDlg);
    g_reportInProgress = false;

    if (cancelled) return;   // discard the partial report, show nothing

    // ---- assemble ---------------------------------------------------------
    std::string body = out.Empty() ? std::string("(no output)") : out.Render();

    const UINT codePage =
        (static_cast<int>(::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0)) == SC_CP_UTF8)
            ? CP_UTF8
            : CP_ACP;

    // The generated header is built in UTF-16 directly and kept ASCII apart
    // from the file name, so it does not depend on the document's code page.
    std::wstring headerLine;
    headerLine  = title;
    headerLine += L" - ";
    headerLine += fileName;
    headerLine += L"  (";
    headerLine += ToWide(GroupDigits(totalLines), CP_ACP);
    headerLine += L" lines)";

    std::wstring text = headerLine;
    text += L'\n';
    text.append(headerLine.size(), L'=');
    text += L"\n\n";
    text += ToWide(body, codePage);

    buf.reportText  = ToCrLf(text);
    buf.reportIndex = index;

    ShowReportDialog(g_nppData._nppHandle, g_hInstance, title, buf.reportText);
}
