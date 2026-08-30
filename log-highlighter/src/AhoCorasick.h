#pragma once
#include <vector>
#include <queue>
#include <algorithm>

// ---------------------------------------------------------------------------
//  AhoCorasick - multi-pattern automaton, header-only.
//
//  Scans a buffer in a single O(N) pass regardless of how many patterns were
//  added. Patterns are identified by an integer index chosen by the caller;
//  this file has no knowledge of what a pattern means.
//
//  Deliberately free of <windows.h> and Scintilla so it can be included from
//  ReportApi.h, which must stay clear of platform types.
//
//  Usage:
//      AhoCorasick ac;
//      ac.addPattern("[ ERROR ]", 0);
//      ac.addPattern("[ WARN ]",  1);
//      ac.build();
//
//      int state = 0;
//      for (const char* p = text; p < end; ++p) {
//          state = ac.step(state, *p);
//          for (const auto& out : ac.outputs(state)) { ... }
//      }
// ---------------------------------------------------------------------------
struct AhoCorasick
{
    // One pattern ending at a given state. `len` is the pattern's byte length,
    // so the match starts at (p - len + 1).
    struct Output { int patternIndex; int len; };

    struct State
    {
        int next[256];
        int fail = 0;
        std::vector<Output> outputs;  // patterns that end at this state
        State() { std::fill(next, next + 256, -1); }
    };

    std::vector<State> s;

    AhoCorasick() { s.emplace_back(); }   // state 0 = root

    // Adds a NUL-terminated pattern. Empty patterns are ignored.
    void addPattern(const char* pat, int patternIndex)
    {
        if (!pat || !*pat) return;

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
        s[cur].outputs.push_back({ patternIndex, len });
    }

    // Adds a pattern given as an explicit byte range. Empty patterns are ignored.
    void addPattern(const char* pat, size_t len, int patternIndex)
    {
        if (!pat || len == 0) return;

        int cur = 0;
        for (size_t i = 0; i < len; ++i)
        {
            unsigned char c = static_cast<unsigned char>(pat[i]);
            if (s[cur].next[c] == -1)
            {
                s[cur].next[c] = static_cast<int>(s.size());
                s.emplace_back();
            }
            cur = s[cur].next[c];
        }
        s[cur].outputs.push_back({ patternIndex, static_cast<int>(len) });
    }

    void build()
    {
        // BFS to set failure links and complete the goto function so every
        // state has a valid transition for every character (no -1 entries).
        std::queue<int> q;
        for (int c = 0; c < 256; ++c)
        {
            if (s[0].next[c] == -1)
                s[0].next[c] = 0;          // undefined -> loop to root
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

    // Advances one character. Valid only after build().
    int step(int state, char ch) const
    {
        return s[state].next[static_cast<unsigned char>(ch)];
    }

    const std::vector<Output>& outputs(int state) const
    {
        return s[state].outputs;
    }
};
