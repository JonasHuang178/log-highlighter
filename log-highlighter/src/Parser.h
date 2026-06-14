#pragma once
#include <windows.h>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
//  MatchType - corresponds to the two rule types in config/LogPatterns.h
// ---------------------------------------------------------------------------
enum class MatchType { LOG_TYPE, STEP_TYPE };

// ---------------------------------------------------------------------------
//  Match - one parse result entry
//
//  type       : rule type (LOG_TYPE or STEP_TYPE)
//  ruleIndex  : index into LOG_TYPE_RULES[] or STEP_TYPE_RULES[]
//  byteOffset : start byte position in the document
//  length     : byte length of the highlighted range
//               LOG_TYPE  -> length of the keyword itself
//               STEP_TYPE -> length from prefix start to end of line
// ---------------------------------------------------------------------------
struct Match {
    MatchType type;
    int       ruleIndex;
    intptr_t  byteOffset;
    intptr_t  length;
};

// progressFn(currentLine, totalLines) is called every 500 lines on the UI thread.
// Return false to cancel; ParseDocument returns an empty vector on cancel.
// Pass nullptr to parse without progress reporting.
std::vector<Match> ParseDocument(
    HWND                                      hScintilla,
    std::function<bool(int, int)>             progressFn = nullptr);
