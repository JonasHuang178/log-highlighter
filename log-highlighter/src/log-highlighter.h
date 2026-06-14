#pragma once
#include <windows.h>
#include <vector>
#include "Parser.h"

// Configure all indicator styles and colors based on rules in config/LogPatterns.h.
// Called on every SCN_MODIFIED to ensure indicator settings are not lost.
void InitStyles(HWND hScintilla);

// Clear all Log Type and Step Type indicators across the entire document.
void ClearAllHighlights(HWND hScintilla);

// Apply the results returned by ParseDocument() to Scintilla.
//   repaintAfter = true  -> triggered by Ctrl+Alt+Q; calls WM_SETREDRAW + InvalidateRect
//   repaintAfter = false -> triggered by SCN_MODIFIED; lets Scintilla repaint itself
void ApplyHighlights(HWND hScintilla,
                     const std::vector<Match>& matches,
                     bool repaintAfter = true);
