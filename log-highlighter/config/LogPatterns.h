#pragma once
#ifndef LOG_PATTERNS_H
#define LOG_PATTERNS_H

#include <windows.h>

// -----------------------------------------------------------------------------
//  Color helper macro: converts RGB to Scintilla's BGR format.
//  Usage: MAKE_BGR(red, green, blue)
// -----------------------------------------------------------------------------
#define MAKE_BGR(r, g, b) \
    ( ((COLORREF)(b) << 16) | ((COLORREF)(g) << 8) | (COLORREF)(r) )


// -----------------------------------------------------------------------------
//  Log Type Rules
//
//  Match  : exact string match (case-sensitive)
//  Range  : the keyword itself gets the text color applied
//
//  Fields:
//    keyword   - target string (UTF-8)
//    textColor - foreground color, use MAKE_BGR(R, G, B)
// -----------------------------------------------------------------------------
struct LogTypeRule {
    const char* keyword;
    COLORREF    textColor;
};

static const LogTypeRule LOG_TYPE_RULES[] = {
//   keyword        textColor
    { "[ ERROR ]",  MAKE_BGR(220,   0,   0) },  // red
    { "[ WARN ]",   MAKE_BGR(200, 160,   0) },  // golden yellow
    { "[ DEBUG ]",  MAKE_BGR( 70, 150, 255) },  // cornflower blue
};


// -----------------------------------------------------------------------------
//  Step Type Rules
//
//  Match  : prefix + one-or-more digits + space
//    OK : "Step1 "  "Step12 "  "Step123 "
//    NG : "Stepname"  "Step1init"  "Step " (no digits)
//
//  Range  : from the start of prefix to end-of-line (excluding the newline)
//
//  Fields:
//    prefix  - literal text before the digit sequence (UTF-8), e.g. "Step"
//    bgColor - background color, use MAKE_BGR(R, G, B), fully opaque
// -----------------------------------------------------------------------------
struct StepTypeRule {
    const char* prefix;
    COLORREF    bgColor;
};

static const StepTypeRule STEP_TYPE_RULES[] = {
//   prefix   bgColor
    { "Step",  MAKE_BGR(180, 230, 180) },  // light green background
};

#endif // LOG_PATTERNS_H
