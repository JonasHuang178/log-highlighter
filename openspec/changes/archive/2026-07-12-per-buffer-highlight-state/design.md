## Context

Currently `Plugin.cpp` holds parse results in three globals:

```cpp
static std::vector<Match> g_matches;
static intptr_t           g_appliedByteEnd = -1;
static bool               g_highlightActive = false;
```

Because they are global, (1) `SCN_MODIFIED` auto-re-parses every file once `g_highlightActive` is true, and (2) switching tabs corrupts the Overview Panel — it shows the previously-parsed buffer's marks, not the current one's.

Scintilla indicators are already stored per-buffer by NPP internally. Visual highlights survive tab switches for free. Only the in-memory state (match list, lazy cursor, active flag) needs to be per-buffer.

## Goals / Non-Goals

**Goals:**
- Parse results (matches, lazy byte cursor, active flag) stored per NPP buffer ID
- Tab switch restores Overview Panel and lazy-rendering cursor to the correct buffer state instantly
- `SCN_MODIFIED` no longer triggers any parse or re-highlight
- Ctrl+Alt+Q remains the sole parse trigger

**Non-Goals:**
- Persisting parse results across NPP restarts (session state)
- Auto-parsing on file open or language change
- Supporting multiple Scintilla views (secondary pane)

## Decisions

### 1. Data structure: `std::unordered_map<LRESULT, BufferState>`

```cpp
struct BufferState {
    std::vector<Match> matches;
    intptr_t           appliedByteEnd = -1;
    bool               highlightActive = false;
};
static std::unordered_map<LRESULT, BufferState> g_bufferStates;
```

Key = NPP buffer ID obtained via `NPPM_GETCURRENTBUFFERID`.

**Why map over alternatives:**
- Buffer ID (LRESULT/UINT_PTR) is stable for the lifetime of an open document
- `unordered_map` gives O(1) lookup on tab switch
- Simpler than tracking NPP's buffer list manually

### 2. Active buffer helper

```cpp
static BufferState& CurrentBuffer() {
    LRESULT id = ::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
    return g_bufferStates[id];  // default-constructs if new
}
```

All code that previously referenced `g_matches`, `g_appliedByteEnd`, `g_highlightActive` now calls `CurrentBuffer().field`.

### 3. `NPPN_BUFFERACTIVATED` — tab switch handler

```cpp
case NPPN_BUFFERACTIVATED: {
    LRESULT id = static_cast<LRESULT>(notification->nmhdr.idFrom);
    auto it = g_bufferStates.find(id);
    if (it != g_bufferStates.end() && it->second.highlightActive) {
        // Restore Overview Panel for this buffer
        g_overviewPanel.Update(GetCurrentScintilla(),
                               BuildPanelMarks(GetCurrentScintilla(), it->second.matches));
    } else {
        // Buffer not parsed — clear the panel
        if (g_overviewPanel.IsInitialized())
            g_overviewPanel.Update(GetCurrentScintilla(), {});
    }
    break;
}
```

No re-parse, no ClearAllHighlights — Scintilla already holds the correct indicator state for the buffer.

### 4. Remove `SCN_MODIFIED` re-parse block entirely

The entire `SCN_MODIFIED` case in `beNotified` is removed. Real-time re-highlight on typing is no longer supported (by design — user wants Ctrl+Alt+Q only).

### 5. Buffer cleanup on file close

`NPPN_FILEBEFORECLOSE` removes the buffer's entry from the map to avoid unbounded growth:

```cpp
case NPPN_FILEBEFORECLOSE: {
    LRESULT id = static_cast<LRESULT>(notification->nmhdr.idFrom);
    g_bufferStates.erase(id);
    break;
}
```

## Risks / Trade-offs

- **Real-time re-highlight removed**: Users who relied on auto-highlight-as-you-type lose that feature. Acceptable per requirement — Ctrl+Alt+Q is intentional.
- **Map grows with open tabs**: Bounded by the number of open NPP buffers. Each entry holds a `vector<Match>` — for a 64k-match file ~3MB. With 10 such files open: ~30MB. Acceptable.
- **`NPPN_BUFFERACTIVATED` idFrom**: NPP docs confirm `idFrom` carries the buffer ID for this notification. Value verified against NPP plugin examples.

## Open Questions

None — requirements are clear and implementation is straightforward.
