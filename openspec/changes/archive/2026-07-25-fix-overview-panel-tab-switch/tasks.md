## 1. Fix notification constants in PluginInterface.h

- [x] 1.1 Change `NPPN_FILEBEFORECLOSE` from `NPPN_FIRST + 6` to `NPPN_FIRST + 3` (1003)
- [x] 1.2 Change `NPPN_BUFFERACTIVATED` from `NPPN_FIRST + 13` to `NPPN_FIRST + 10` (1010)

## 2. Fix OverviewPanel::Update() to handle Scintilla HWND changes

- [x] 2.1 In `src/OverviewPanel.cpp`, detect when `hSci != m_hSci` (view switch) and migrate the subclass: `RemoveWindowSubclass` from old, `SetWindowPos(SWP_FRAMECHANGED)` on old, `SetWindowSubclass` on new, `SetWindowPos(SWP_FRAMECHANGED)` on new
- [x] 2.2 Replace the `InvalidateFrame()` call at the end of `Update()` with a direct `RedrawWindow` that includes `RDW_UPDATENOW` for synchronous repaint

## 3. Verify

- [x] 3.1 Build Release x64 — no new warnings beyond pre-existing C4819/C4312/LNK4070
- [ ] 3.2 Parse file A (Ctrl+Alt+Q), switch to unparsed file B: panel clears immediately
- [ ] 3.3 Switch back to file A: panel restores A's marks immediately
- [ ] 3.4 Parse file B (Ctrl+Alt+Q), switch between A and B: each tab shows its own marks
- [ ] 3.5 Dual-view: move file B to other view, switch between views — panel follows active view
- [ ] 3.6 Close a parsed tab: its state is removed, switching to remaining tabs works correctly
