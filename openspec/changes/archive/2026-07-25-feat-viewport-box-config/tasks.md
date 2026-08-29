## 1. Add constants to OverviewConfig.h

- [x] 1.1 Add `#define OVERVIEW_VIEWPORT_BORDER_WIDTH 1` with a comment explaining it controls the pen width of the viewport box border
- [x] 1.2 Add `#define OVERVIEW_VIEWPORT_BORDER_VISIBLE true` with a comment explaining it controls whether the viewport indicator box (fill + border) is drawn
- [x] 1.3 Update the comment on `OVERVIEW_VIEWPORT_COLOR` to clarify it is the viewport box border color

## 2. Use the constants in DrawPanel()

- [x] 2.1 In `src/OverviewPanel.cpp`, change `CreatePen(PS_SOLID, 1, OVERVIEW_VIEWPORT_COLOR)` to use `OVERVIEW_VIEWPORT_BORDER_WIDTH` instead of the hard-coded `1`
- [x] 2.2 Wrap the entire viewport box drawing section (clip setup, fill, and border) inside `if (OVERVIEW_VIEWPORT_BORDER_VISIBLE)`, keeping `m_visibleLines` computation outside
- [x] 2.3 Add `SaveDC`/`IntersectClipRect`/`RestoreDC` to clip the viewport box drawing to the scrollbar track region (between arrow buttons)

## 3. Verify

- [ ] 3.1 Build Release x64 — no new warnings
- [ ] 3.2 Default appearance unchanged (border width 1, visible, same color)
- [ ] 3.3 Change `OVERVIEW_VIEWPORT_BORDER_WIDTH` to 3, rebuild — border visibly thicker
- [ ] 3.4 Change `OVERVIEW_VIEWPORT_COLOR` to `RGB(0,255,0)`, rebuild — border turns green
- [ ] 3.5 Set `OVERVIEW_VIEWPORT_BORDER_VISIBLE` to `false`, rebuild — viewport box disappears entirely
- [ ] 3.6 With large border width (e.g. 20), verify border is clipped to scrollbar track region
