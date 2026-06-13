#include "Parser.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <cstring>
#include <cctype>

// ---------------------------------------------------------------------------
//  Rule counts - computed at compile time from the arrays in LogPatterns.h
// ---------------------------------------------------------------------------
static constexpr int LOG_RULE_COUNT =
    static_cast<int>(sizeof(LOG_TYPE_RULES) / sizeof(LOG_TYPE_RULES[0]));
static constexpr int STEP_RULE_COUNT =
    static_cast<int>(sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]));

// ---------------------------------------------------------------------------
//  ParseDocument
//
//  Uses SCI_GETCHARACTERPOINTER to obtain a direct pointer into Scintilla's
//  internal UTF-8 buffer - no copy, no struct alignment issues.
//  The pointer remains valid until the next text-modifying call; scanning
//  completes before ApplyHighlights touches the document.
// ---------------------------------------------------------------------------
std::vector<Match> ParseDocument(HWND hScintilla)
{
    std::vector<Match> results;

    const intptr_t docLen = static_cast<intptr_t>(
        ::SendMessage(hScintilla, SCI_GETLENGTH, 0, 0));
    if (docLen <= 0) return results;

    const char* const text = reinterpret_cast<const char*>(
        ::SendMessage(hScintilla, SCI_GETCHARACTERPOINTER, 0, 0));
    if (!text) return results;

    const char* const docEnd = text + docLen;

    // -- Log Type: exact keyword match, apply text color to keyword only ------
    for (int i = 0; i < LOG_RULE_COUNT; ++i)
    {
        const char*    kw    = LOG_TYPE_RULES[i].keyword;
        const intptr_t kwLen = static_cast<intptr_t>(std::strlen(kw));
        const char*    search = text;

        while (search < docEnd)
        {
            const char* hit = std::strstr(search, kw);
            if (!hit) break;

            results.push_back({ MatchType::LOG_TYPE, i,
                                 static_cast<intptr_t>(hit - text),
                                 kwLen });
            search = hit + 1;
        }
    }

    // -- Step Type: prefix + one-or-more digits + space ----------------------
    //
    //  Examples (prefix = "Step"):
    //    OK: "Step1 "  "Step12 "  "Step123 "
    //    NG: "Stepname"  "Step1init"  "Step " (no digits)
    //
    //  Highlighted range: from prefix start to end-of-line (excl. newline)
    // -------------------------------------------------------------------------
    for (int i = 0; i < STEP_RULE_COUNT; ++i)
    {
        const char*    prefix    = STEP_TYPE_RULES[i].prefix;
        const intptr_t prefixLen = static_cast<intptr_t>(std::strlen(prefix));
        const char*    search    = text;

        while (search < docEnd)
        {
            const char* hit = std::strstr(search, prefix);
            if (!hit) break;

            // At least one digit must follow the prefix
            const char* p = hit + prefixLen;
            if (p >= docEnd || !std::isdigit(static_cast<unsigned char>(*p)))
            {
                search = hit + 1;
                continue;
            }

            // Consume all consecutive digits
            while (p < docEnd && std::isdigit(static_cast<unsigned char>(*p)))
                ++p;

            // The character after the digits must be a space
            if (p < docEnd && *p != ' ')
            {
                search = hit + 1;
                continue;
            }

            // Find end of line (stop before \r or \n)
            const char* lineEnd = hit;
            while (lineEnd < docEnd && *lineEnd != '\r' && *lineEnd != '\n')
                ++lineEnd;

            results.push_back({ MatchType::STEP_TYPE, i,
                                 static_cast<intptr_t>(hit - text),
                                 static_cast<intptr_t>(lineEnd - hit) });

            search = hit + 1;
        }
    }

    return results;
}
