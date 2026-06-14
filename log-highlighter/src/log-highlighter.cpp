#include "log-highlighter.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"

// ---------------------------------------------------------------------------
//  Indicator index layout
//
//  Notepad++ reserves index 8 (Smart Highlight) and uses 0-10 for built-ins.
//  Starting at 11 avoids all known conflicts.
//
//   11 .. (11 + LOG_RULE_COUNT  - 1) -> Log Type  (INDIC_TEXTFORE)
//   14 .. (14 + STEP_RULE_COUNT - 1) -> Step Type (INDIC_FULLBOXBLOCK)
//
//  Default LogPatterns.h (3 Log rules, 1 Step rule):
//   11 = [ ERROR ]   12 = [ WARN ]   13 = [ DEBUG ]   14 = Step
// ---------------------------------------------------------------------------
static constexpr int LOG_RULE_COUNT =
    static_cast<int>(sizeof(LOG_TYPE_RULES) / sizeof(LOG_TYPE_RULES[0]));
static constexpr int STEP_RULE_COUNT =
    static_cast<int>(sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]));

static constexpr int INDIC_LOG_BASE  = 11;
static constexpr int INDIC_STEP_BASE = INDIC_LOG_BASE + LOG_RULE_COUNT;  // = 14

static inline LRESULT Sci(HWND h, UINT msg, WPARAM wp = 0, LPARAM lp = 0)
{
    return ::SendMessage(h, msg, wp, lp);
}

// ---------------------------------------------------------------------------
void InitStyles(HWND hSci)
{
    // Log Type: INDIC_TEXTFORE - changes text foreground color only
    for (int i = 0; i < LOG_RULE_COUNT; ++i)
    {
        const int idx = INDIC_LOG_BASE + i;
        Sci(hSci, SCI_INDICSETSTYLE, idx, INDIC_TEXTFORE);
        Sci(hSci, SCI_INDICSETFORE,  idx,
            static_cast<LPARAM>(LOG_TYPE_RULES[i].textColor));
    }

    // Step Type: INDIC_FULLBOXBLOCK - fills the background behind the text
    //   SCI_INDICSETUNDER(TRUE) -> render below the text so text remains visible
    //   SCI_INDICSETALPHA(255)  -> fully opaque
    for (int i = 0; i < STEP_RULE_COUNT; ++i)
    {
        const int idx = INDIC_STEP_BASE + i;
        Sci(hSci, SCI_INDICSETSTYLE, idx, INDIC_FULLBOXBLOCK);
        Sci(hSci, SCI_INDICSETFORE,  idx,
            static_cast<LPARAM>(STEP_TYPE_RULES[i].bgColor));
        Sci(hSci, SCI_INDICSETALPHA, idx, 255);   // fully opaque
        Sci(hSci, SCI_INDICSETUNDER, idx, TRUE);  // render under text layer
    }
}

// ---------------------------------------------------------------------------
void ClearAllHighlights(HWND hSci)
{
    const intptr_t docLen = static_cast<intptr_t>(Sci(hSci, SCI_GETLENGTH));
    if (docLen <= 0) return;

    for (int i = 0; i < LOG_RULE_COUNT; ++i)
    {
        Sci(hSci, SCI_SETINDICATORCURRENT, INDIC_LOG_BASE + i);
        Sci(hSci, SCI_INDICATORCLEARRANGE, 0, static_cast<LPARAM>(docLen));
    }
    for (int i = 0; i < STEP_RULE_COUNT; ++i)
    {
        Sci(hSci, SCI_SETINDICATORCURRENT, INDIC_STEP_BASE + i);
        Sci(hSci, SCI_INDICATORCLEARRANGE, 0, static_cast<LPARAM>(docLen));
    }
}

// ---------------------------------------------------------------------------
void ApplyHighlights(HWND hSci,
                     const std::vector<Match>& matches,
                     bool repaintAfter)
{
    if (matches.empty()) return;

    for (const auto& m : matches)
    {
        const int idx = (m.type == MatchType::LOG_TYPE)
                      ? INDIC_LOG_BASE  + m.ruleIndex
                      : INDIC_STEP_BASE + m.ruleIndex;

        Sci(hSci, SCI_SETINDICATORCURRENT, idx);
        Sci(hSci, SCI_INDICATORFILLRANGE,
            static_cast<WPARAM>(m.byteOffset),
            static_cast<LPARAM>(m.length));
    }

    if (repaintAfter)
        ::RedrawWindow(hSci, nullptr, nullptr,
                       RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}
