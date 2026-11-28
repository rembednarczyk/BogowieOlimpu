# Interface scale (docs/06-MOBILE-UX.md, decision 2026-11-27): the interface is laid out in dp, 1/160 inch, so a
# 48 dp button is 7.6 mm on every phone and tablet; on a PC the system's display scale decides (a bigger monitor
# shows more of the city, as the reference's fixed-size panel did). The player's slider (75-150 %) multiplies it.
# The window keeps its pixels for the 3D view (canvas_items); only the canvas of the interface is scaled.
class_name UiScale
extends RefCounted

const TAP := 48.0          # dp: the smallest target for a finger (Android, owner's choice)
const FONT := 16           # dp: body text and buttons
const FONT_SMALL := 14     # dp: the panel's categories and the buildings' names (Android's minimum is 12 sp)
const FONT_H2 := 18        # dp: section titles
const FONT_H1 := 20        # dp: panel titles
const GAP := 4.0           # dp between buttons
const MARGIN := 12.0       # dp from the screen's safe edges
const MIN_PERCENT := 75
const MAX_PERCENT := 150
const SETTINGS := "user://settings.cfg"
static var forced_dpi := 0  # the screenshot harness pretends a phone's density (screenshot.gd dpi=)

# Pixels per dp: dpi / 160 on phones and tablets, the system scale on a PC; times the player's percent.
static func factor(dpi: int, system_scale: float, mobile: bool, percent: int) -> float:
	var base := (maxf(dpi, 120.0) / 160.0) if mobile else maxf(system_scale, 1.0)
	return base * clampi(percent, MIN_PERCENT, MAX_PERCENT) / 100.0

static func device_factor(percent: int) -> float:
	var screen := DisplayServer.window_get_current_screen()
	if forced_dpi > 0:
		return factor(forced_dpi, 1.0, true, percent)
	return factor(DisplayServer.screen_get_dpi(screen), DisplayServer.screen_get_scale(screen),
		OS.has_feature("mobile"), percent)

static func saved_percent() -> int:
	var cfg := ConfigFile.new()
	return int(cfg.get_value("ui", "scale", 100)) if cfg.load(SETTINGS) == OK else 100

static func save_percent(percent: int) -> void:
	var cfg := ConfigFile.new()
	cfg.load(SETTINGS)
	cfg.set_value("ui", "scale", clampi(percent, MIN_PERCENT, MAX_PERCENT))
	cfg.save(SETTINGS)

# The interface's canvas: the window's pixels divided by the factor, exactly (no letterbox, no stretch).
static func apply(window: Window, f: float) -> void:
	window.content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	window.content_scale_aspect = Window.CONTENT_SCALE_ASPECT_IGNORE
	window.content_scale_factor = 1.0
	window.content_scale_size = Vector2i(maxi(1, roundi(window.size.x / f)), maxi(1, roundi(window.size.y / f)))

# The edges the system keeps (a camera cut-out, rounded corners), in dp: left, top, right, bottom.
static func safe_insets(window: Window, f: float) -> Vector4:
	var safe := DisplayServer.get_display_safe_area()
	if safe.size.x <= 0 or safe.size.y <= 0:
		return Vector4.ZERO
	var win := Rect2i(window.position, window.size)
	var inner := safe.intersection(win)
	if inner.size.x <= 0:
		return Vector4.ZERO
	return Vector4(inner.position.x - win.position.x, inner.position.y - win.position.y,
		win.end.x - inner.end.x, win.end.y - inner.end.y) / f
