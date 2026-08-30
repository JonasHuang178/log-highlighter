#include "Parser.h"
#include "AhoCorasick.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <cstring>
#include <cctype>
#include <vector>

static constexpr int LOG_RULE_COUNT =
    static_cast<int>(sizeof(LOG_TYPE_RULES) / sizeof(LOG_TYPE_RULES[0]));
static constexpr int STEP_RULE_COUNT =
    static_cast<int>(sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]));
static constexpr int BOOKMARK_RULE_COUNT =
    static_cast<int>(sizeof(BOOKMARK_RULES) / sizeof(BOOKMARK_RULES[0]));

// ---------------------------------------------------------------------------
//  Pattern index layout inside the shared automaton.
//
//  The three rule tables are laid out back to back, each base derived from the
//  previous table's size, so adding a rule shifts the later ranges
//  automatically — the same scheme log-highlighter.cpp uses for indicators.
// ---------------------------------------------------------------------------
static constexpr int LOG_PATTERN_BASE      = 0;
static constexpr int STEP_PATTERN_BASE     = LOG_PATTERN_BASE  + LOG_RULE_COUNT;
static constexpr int BOOKMARK_PATTERN_BASE = STEP_PATTERN_BASE + STEP_RULE_COUNT;

// Maps a pattern index back to the rule table it came from.
static void DecodePattern(int patternIndex, MatchType& type, int& ruleIndex)
{
    if (patternIndex < STEP_PATTERN_BASE)
    {
        type      = MatchType::LOG_TYPE;
        ruleIndex = patternIndex - LOG_PATTERN_BASE;
    }
    else if (patternIndex < BOOKMARK_PATTERN_BASE)
    {
        type      = MatchType::STEP_TYPE;
        ruleIndex = patternIndex - STEP_PATTERN_BASE;
    }
    else
    {
        type      = MatchType::BOOKMARK;
        ruleIndex = patternIndex - BOOKMARK_PATTERN_BASE;
    }
}

// Build the automaton once from the compile-time pattern tables.
static const AhoCorasick& getAC()
{
    static AhoCorasick ac = []() {
        AhoCorasick a;
        for (int i = 0; i < LOG_RULE_COUNT;  ++i)
            a.addPattern(LOG_TYPE_RULES [i].keyword, LOG_PATTERN_BASE      + i);
        for (int i = 0; i < STEP_RULE_COUNT; ++i)
            a.addPattern(STEP_TYPE_RULES[i].prefix,  STEP_PATTERN_BASE     + i);
        for (int i = 0; i < BOOKMARK_RULE_COUNT; ++i)
            a.addPattern(BOOKMARK_RULES [i].keyword, BOOKMARK_PATTERN_BASE + i);
        a.build();
        return a;
    }();
    return ac;
}

// ---------------------------------------------------------------------------
//  ScanBuffer — single-pass scan using Aho-Corasick.
//  Safe to call from any thread.
// ---------------------------------------------------------------------------
static std::vector<Match> ScanBuffer(const char*                    text,
                                      size_t                         len,
                                      int                            totalLines,
                                      std::function<bool(int, int)>  progressFn)
{
    std::vector<Match> results;
    if (!text || len == 0) return results;

    const AhoCorasick& ac  = getAC();
    const char* const  end = text + len;

    int  acState  = 0;
    int  lineNo   = 0;

    for (const char* p = text; p < end; ++p)
    {
        const unsigned char c = static_cast<unsigned char>(*p);

        // Count completed lines for progress (newline = end of a line)
        if (c == '\n')
        {
            ++lineNo;
            if (progressFn && (lineNo % 500 == 0 || lineNo == totalLines))
                if (!progressFn(lineNo, totalLines)) return {};
        }

        acState = ac.step(acState, *p);

        const auto& outs = ac.outputs(acState);
        if (outs.empty()) continue;

        for (const auto& out : outs)
        {
            MatchType type;
            int       ruleIndex;
            DecodePattern(out.patternIndex, type, ruleIndex);

            // AC reports a match ending at p; matchStart is p - len + 1.
            const char* matchStart = p - out.len + 1;

            if (type == MatchType::LOG_TYPE || type == MatchType::BOOKMARK)
            {
                results.push_back({ type, ruleIndex,
                                     static_cast<intptr_t>(matchStart - text),
                                     static_cast<intptr_t>(out.len) });
            }
            else // STEP_TYPE — prefix matched; validate digit(s) + space/EOL
            {
                const char* q = p + 1;  // first char after prefix
                if (q >= end || !std::isdigit(static_cast<unsigned char>(*q)))
                    continue;
                while (q < end && std::isdigit(static_cast<unsigned char>(*q)))
                    ++q;
                if (q < end && *q != ' ' && *q != '\r' && *q != '\n')
                    continue;
                // Extend match to end-of-line content
                const char* lineEnd = q;
                while (lineEnd < end && *lineEnd != '\r' && *lineEnd != '\n')
                    ++lineEnd;
                results.push_back({ MatchType::STEP_TYPE, ruleIndex,
                                     static_cast<intptr_t>(matchStart - text),
                                     static_cast<intptr_t>(lineEnd - matchStart) });
            }
        }
    }

    // Final progress tick if file doesn't end with a newline
    if (progressFn && lineNo < totalLines)
        progressFn(totalLines, totalLines);

    return results;
}

// ---------------------------------------------------------------------------
//  SnapshotDocument — copy the document out of Scintilla.
// ---------------------------------------------------------------------------
std::vector<char> SnapshotDocument(HWND hScintilla)
{
    const intptr_t docLen = static_cast<intptr_t>(
        ::SendMessage(hScintilla, SCI_GETLENGTH, 0, 0));
    if (docLen <= 0) return {};

    const char* raw = reinterpret_cast<const char*>(
        ::SendMessage(hScintilla, SCI_GETCHARACTERPOINTER, 0, 0));
    if (!raw) return {};

    return std::vector<char>(raw, raw + docLen);
}

// ---------------------------------------------------------------------------
//  ParseDocument — snapshot + scan on the UI thread.
// ---------------------------------------------------------------------------
std::vector<Match> ParseDocument(HWND                          hScintilla,
                                  std::function<bool(int, int)> progressFn)
{
    std::vector<char> localBuf = SnapshotDocument(hScintilla);
    if (localBuf.empty()) return {};

    const int totalLines = static_cast<int>(
        ::SendMessage(hScintilla, SCI_GETLINECOUNT, 0, 0));

    return ScanBuffer(localBuf.data(), localBuf.size(),
                      totalLines, std::move(progressFn));
}
