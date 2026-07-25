## Context

The Overview Panel's viewport indicator box is drawn in `DrawPanel()` with two visual components:

1. **Fill** — `OVERVIEW_VIEWPORT_BG_COLOR` (`CLR_NONE` = system scrollbar color), drawn via `FillRect`
2. **Border** — `OVERVIEW_VIEWPORT_COLOR` (`RGB(130,130,130)`), drawn via `CreatePen(PS_SOLID, 1, ...)` + `Rectangle`

The fill color and background color are already configurable in `OverviewConfig.h`. The border width is hard-coded to 1px in the `CreatePen` call, and there is no way to hide the viewport box entirely. This change adds constants for the border width and visibility toggle, adds a clip region to prevent thick borders from bleeding into scrollbar arrow zones, and clarifies the existing color constant's purpose.

## Goals / Non-Goals

**Goals:**
- Viewport box border width is configurable via `OVERVIEW_VIEWPORT_BORDER_WIDTH` in `OverviewConfig.h`
- Viewport box visibility is togglable via `OVERVIEW_VIEWPORT_BORDER_VISIBLE` in `OverviewConfig.h`
- Thick borders are clipped to the scrollbar track region (between arrow buttons)
- Existing `OVERVIEW_VIEWPORT_COLOR` constant gets a clearer comment
- Default appearance is unchanged (width 1, visible, color `RGB(130,130,130)`)
- Rebuild-only customization pattern is maintained (edit header, rebuild)

**Non-Goals:**
- Runtime configuration (INI file, settings dialog) — the project uses compile-time constants throughout
- Adding new color constants — `OVERVIEW_VIEWPORT_COLOR` (border) and `OVERVIEW_VIEWPORT_BG_COLOR` (fill) already cover both parts of the viewport box
- Changing the default values

## Decisions

### Decision: Single constant for border width, not separate top/side/bottom

The border is drawn with a single `Rectangle()` call using one pen. GDI pens have uniform width — there's no way to have different widths per side without drawing four separate lines. A single `OVERVIEW_VIEWPORT_BORDER_WIDTH` matches the GDI API naturally.

### Decision: Keep `OVERVIEW_VIEWPORT_COLOR` name unchanged

Renaming would break any user who has already customized it. The name is adequate — only the comment needs clarification.

### Decision: Place in `OverviewConfig.h` next to existing viewport constants

Consistent with `OVERVIEW_VIEWPORT_COLOR` and `OVERVIEW_VIEWPORT_BG_COLOR` — all viewport box appearance constants are grouped together.

### Decision: `OVERVIEW_VIEWPORT_BORDER_VISIBLE` controls the entire viewport box (fill + border)

When set to `false`, both the fill rectangle and the border are skipped. The name uses "BORDER" for consistency with the other viewport border constants, but the toggle controls the complete viewport indicator. The `m_visibleLines` cache is still computed so `DoNavigation()` keeps working.

### Decision: Clip viewport box to scrollbar track region

GDI `Rectangle()` centers its pen on the edge, so a thick border (e.g. 20px) extends half the pen width outside the box bounds, bleeding into the scrollbar arrow-button zones. A `SaveDC`/`IntersectClipRect`/`RestoreDC` sequence constrains all viewport drawing to the track region between the arrow buttons.

## Risks / Trade-offs

- [Risk] Large border widths (e.g. 5+) on a 14px-wide panel would look odd. Accepted — users who edit the config are expected to use reasonable values; no runtime validation needed for a compile-time constant.
- [Risk] The visibility toggle name (`OVERVIEW_VIEWPORT_BORDER_VISIBLE`) might imply only the border is hidden. Accepted — the config comment clarifies it controls the entire viewport indicator box.
