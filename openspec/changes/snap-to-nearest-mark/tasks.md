## 1. Implement snap-to-nearest-mark in OnNCLButtonDblClk

- [x] 1.1 In `OnNCLButtonDblClk` (OverviewPanel.cpp), after computing `rawLine` from click position, add snap logic: if `m_marks` is non-empty, use `std::min_element` to find the mark with minimum `|mark.line - rawLine|` and set `line` to that mark's line
- [x] 1.2 If `m_marks` is empty, keep existing behavior (use `rawLine` directly)

## 2. Verify and test

- [ ] 2.1 Parse a large log file (10,000+ lines) — click anywhere on the panel → editor jumps to nearest ERROR mark
- [ ] 2.2 Click at the top vs bottom of a dense cluster of marks — verify upper click goes to upper mark, lower click goes to lower mark
- [ ] 2.3 Clear marks (no parse) — click panel → proportional jump still works
- [ ] 2.4 File with a single mark — click anywhere on panel → always lands on that mark's line
