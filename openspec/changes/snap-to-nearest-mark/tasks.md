## 1. Add OVERVIEW_SNAP_RADIUS to OverviewConfig.h

- [x] 1.3 Add `#define OVERVIEW_SNAP_RADIUS 50` to `config/OverviewConfig.h` with a comment explaining the unit (document lines)

## 2. Update snap logic in OnNCLButtonDblClk to respect radius

- [x] 2.0 (done) Compute `rawLine`, use `std::min_element` to find nearest mark
- [x] 2.1 After finding the nearest mark, only snap if `std::abs(it->line - rawLine) <= OVERVIEW_SNAP_RADIUS`; otherwise use `rawLine` directly

## 2. Verify and test

- [ ] 2.1 Parse a large log file (10,000+ lines) — click anywhere on the panel → editor jumps to nearest ERROR mark
- [ ] 2.2 Click at the top vs bottom of a dense cluster of marks — verify upper click goes to upper mark, lower click goes to lower mark
- [ ] 2.3 Clear marks (no parse) — click panel → proportional jump still works
- [ ] 2.4 File with a single mark — click anywhere on panel → always lands on that mark's line
