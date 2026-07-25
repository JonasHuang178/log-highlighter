## 1. Add OVERVIEW_SNAP_RADIUS to OverviewConfig.h

- [x] 1.3 Add `#define OVERVIEW_SNAP_RADIUS 50` to `config/OverviewConfig.h` with a comment explaining the unit (document lines)

## 2. Update snap logic in OnNCLButtonDblClk to respect radius

- [x] 2.0 (done) Compute `rawLine`, use `std::min_element` to find nearest mark
- [x] 2.1 After finding the nearest mark, only snap if `std::abs(it->line - rawLine) <= OVERVIEW_SNAP_RADIUS`; otherwise use `rawLine` directly

