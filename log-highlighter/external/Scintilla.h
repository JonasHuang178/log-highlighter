#pragma once
#ifndef SCINTILLA_H
#define SCINTILLA_H

// ---------------------------------------------------------------------------
// Minimal Scintilla API for LogHighlighter — 64-bit positions
// Full header: https://github.com/notepad-plus-plus/notepad-plus-plus
//              scintilla/include/Scintilla.h
// ---------------------------------------------------------------------------

#include <stdint.h>

typedef intptr_t Sci_Position;
typedef intptr_t Sci_PositionCR;

struct Sci_CharacterRange {
    Sci_PositionCR cpMin;
    Sci_PositionCR cpMax;
};

struct Sci_TextRange {
    Sci_CharacterRange chrg;
    char*              lpstrText;
};

// ---------------------------------------------------------------------------
// Document
// ---------------------------------------------------------------------------
#define SCI_GETLENGTH               2006
#define SCI_GETTEXTRANGE            2162
#define SCI_GETCHARACTERPOINTER     2520  // returns const char* to raw UTF-8 buffer

// ---------------------------------------------------------------------------
// Lines / navigation  (used by OverviewPanel)
// ---------------------------------------------------------------------------
#define SCI_GETLINECOUNT            2154  // → total line count in document
#define SCI_LINEFROMPOSITION        2166  // wParam=bytePos → line number (0-based)
#define SCI_POSITIONFROMLINE        2167  // wParam=line → byte offset at start of line
#define SCI_GETLINEENDPOSITION      2136  // wParam=line → byte offset at end of line (excl. newline)
#define SCI_GETFIRSTVISIBLELINE     2152  // → first visible line number
#define SCI_LINESONSCREEN           2197  // → number of lines visible in view
#define SCI_GOTOLINE                2024  // wParam=line → move caret to line start
#define SCI_SCROLLCARET             2169  // ensure caret is visible (centers if needed)
#define SCI_SETFIRSTVISIBLELINE     2613  // wParam=line → scroll so this line is first visible
#define SCI_LINESCROLL              2300  // wParam=columns, lParam=lines → scroll relative amount
#define SCI_SETSELECTIONSTART       2142  // wParam=pos → set anchor (does not scroll)
#define SCI_SETSELECTIONEND         2143  // wParam=pos → set caret  (does not scroll)
#define SCI_SETEMPTYSELECTION       2556  // wParam=pos → set anchor=caret=pos, no selection, no scroll

// ---------------------------------------------------------------------------
// Style definitions
// ---------------------------------------------------------------------------
#define STYLE_DEFAULT               32    // Scintilla built-in default style

#define SCI_STYLECLEARALL           2050  // copy STYLE_DEFAULT to all styles
#define SCI_STYLESETFORE            2051
#define SCI_STYLESETBACK            2052
#define SCI_STYLESETBOLD            2053
#define SCI_STYLESETSIZE            2055
#define SCI_STYLESETFONT            2056
#define SCI_STYLEGETFORE            2481
#define SCI_STYLEGETBACK            2482
#define SCI_STYLEGETBOLD            2483
#define SCI_STYLEGETSIZE            2484
#define SCI_STYLEGETFONT            2486

// ---------------------------------------------------------------------------
// Style application
// ---------------------------------------------------------------------------
#define SCI_STARTSTYLING            2032
#define SCI_SETSTYLING              2033

// ---------------------------------------------------------------------------
// Indicators (kept for reference, not used in current implementation)
// ---------------------------------------------------------------------------
#define SCI_INDICSETSTYLE           2080
#define SCI_INDICSETFORE            2082
#define SCI_INDICSETALPHA           2523
#define SCI_INDICSETUNDER           2510
#define SCI_SETINDICATORCURRENT     2500
#define SCI_INDICATORFILLRANGE      2504
#define SCI_INDICATORCLEARRANGE     2505

// Indicator styles
#define INDIC_FULLBOXBLOCK          16
#define INDIC_TEXTFORE              17    // changes text foreground only

#endif // SCINTILLA_H
