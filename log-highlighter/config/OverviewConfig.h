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
#define OVERVIEW_MARK_MIN_H       3

// Color of the viewport indicator box (the rectangle showing the visible region)
// Use standard Windows RGB() macro
#define OVERVIEW_VIEWPORT_COLOR  RGB(200, 200, 200)

#endif // OVERVIEW_CONFIG_H
