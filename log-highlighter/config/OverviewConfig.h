#pragma once
#ifndef OVERVIEW_CONFIG_H
#define OVERVIEW_CONFIG_H

#include <windows.h>

// -----------------------------------------------------------------------------
//  Overview Panel configuration
//  Edit this file and rebuild to customize the panel appearance.
// -----------------------------------------------------------------------------

// Width of the Overview Panel in pixels
#define OVERVIEW_PANEL_WIDTH     14

// Minimum height of a colored mark in pixels (prevents marks from disappearing
// on very large documents)
#define OVERVIEW_MARK_MIN_H       1

// Background color of the overview panel
#define OVERVIEW_BG_COLOR        RGB(60, 60, 60)

// Border color of the viewport indicator box
#define OVERVIEW_VIEWPORT_COLOR     RGB(130, 130, 130)

// Fill (background) color of the viewport indicator box.
// CLR_NONE (0xFFFFFFFF) = use the system scrollbar track color at runtime
// (matches the scrollbar to the left of the panel).
// Override with any RGB() value to use a custom color, e.g. RGB(80, 80, 100).
#define OVERVIEW_VIEWPORT_BG_COLOR  CLR_NONE

#endif // OVERVIEW_CONFIG_H
