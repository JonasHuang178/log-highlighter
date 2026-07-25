## REMOVED Requirements

### Requirement: SCN_UPDATEUI extension
**Reason**: Never implemented. `Plugin.cpp`'s `SCN_UPDATEUI` handler only refreshes the Overview Panel viewport box; it never calls `ApplyHighlightsInRange`, and the required `GetLookaheadByteEnd` helper does not exist. The performance problem lazy rendering was meant to solve is handled instead by masking `SC_MOD_CHANGEINDICATOR` during bulk fill in `ApplyHighlights`, so deferring work is no longer needed.

**Migration**: None. `ParseLog()` already applies all highlights for the whole document in Phase 2 of the progress dialog, so behavior is unchanged. The `lazy-rendering` capability file is deleted from `openspec/specs/`.

### Requirement: SCN_MODIFIED full apply
**Reason**: Already removed by the `per-buffer-highlight-state` change; carried in this capability's `## REMOVED Requirements` section only as a migration note. The note is dropped along with the capability.

**Migration**: Press Ctrl+Alt+Q to re-parse after editing the document.
