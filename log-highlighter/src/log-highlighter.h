#pragma once
#include <windows.h>
#include <vector>
#include "Parser.h"

// Configure all indicator styles and colors based on rules in config/LogPatterns.h.
// Called once per ParseLog() before scanning.
void InitStyles(HWND hScintilla);

// Clear all Log Type and Step Type indicators across the entire document.
void ClearAllHighlights(HWND hScintilla);

// Apply all match results to Scintilla, then force an immediate repaint.
void ApplyHighlights(HWND hScintilla,
                     const std::vector<Match>& matches);
