## Why

The viewport indicator box (the colored rectangle showing the currently visible region in the Overview Panel) has its border color and width hard-coded in `OverviewPanel.cpp`. Users who want to change its appearance — for example, to make it wider for better visibility, or to change the color to match their editor theme — must edit the source code and understand which constant does what.

`OverviewConfig.h` already hosts other panel appearance constants (`OVERVIEW_PANEL_WIDTH`, `OVERVIEW_VIEWPORT_COLOR`, `OVERVIEW_VIEWPORT_BG_COLOR`), but two aspects are missing:

1. **Viewport box border width** — currently hard-coded as `CreatePen(PS_SOLID, 1, ...)`. There is no constant to control it; the border is always 1px.
2. **Viewport box border color** — `OVERVIEW_VIEWPORT_COLOR` already exists (`RGB(130,130,130)`) but its name and comment don't clearly convey that it controls the border of the viewport indicator box. This proposal keeps the existing constant and improves its documentation.
3. **Viewport box visibility toggle** — there is no way to hide the viewport indicator box entirely. Users who prefer a clean mark-only panel have no option to do so.

## What Changes

- Add `OVERVIEW_VIEWPORT_BORDER_WIDTH` to `config/OverviewConfig.h` (default: 1), controlling the pen width used to draw the viewport box border
- Add `OVERVIEW_VIEWPORT_BORDER_VISIBLE` to `config/OverviewConfig.h` (default: true), controlling whether the viewport indicator box (fill + border) is drawn
- Update the comment on the existing `OVERVIEW_VIEWPORT_COLOR` to clarify it controls the viewport box border color
- `DrawPanel()` reads all three constants when drawing the viewport box; clips drawing to the scrollbar track region to prevent thick borders from bleeding into arrow-button zones

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `log-patterns-config`: `OverviewConfig.h` gains two new constants (`OVERVIEW_VIEWPORT_BORDER_WIDTH`, `OVERVIEW_VIEWPORT_BORDER_VISIBLE`)
- `overview-panel`: Viewport box uses configurable width, visibility toggle, and clip region from `OverviewConfig.h`

## Impact

- `config/OverviewConfig.h` — add `OVERVIEW_VIEWPORT_BORDER_WIDTH` (default 1) and `OVERVIEW_VIEWPORT_BORDER_VISIBLE` (default true); improve comment on `OVERVIEW_VIEWPORT_COLOR`
- `src/OverviewPanel.cpp` — `DrawPanel()`: use `OVERVIEW_VIEWPORT_BORDER_WIDTH` in `CreatePen` call; skip entire viewport box drawing when `OVERVIEW_VIEWPORT_BORDER_VISIBLE` is false; clip viewport box to scrollbar track region
- No changes to `OverviewPanel.h` or `Plugin.cpp`
- Default behavior unchanged (border width 1, visible, same color) — only affects users who edit `OverviewConfig.h`
