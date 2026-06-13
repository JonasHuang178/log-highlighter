#pragma once
#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <windows.h>
#include <tchar.h>

// ---------------------------------------------------------------------------
// Minimal Notepad++ Plugin Interface
// Source: https://github.com/notepad-plus-plus/notepad-plus-plus (PowerEditor/src)
// ---------------------------------------------------------------------------

const int MENU_ITEM_MAX_LENGTH = 64;

typedef void (*PFUNCPLUGINCMD)();

struct ShortcutKey {
    bool  _isCtrl;
    bool  _isAlt;
    bool  _isShift;
    UCHAR _key;
};

struct FuncItem {
    TCHAR          _itemName[MENU_ITEM_MAX_LENGTH];
    PFUNCPLUGINCMD _pFunc;
    int            _cmdID;
    bool           _init2Check;
    ShortcutKey*   _pShKey;
};

struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};

// Minimal SCNotification — only fields used in Phase 1
struct SCNotification {
    NMHDR    nmhdr;
    intptr_t position;
    int      ch;
    int      modifiers;
    int      modificationType;
    const char* text;
    intptr_t length;
    intptr_t linesAdded;
    int      message;
    uintptr_t wParam;
    intptr_t  lParam;
    intptr_t  line;
    int      foldLevelNow;
    int      foldLevelPrev;
    int      margin;
    int      listType;
    int      x;
    int      y;
    int      token;
    intptr_t annotationLinesAdded;
    int      updated;
    int      listCompletionMethod;
    intptr_t characterSource;
};

// ---------------------------------------------------------------------------
// Scintilla notification codes (nmhdr.code values in SCNotification)
// ---------------------------------------------------------------------------
enum ScintillaNotification : UINT {
    SCN_STYLENEEDED  = 2000,   // Scintilla needs bytes styled before paint
    SCN_CHARADDED    = 2001,
    SCN_MODIFIED     = 2008,
    SCN_UPDATEUI     = 2007,
};

// SCN_MODIFIED modificationType flags
static constexpr int SC_MOD_INSERTTEXT      = 0x001; // text was inserted
static constexpr int SC_MOD_DELETETEXT      = 0x002; // text was deleted
static constexpr int SC_MOD_CHANGEINDICATOR = 0x4000; // indicator changed (not a text edit)

// ---------------------------------------------------------------------------
// Notepad++ Docking API
// Source: Notepad_plus_msgs.h (Notepad++ official SDK)
// ---------------------------------------------------------------------------

// tTbData.uMask flags
static constexpr UINT DWS_DF_CONT_RIGHT = 0x00000002; // dock to right side
static constexpr UINT DWS_ICONTAB       = 0x00000010; // show icon in tab
static constexpr UINT DWS_ADDINFO       = 0x00000020; // show pszAddInfo

struct tTbData {
    HWND         hClient;       // plugin panel HWND
    const TCHAR* pszName;       // panel title shown in tab
    int          dlgID;         // dialog resource ID (0 if not using resource)
    UINT         uMask;         // combination of DWS_* flags
    HICON        hIconTab;      // icon shown in tab (nullptr = none)
    const TCHAR* pszAddInfo;    // extra info string (nullptr if not used)
    RECT         rcFloat;       // position when floating (ignored when docked)
    int          iPrevCont;     // previous container index (set to -1)
    const TCHAR* pszModuleName; // plugin DLL module name (for persistence)
};

// ---------------------------------------------------------------------------
// Notepad++ messages
// ---------------------------------------------------------------------------
enum NppMsg : UINT {
    NPPMSG                      = WM_USER + 1000,
    NPPM_GETCURRENTSCINTILLA    = NPPMSG + 4,
    NPPM_GETCURRENTLANGTYPE     = NPPMSG + 5,
    NPPM_SETSTATUSBAR           = NPPMSG + 24,
    NPPM_GETPLUGINSCONFIGDIR    = NPPMSG + 46,
    NPPM_MENUCOMMAND            = NPPMSG + 48,
    NPPM_DMMREGASDCKDLG         = NPPMSG + 33, // register a docking panel
    NPPM_GETCURRENTVIEW         = NPPMSG + 98,
};

// ---------------------------------------------------------------------------
// Notepad++ notification codes  (nmhdr.code in WM_NOTIFY to plugin)
// Source: Notepad_plus_msgs.h
// ---------------------------------------------------------------------------
static constexpr UINT NPPN_FIRST = 1000;
static constexpr UINT NPPN_READY = NPPN_FIRST + 1; // NPP fully initialized

// Status bar panel indices
enum StatusBarSection : WPARAM {
    STATUSBAR_DOC_TYPE     = 0,
    STATUSBAR_DOC_SIZE     = 1,
    STATUSBAR_CUR_POS      = 2,
    STATUSBAR_EOF_FORMAT   = 3,
    STATUSBAR_UNICODE_TYPE = 4,
    STATUSBAR_TYPING_MODE  = 5,
};

#endif // PLUGIN_INTERFACE_H
