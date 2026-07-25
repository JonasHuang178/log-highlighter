## 1. Inset the drawing area in DrawPanel()

- [ ] 1.1 Compute `arrowH = GetSystemMetrics(SM_CYVSCROLL)` at the start of `DrawPanel()`
- [ ] 1.2 Define `trackTop = arrowH`, `trackBot = panelH - arrowH`, `trackH = trackBot - trackTop`; clamp `trackH` to at least 1
- [ ] 1.3 Offset all mark y-positions by `trackTop`: `y = trackTop + (line / totalLines * trackH)`
- [ ] 1.4 Offset viewport box by `trackTop`: `boxTop = trackTop + (first / totalLines * trackH)`, same for `boxBot`
- [ ] 1.5 Clamp marks and viewport box to `[trackTop, trackBot]`

## 2. Update click navigation in OnNCLButtonDblClk()

- [ ] 2.1 Compute the same `arrowH` and `trackTop` / `trackH`
- [ ] 2.2 Change `rawLine` calculation: `rawLine = ((clickY - trackTop) / trackH) * totalLines`, clamped to `[0, totalLines - 1]`

## 3. Verify

- [ ] 3.1 Build Release x64 — no new warnings
- [ ] 3.2 Visual: viewport box top edge aligns with the scrollbar track top (below the up-arrow)
- [ ] 3.3 Visual: viewport box bottom edge aligns with the scrollbar track bottom (above the down-arrow)
- [ ] 3.4 Marks appear only within the track-aligned region, not in the arrow zones
- [ ] 3.5 Click in the track-aligned region navigates correctly
- [ ] 3.6 Click in the arrow zone (top/bottom inset) clamps to first/last line
- [ ] 3.7 Panel background covers the full strip with no gaps
