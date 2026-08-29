## MODIFIED Requirements

### Requirement: Viewport indicator box
The panel SHALL draw a filled rectangle with a border indicating the currently visible region, controlled by `OVERVIEW_VIEWPORT_BORDER_VISIBLE` (default: true). When visible: fill color uses `OVERVIEW_VIEWPORT_BG_COLOR` (`CLR_NONE` = `GetSysColor(COLOR_SCROLLBAR)`); border color uses `OVERVIEW_VIEWPORT_COLOR`; border width uses `OVERVIEW_VIEWPORT_BORDER_WIDTH` (default: 1px). When `OVERVIEW_VIEWPORT_BORDER_VISIBLE` is false, neither fill nor border is drawn. All viewport box drawing SHALL be clipped to the scrollbar track region (between the arrow buttons) to prevent thick borders from bleeding outside. The box SHALL update on every `SCN_UPDATEUI` notification.

#### Scenario: Custom border width
- **WHEN** `OVERVIEW_VIEWPORT_BORDER_WIDTH` is set to 3 and the plugin is rebuilt
- **THEN** the viewport box border is drawn with a 3px pen width, clipped to the scrollbar track region

#### Scenario: Custom border color
- **WHEN** `OVERVIEW_VIEWPORT_COLOR` is set to `RGB(0, 255, 0)` and the plugin is rebuilt
- **THEN** the viewport box border is drawn in green

#### Scenario: Viewport box hidden
- **WHEN** `OVERVIEW_VIEWPORT_BORDER_VISIBLE` is set to `false` and the plugin is rebuilt
- **THEN** the viewport indicator box (fill and border) is not drawn; only marks and background are visible
