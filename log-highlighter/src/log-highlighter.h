#pragma once
#include <windows.h>
#include <vector>
#include "Parser.h"

// Configure all indicator styles and colors based on rules in config/LogPatterns.h.
// Called on every SCN_MODIFIED to ensure indicator settings are not lost.
void InitStyles(HWND hScintilla);

// Clear all Log Type and Step Type indicators across the entire document.
void ClearAllHighlights(HWND hScintilla);

// Apply ALL match results to Scintilla (used for SCN_MODIFIED re-highlights).
//   repaintAfter = false -> lets Scintilla repaint itself after a text change
void ApplyHighlights(HWND hScintilla,
                     const std::vector<Match>& matches,
                     bool repaintAfter = true);

// Apply only matches whose byteOffset falls in [fromByte, toByteExclusive).
// 'matches' must be sorted by byteOffset (as returned by ParseDocument).
// Used for lazy/deferred viewport highlighting after Ctrl+Alt+Q.
void ApplyHighlightsInRange(HWND hScintilla,
                             const std::vector<Match>& matches,
                             intptr_t fromByte,
                             intptr_t toByteExclusive,
                             bool repaintAfter = true);
