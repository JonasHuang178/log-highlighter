#pragma once

// -----------------------------------------------------------------------------
//  AboutInfo.h - Plugin metadata displayed in the About dialog
//  Edit this file to update the version or description shown to users.
// -----------------------------------------------------------------------------

#define PLUGIN_VERSION   L"v1.0"
#define PLUGIN_NAME      L"log-highlighter"

// Each line is a separate macro so long descriptions stay readable.
// Use L"\n" to add a blank line between sections.
#define ABOUT_TITLE \
    PLUGIN_NAME L" " PLUGIN_VERSION

#define ABOUT_CONTENT \
    L"Plugin : log-highlighter " PLUGIN_VERSION         L"\n" \
    L"Author : (your name)"                             L"\n"
