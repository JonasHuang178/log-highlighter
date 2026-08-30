#include "DebugConsole.h"

static HANDLE g_conOut     = INVALID_HANDLE_VALUE;
static bool   g_weAllocated = false;

// Swallows Ctrl+C / Ctrl+Break / close so a stray keystroke in the console
// cannot terminate Notepad++. This is a second line of defence only: Windows
// allows a bounded time to respond to CTRL_CLOSE_EVENT and terminates the
// process regardless afterwards, which is why the close box is removed too.
static BOOL WINAPI DebugConsoleCtrlHandler(DWORD type)
{
    switch (type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        return TRUE;   // handled — do not pass to the default handler
    default:
        return FALSE;
    }
}

void OpenDebugConsole()
{
    if (g_conOut != INVALID_HANDLE_VALUE) return;

    // Notepad++ normally has no console. If one is somehow already attached,
    // reuse it rather than failing.
    if (::GetConsoleWindow() == nullptr)
    {
        if (!::AllocConsole()) return;
        g_weAllocated = true;
    }

    // CONOUT$ rather than GetStdHandle: the standard handles of a GUI process
    // may point somewhere else entirely.
    g_conOut = ::CreateFileW(L"CONOUT$",
                             GENERIC_WRITE | GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_EXISTING, 0, nullptr);
    if (g_conOut == INVALID_HANDLE_VALUE)
    {
        if (g_weAllocated) { ::FreeConsole(); g_weAllocated = false; }
        return;
    }

    ::SetConsoleTitleW(L"log-highlighter debug");
    ::SetConsoleCtrlHandler(DebugConsoleCtrlHandler, TRUE);

    // Remove the close box. Without this, closing the console kills Notepad++.
    if (HWND hCon = ::GetConsoleWindow())
    {
        if (HMENU hMenu = ::GetSystemMenu(hCon, FALSE))
        {
            ::DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
            ::DrawMenuBar(hCon);
        }
    }
}

void CloseDebugConsole()
{
    if (g_conOut != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(g_conOut);
        g_conOut = INVALID_HANDLE_VALUE;
    }

    ::SetConsoleCtrlHandler(DebugConsoleCtrlHandler, FALSE);

    if (g_weAllocated)
    {
        ::FreeConsole();
        g_weAllocated = false;
    }
}

bool IsDebugConsoleOpen()
{
    return g_conOut != INVALID_HANDLE_VALUE;
}

void DebugConsoleWrite(const wchar_t* text, size_t len)
{
    if (g_conOut == INVALID_HANDLE_VALUE || !text || len == 0) return;

    DWORD written = 0;
    ::WriteConsoleW(g_conOut, text, static_cast<DWORD>(len), &written, nullptr);
}
