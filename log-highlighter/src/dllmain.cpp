#include <windows.h>
#include "DebugConsole.h"

// Global HINSTANCE stored for use in OverviewPanel::Init()
HINSTANCE g_hInstance = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   ul_reason_for_call,
                      LPVOID  /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;
        break;
    case DLL_PROCESS_DETACH:
        // Releasing the console handle is safe here. Allocating one would not
        // be — AllocConsole runs under the loader lock, which is why the
        // console is created from the NPPN_READY handler instead.
        CloseDebugConsole();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
