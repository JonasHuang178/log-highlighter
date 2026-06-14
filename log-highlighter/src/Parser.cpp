#include "Parser.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <cstring>
#include <cctype>
#include <vector>

static constexpr int LOG_RULE_COUNT =
    static_cast<int>(sizeof(LOG_TYPE_RULES) / sizeof(LOG_TYPE_RULES[0]));
static constexpr int STEP_RULE_COUNT =
    static_cast<int>(sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]));

// Bounded substring search within [hay, hayEnd).
static const char* find_range(const char* hay, const char* hayEnd,
                               const char* needle, size_t needleLen)
{
    if (needleLen == 0) return hay;
    if (hayEnd - hay < static_cast<ptrdiff_t>(needleLen)) return nullptr;
    const char* last = hayEnd - needleLen;
    for (; hay <= last; ++hay)
        if (memcmp(hay, needle, needleLen) == 0) return hay;
    return nullptr;
}

// ---------------------------------------------------------------------------
//  ParseDocument
//
//  Scans the document line-by-line and returns all matches.
//
//  progressFn(currentLine, totalLines) is called every 500 lines.
//  Return false from progressFn to cancel; an empty vector is returned.
//
//  The Scintilla buffer is copied before scanning so PeekMessage calls
//  inside progressFn cannot invalidate the pointer via document edits.
// ---------------------------------------------------------------------------
std::vector<Match> ParseDocument(HWND                          hScintilla,
                                  std::function<bool(int, int)> progressFn)
{
    std::vector<Match> results;

    const intptr_t docLen = static_cast<intptr_t>(
        ::SendMessage(hScintilla, SCI_GETLENGTH, 0, 0));
    if (docLen <= 0) return results;

    const char* raw = reinterpret_cast<const char*>(
        ::SendMessage(hScintilla, SCI_GETCHARACTERPOINTER, 0, 0));
    if (!raw) return results;

    // Local copy — safe even if Scintilla's buffer is reallocated during
    // the PeekMessage loop inside progressFn.
    std::vector<char> localBuf(raw, raw + docLen);
    const char* const text   = localBuf.data();
    const char* const docEnd = text + docLen;

    const int totalLines = static_cast<int>(
        ::SendMessage(hScintilla, SCI_GETLINECOUNT, 0, 0));

    // Pre-compute keyword lengths once.
    size_t logKwLen [LOG_RULE_COUNT];
    size_t stepPfLen[STEP_RULE_COUNT];
    for (int i = 0; i < LOG_RULE_COUNT;  ++i) logKwLen [i] = strlen(LOG_TYPE_RULES [i].keyword);
    for (int i = 0; i < STEP_RULE_COUNT; ++i) stepPfLen[i] = strlen(STEP_TYPE_RULES[i].prefix);

    int         lineNo    = 0;
    const char* lineStart = text;

    while (lineStart < docEnd)
    {
        // Locate end of line content (before \r or \n).
        const char* lineEnd = lineStart;
        while (lineEnd < docEnd && *lineEnd != '\r' && *lineEnd != '\n')
            ++lineEnd;

        // --- Log Type: all keyword occurrences on this line ---
        for (int i = 0; i < LOG_RULE_COUNT; ++i)
        {
            const char* kw    = LOG_TYPE_RULES[i].keyword;
            const size_t kwLen = logKwLen[i];
            const char* p      = lineStart;

            while (true)
            {
                const char* hit = find_range(p, lineEnd, kw, kwLen);
                if (!hit) break;
                results.push_back({ MatchType::LOG_TYPE, i,
                                     static_cast<intptr_t>(hit - text),
                                     static_cast<intptr_t>(kwLen) });
                p = hit + 1;
            }
        }

        // --- Step Type: prefix + one-or-more digits + (space or end-of-line) ---
        for (int i = 0; i < STEP_RULE_COUNT; ++i)
        {
            const char*  prefix    = STEP_TYPE_RULES[i].prefix;
            const size_t prefixLen = stepPfLen[i];
            const char*  p         = lineStart;

            while (true)
            {
                const char* hit = find_range(p, lineEnd, prefix, prefixLen);
                if (!hit) break;

                const char* q = hit + prefixLen;
                if (q >= lineEnd || !std::isdigit(static_cast<unsigned char>(*q)))
                {
                    p = hit + 1;
                    continue;
                }
                while (q < lineEnd && std::isdigit(static_cast<unsigned char>(*q)))
                    ++q;
                if (q < lineEnd && *q != ' ')
                {
                    p = hit + 1;
                    continue;
                }

                // Match covers from prefix start to end of line content.
                results.push_back({ MatchType::STEP_TYPE, i,
                                     static_cast<intptr_t>(hit  - text),
                                     static_cast<intptr_t>(lineEnd - hit) });
                p = hit + 1;
            }
        }

        ++lineNo;

        // Progress callback every 500 lines and on the final line.
        if (progressFn && (lineNo % 500 == 0 || lineNo == totalLines))
        {
            if (!progressFn(lineNo, totalLines))
                return {};  // cancelled
        }

        // Advance past the newline sequence (\r, \n, or \r\n).
        lineStart = lineEnd;
        if (lineStart < docEnd && *lineStart == '\r') ++lineStart;
        if (lineStart < docEnd && *lineStart == '\n') ++lineStart;
    }

    return results;
}
