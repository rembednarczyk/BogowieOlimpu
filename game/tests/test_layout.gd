# Layout gate (docs/06-MOBILE-UX.md, decision 2026-11-27): on every screen we support the interface's buttons are
# at least 48 dp, lie on the screen and do not overlap, in each state of the interface. Sizes in dp, landscape:
# the smallest phone, a 6.1" phone, the reference Xiaomi 15T Pro, 1280x720, an 11" tablet, a 1080p PC.
# Run: godot --headless --path game --script res://tests/test_layout.gd
extends "res://tests/check.gd"

const SIZES := {"min 640x360": Vector2i(640, 360), "6.1in 892x401": Vector2i(892, 401),
	"xiaomi 993x458": Vector2i(993, 458), "1280x720": Vector2i(1280, 720),
	"tablet 1410x1057": Vector2i(1410, 1057), "pc 1920x1080": Vector2i(1920, 1080)}
const EPS := 0.5

var game: Node3D

func _init() -> void:
	await process_frame
	# The factor (ui_scale.gd): pixels per dp on a phone, the system scale on a PC, the slider's 75-150 %.
	check(absf(UiScale.factor(447, 1.0, true, 100) - 2.79) < 0.01, "Xiaomi 15T Pro: 447 dpi is 2.79 px a dp")
	check(UiScale.factor(96, 1.0, false, 100) == 1.0, "a PC at the system scale 1: one pixel a dp")
	check(UiScale.factor(96, 2.0, false, 150) == 3.0, "a PC at the system scale 2 and the slider at 150 %")
	check(UiScale.factor(447, 1.0, true, 300) == UiScale.factor(447, 1.0, true, 150), "the slider stops at 150 %")
	check(UiScale.factor(447, 1.0, true, 10) == UiScale.factor(447, 1.0, true, 75), "and at 75 %")
	root.content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	root.content_scale_aspect = Window.CONTENT_SCALE_ASPECT_IGNORE
	for name in SIZES:
		root.content_scale_size = SIZES[name]
		game = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
		game.use_save = false
		root.add_child(game)
		game.ticks_per_second = 0.0
		TranslationServer.set_locale("pl")  # the longer texts
		await _states(name)
		game.queue_free()
		await process_frame
	finish("LAYOUT TESTS PASSED")

func _states(size_name: String) -> void:
	var ui = game.ui
	await _settle()
	_check(size_name, "start")
	for tab in ui.tabs:
		ui._show_tab(tab)
		await _settle()
		_check(size_name, "column " + tab)
	ui._select("common_housing")
	await _settle()
	_check(size_name, "building")
	ui.cancel()
	game.set_overlay("water")
	ui._open_card(Vector2i(41, 1))  # the Palace of the demo
	await _settle()
	_check(size_name, "card and legend")
	game.set_overlay("")
	ui.toggle_report()
	await _settle()
	_check(size_name, "report")
	ui._close_overlays()
	ui.army_world.toggle_army()
	await _settle()
	_check(size_name, "army")
	ui.army_world.toggle_world()
	await _settle()
	_check(size_name, "world")
	ui.army_world.toggle_world()

func _settle() -> void:
	for i in 3:
		await process_frame

# Every visible button or slider: 48 dp at least, on the screen (within its scroll area), and no two overlap.
func _check(size_name: String, state: String) -> void:
	var screen: Rect2 = game.ui.root.get_global_rect()
	var items := []
	_collect(game.ui.root, items)
	var shown := []
	for c in items:
		var r: Rect2 = c.get_global_rect()
		var what := "%s, %s: %s" % [size_name, state, _name(c)]
		check(r.size.y >= UiScale.TAP - EPS and r.size.x >= UiScale.TAP - EPS, "%s is %s, under 48 dp" % [what, r.size])
		var need := _text_width(c)
		check(need <= r.size.x, "%s: its text needs %.0f dp of %.0f, cut off" % [what, need, r.size.x])
		var v := _visible_part(c, r)
		if v.size.x <= EPS or v.size.y <= EPS:
			continue  # scrolled out of its area
		check(screen.grow(EPS).encloses(v), "%s at %s leaves the screen %s" % [what, v, screen])
		shown.append([c, v, what])
	for i in shown.size():
		for j in range(i + 1, shown.size()):
			var a: Rect2 = shown[i][1]
			var b: Rect2 = shown[j][1]
			var o := a.intersection(b)
			if o.size.x > 1.0 and o.size.y > 1.0 and not shown[i][0].is_ancestor_of(shown[j][0]) \
					and not shown[j][0].is_ancestor_of(shown[i][0]):
				check(false, "%s overlaps %s" % [shown[i][2], _name(shown[j][0])])

func _collect(node: Node, out: Array) -> void:
	for c in node.get_children():
		if c is CanvasItem and not c.visible:
			continue
		if c is BaseButton or c is Range and not c is ScrollBar:
			out.append(c)
			continue  # an OptionButton's own children are its popup
		_collect(c, out)

# The part of a control its scroll areas show.
func _visible_part(c: Control, r: Rect2) -> Rect2:
	var p := c.get_parent()
	while p != null and p != game.ui.root:
		if p is ScrollContainer:
			r = r.intersection((p as Control).get_global_rect())
		p = p.get_parent()
	return r

# The width a button's text needs in its font, with the style's margins and an OptionButton's arrow.
func _text_width(c: Control) -> float:
	if not c is Button or (c as Button).text == "":
		return 0.0
	var b := c as Button
	var font := b.get_theme_font("font")
	var fs := b.get_theme_font_size("font_size")
	var style := b.get_theme_stylebox("normal")
	var sides := style.get_margin(SIDE_LEFT) + style.get_margin(SIDE_RIGHT)
	if b.autowrap_mode != TextServer.AUTOWRAP_OFF:  # wrapped: the lines must fit the height
		var room := b.size.x - sides
		var h := font.get_multiline_string_size(tr(b.text), HORIZONTAL_ALIGNMENT_LEFT, room, fs).y
		var free := b.size.y - style.get_margin(SIDE_TOP) - style.get_margin(SIDE_BOTTOM)
		return b.size.x if h <= free + 1.0 else b.size.x + 100.0
	var w := font.get_string_size(tr(b.text), HORIZONTAL_ALIGNMENT_LEFT, -1, fs).x + sides
	if b is OptionButton:
		w += b.get_theme_icon("arrow").get_width() + b.get_theme_constant("arrow_margin")
	if b is CheckButton:
		w += b.get_theme_icon("checked").get_width() + b.get_theme_constant("h_separation")
	return w

func _name(c: Control) -> String:
	if c is Button and (c as Button).text != "":
		return "%s '%s'" % [c.get_class(), tr((c as Button).text)]
	return c.get_class()
