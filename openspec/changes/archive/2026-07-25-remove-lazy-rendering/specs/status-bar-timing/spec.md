## MODIFIED Requirements

### Requirement: Timing scope
`t0` is captured immediately after `InitStyles(hSci)` — before the progress dialog is created and before any parse work. `t1` is captured after the Overview Panel `Update()` call completes. The elapsed time thus covers:

1. Parser scan (`ParseDocument` → `ScanBuffer`), including the document snapshot
2. `ClearAllHighlights`
3. `ApplyHighlights` for all matches in the document
4. `g_overviewPanel.Update` (building panel marks + NCA redraw)

#### Scenario: Timing covers the full apply phase
- **WHEN** a parse produces 64 000 matches and Phase 2 takes most of the wall time
- **THEN** the reported duration includes that apply time, because `t1` is taken after `ApplyHighlights` and the panel update
