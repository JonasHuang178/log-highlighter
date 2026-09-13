#pragma once
#include <windows.h>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
//  MatchType - corresponds to the three rule types in config/LogPatterns.h
// ---------------------------------------------------------------------------
enum class MatchType { LOG_TYPE, STEP_TYPE, BOOKMARK };

// ---------------------------------------------------------------------------
//  Match - one parse result entry
//
//  type       : rule type (LOG_TYPE, STEP_TYPE or BOOKMARK)
//  ruleIndex  : index into the rule table named by `type`
//  byteOffset : start byte position in the document
//  length     : byte length of the highlighted range
//               LOG_TYPE  -> length of the keyword itself
//               BOOKMARK  -> length of the keyword itself
//               STEP_TYPE -> length from prefix start to end of line
// ---------------------------------------------------------------------------
struct Match {
    MatchType type;
    int       ruleIndex;
    intptr_t  byteOffset;
    intptr_t  length;
};

// Copy the entire document out of a Scintilla window into a local buffer.
// Must be called on the UI thread. Returns an empty vector for an empty document.
//
// Everything that scans the document goes through this first, so a scan stays
// valid even if the caller pumps messages and the user edits the text while a
// progress callback is running.
std::vector<char> SnapshotDocument(HWND hScintilla);

// Scan the document in a Scintilla window (must be called on the UI thread).
// The document is snapshotted into a local buffer before scanning, so the scan
// stays valid even if progressFn pumps messages and the user edits the text.
std::vector<Match> ParseDocument(
    HWND                                      hScintilla,
    std::function<bool(int, int)>             progressFn = nullptr);
