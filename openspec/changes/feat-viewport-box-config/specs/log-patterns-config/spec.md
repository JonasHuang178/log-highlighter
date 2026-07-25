## MODIFIED Requirements

### Requirement: Rebuild-only customization
Adding, removing, or editing rules SHALL require only editing `config/LogPatterns.h` and rebuilding. Overview Panel appearance SHALL require only editing `config/OverviewConfig.h` and rebuilding. No other source files need modification.

`OverviewConfig.h` SHALL include:
- `OVERVIEW_PANEL_WIDTH` — panel strip width in pixels
- `OVERVIEW_MARK_MIN_H` — minimum mark height in pixels
- `OVERVIEW_SNAP_RADIUS` — click snap radius in document lines
- `OVERVIEW_VIEWPORT_COLOR` — viewport box border color
- `OVERVIEW_VIEWPORT_BG_COLOR` — viewport box fill color
- `OVERVIEW_VIEWPORT_BORDER_WIDTH` — viewport box border pen width (default: 1)
- `OVERVIEW_VIEWPORT_BORDER_VISIBLE` — viewport box visibility toggle (default: true); when false, both fill and border are hidden
