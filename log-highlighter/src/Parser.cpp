#include "Parser.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <cstring>
#include <cctype>
#include <vector>
#include <queue>

static constexpr int LOG_RULE_COUNT =
    static_cast<int>(sizeof(LOG_TYPE_RULES) / sizeof(LOG_TYPE_RULES[0]));
static constexpr int STEP_RULE_COUNT =
    static_cast<int>(sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]));

// ---------------------------------------------------------------------------
//  Aho-Corasick multi-pattern automaton
//  Scans the document in a single pass regardless of the number of patterns.
// ---------------------------------------------------------------------------
struct AhoCorasick
{
    struct Output { int ruleIndex; MatchType type; int len; };

    struct State
    {
        int next[256];
        int fail = 0;
        std::vector<Output> outputs;  // patterns that end at this state
        State() { std::fill(next, next + 256, -1); }
    };

    std::vector<State> s;

    AhoCorasick() { s.emplace_back(); } // state 0 = root

    void addPattern(const char* pat, int ruleIndex, MatchType type)
    {
        int cur = 0;
        int len = 0;
        for (const char* p = pat; *p; ++p, ++len)
        {
            unsigned char c = static_cast<unsigned char>(*p);
            if (s[cur].next[c] == -1)
            {
                s[cur].next[c] = static_cast<int>(s.size());
                s.emplace_back();
            }
            cur = s[cur].next[c];
        }
        s[cur].outputs.push_back({ ruleIndex, type, len });
    }

    void build()
    {
        // BFS to set failure links and complete the goto function so every
        // state has a valid transition for every character (no -1 entries).
        std::queue<int> q;
        for (int c = 0; c < 256; ++c)
        {
            if (s[0].next[c] == -1)
                s[0].next[c] = 0;          // undefined → loop to root
            else
            {
                s[s[0].next[c]].fail = 0;
                q.push(s[0].next[c]);
            }
        }
        while (!q.empty())
        {
            int u = q.front(); q.pop();
            // Inherit outputs from failure state (suffix matches)
            for (const auto& out : s[s[u].fail].outputs)
                s[u].outputs.push_back(out);

            for (int c = 0; c < 256; ++c)
            {
                if (s[u].next[c] == -1)
                    s[u].next[c] = s[s[u].fail].next[c]; // follow fail link
                else
                {
                    s[s[u].next[c]].fail = s[s[u].fail].next[c];
                    q.push(s[u].next[c]);
                }
            }
        }
    }
};

// Build the automaton once from the compile-time pattern tables.
static const AhoCorasick& getAC()
{
    static AhoCorasick ac = []() {
        AhoCorasick a;
        for (int i = 0; i < LOG_RULE_COUNT;  ++i)
            a.addPattern(LOG_TYPE_RULES [i].keyword, i, MatchType::LOG_TYPE);
        for (int i = 0; i < STEP_RULE_COUNT; ++i)
            a.addPattern(STEP_TYPE_RULES[i].prefix,  i, MatchType::STEP_TYPE);
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

        acState = ac.s[acState].next[c];

        if (ac.s[acState].outputs.empty()) continue;

        for (const auto& out : ac.s[acState].outputs)
        {
            // AC reports a match ending at p; matchStart is p - len + 1.
            const char* matchStart = p - out.len + 1;

            if (out.type == MatchType::LOG_TYPE)
            {
                results.push_back({ MatchType::LOG_TYPE, out.ruleIndex,
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
                results.push_back({ MatchType::STEP_TYPE, out.ruleIndex,
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
//  ParseDocument — snapshot + scan on the UI thread.
// ---------------------------------------------------------------------------
std::vector<Match> ParseDocument(HWND                          hScintilla,
                                  std::function<bool(int, int)> progressFn)
{
    const intptr_t docLen = static_cast<intptr_t>(
        ::SendMessage(hScintilla, SCI_GETLENGTH, 0, 0));
    if (docLen <= 0) return {};

    const char* raw = reinterpret_cast<const char*>(
        ::SendMessage(hScintilla, SCI_GETCHARACTERPOINTER, 0, 0));
    if (!raw) return {};

    std::vector<char> localBuf(raw, raw + docLen);

    const int totalLines = static_cast<int>(
        ::SendMessage(hScintilla, SCI_GETLINECOUNT, 0, 0));

    return ScanBuffer(localBuf.data(), static_cast<size_t>(docLen),
                      totalLines, std::move(progressFn));
}

// ---------------------------------------------------------------------------
//  ParseDocument — scan a pre-snapshotted buffer (worker-thread safe).
// ---------------------------------------------------------------------------
std::vector<Match> ParseDocument(const std::vector<char>&       docBuf,
                                  int                            totalLines,
                                  std::function<bool(int, int)>  progressFn)
{
    return ScanBuffer(docBuf.data(), docBuf.size(),
                      totalLines, std::move(progressFn));
}
