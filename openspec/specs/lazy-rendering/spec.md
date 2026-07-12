## ADDED Requirements

### Requirement: SCN_UPDATEUI extension
On every `SCN_UPDATEUI` notification, if the **current buffer's** `highlightActive` is `true` and its `matches` is non-empty:
1. Compute `needed = GetLookaheadByteEnd(hSci)`.
2. If `needed > currentBuffer.appliedByteEnd`:
   - Call `ApplyHighlightsInRange(hSci, currentBuffer.matches, currentBuffer.appliedByteEnd + 1, needed + 1, repaintAfter=false)`.
   - Set `currentBuffer.appliedByteEnd = needed`.

The per-buffer `appliedByteEnd` is restored from `g_bufferStates` on `NPPN_BUFFERACTIVATED`, so lazy rendering resumes correctly after a tab switch.

#### Scenario: No extra work when viewport is already covered
- **WHEN** the user moves the cursor within the already-applied region of the current buffer
- **THEN** `needed <= currentBuffer.appliedByteEnd` and `ApplyHighlightsInRange` is not called

#### Scenario: Lazy rendering resumes after tab switch
- **WHEN** the user switches away from a partially-rendered buffer and switches back
- **THEN** `SCN_UPDATEUI` picks up from the correct `appliedByteEnd` for that buffer, not from another buffer's cursor

## REMOVED Requirements

### Requirement: SCN_MODIFIED full apply
**Reason**: Auto re-highlight on text change is removed by design. Ctrl+Alt+Q is the sole parse trigger.
**Migration**: Press Ctrl+Alt+Q to re-parse after editing the document.
