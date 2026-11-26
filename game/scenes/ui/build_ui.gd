# Player interface on the phone (E7 building by touch, E8 UI; docs/06-MOBILE-UX.md). A panel pinned to the
# right edge with the categories and fixed action buttons; the buildings of a category slide out in a column
# next to it. Placeholder look: plain Godot controls; player texts are catalog keys (game/i18n, rule 7).
# The rules stay in the core: previews ask BoSimulation.check_placement, actions are commands.
extends CanvasLayer

const TABS := ["general", "population", "husbandry", "industry", "distribution", "hygiene_safety", "administration",
	"culture", "mythology", "military", "aesthetics"]
const PANEL_W := 300.0
const CELL := Vector2(144, 60)      # at least 44 pt on a 6" phone at the 720 px base height
const ITEM := Vector2(210, 60)
const ITEMS_PER_COLUMN := 9
const TAP_SLOP := 14.0
# How a building is laid by dragging (owner's choice 2026-11-26): houses and clearing as a rectangle, roads
# and walls as a line; everything else one at a time.
const RECT_TYPES := ["common_housing", "elite_housing", "clear_land"]
const LINE_TYPES := ["road", "wall"]
const SETTINGS := "user://settings.cfg"

var game: Node3D  # boot.gd: sim, screen_to_tile(), show_ghost(), clear_ghost(), ticks_per_second
var buildings_by_tab := {}
var mode := ""        # "", "build", "demolish"
var selected := ""
var turn := 0
var drag_from := Vector2i(-1, -1)
var target := Vector2i(-1, -1)
var press_pos := Vector2.ZERO
var pressed := false
var moved := false
var instant := false  # build when the finger lifts, without Confirm (setting)
var preview: Array = []  # [{"x", "y"}] origins the confirm will act on

var mode_group := ButtonGroup.new()
var flyout: PanelContainer
var flyout_grid: GridContainer
var confirm_btn: Button
var rotate_btn: Button
var cancel_btn: Button
var demolish_btn: Button
var hint: Label
var status: Label
var card: PanelContainer
var card_text: Label

func setup(g: Node3D) -> void:
	game = g
	TranslationServer.set_locale("en" if OS.get_locale_language() == "en" else "pl")  # Polish by default
	var cfg := ConfigFile.new()
	if cfg.load(SETTINGS) == OK:
		instant = cfg.get_value("build", "instant", false)
	_load_catalog()
	_build()
	_set_mode("")

func _load_catalog() -> void:
	var list = JSON.parse_string(FileAccess.get_file_as_string("res://data/buildings.json"))
	for tab in TABS:
		buildings_by_tab[tab] = []
	for b in list:
		var tab: String = b.get("tab", "")
		if not buildings_by_tab.has(tab) or not b.get("player_placeable", true) or b.get("culture", "any") == "atlantean":
			continue
		if b.has("per_god"):
			continue  # the generic sanctuary is only the base of the per-god ones
		buildings_by_tab[tab].append(b["id"])

func _button(key: String, cb: Callable, size: Vector2, toggle := false) -> Button:
	var b := Button.new()
	b.text = key  # a catalog key: the button translates itself and follows a change of language
	b.custom_minimum_size = size
	b.add_theme_font_size_override("font_size", 18)
	b.clip_text = true
	if toggle:
		b.toggle_mode = true
		b.button_group = mode_group
	b.pressed.connect(cb)
	return b

func _build() -> void:
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	status = Label.new()
	status.position = Vector2(16, 10)
	status.add_theme_font_size_override("font_size", 18)
	root.add_child(status)
	hint = Label.new()
	hint.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	hint.position = Vector2(16, -44)
	hint.add_theme_font_size_override("font_size", 20)
	root.add_child(hint)

	# The panel pinned to the right edge: categories, then the fixed action buttons.
	var panel := PanelContainer.new()
	panel.set_anchors_preset(Control.PRESET_RIGHT_WIDE)
	panel.offset_left = -PANEL_W
	root.add_child(panel)
	var col := VBoxContainer.new()
	panel.add_child(col)
	var cats := GridContainer.new()
	cats.columns = 2
	for tab in TABS:
		cats.add_child(_button("tab." + tab, func(): _show_tab(tab), CELL, true))
	demolish_btn = _button("ui.demolish", func(): _set_mode("demolish"), CELL, true)
	cats.add_child(demolish_btn)
	col.add_child(cats)
	var spacer := Control.new()
	spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	col.add_child(spacer)
	var acts := HBoxContainer.new()
	confirm_btn = _button("ui.confirm", _confirm, Vector2(96, CELL.y))
	rotate_btn = _button("ui.rotate", func(): turn = (turn + 1) % 4; _refresh_preview(), Vector2(96, CELL.y))
	cancel_btn = _button("ui.cancel", cancel, Vector2(96, CELL.y))
	for b in [confirm_btn, rotate_btn, cancel_btn]:
		acts.add_child(b)
	col.add_child(acts)
	var speeds := HBoxContainer.new()
	speeds.add_child(_button("ui.pause", func(): _speed(0.0), Vector2(96, CELL.y)))
	for s in [1.0, 2.0, 4.0]:
		speeds.add_child(_button("%dx" % int(s), func(): _speed(s), Vector2(64, CELL.y)))
	col.add_child(speeds)
	var last := HBoxContainer.new()
	last.add_child(_button("ui.undo", func(): game.sim.undo(), Vector2(96, CELL.y)))
	var inst := CheckButton.new()
	inst.text = "ui.instant_build"
	inst.button_pressed = instant
	inst.add_theme_font_size_override("font_size", 16)
	inst.custom_minimum_size = Vector2(196, CELL.y)
	inst.toggled.connect(_set_instant)
	last.add_child(inst)
	col.add_child(last)

	# The column of buildings that slides out next to the panel.
	flyout = PanelContainer.new()
	flyout.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	root.add_child(flyout)
	flyout_grid = GridContainer.new()
	flyout.add_child(flyout_grid)
	flyout.visible = false

	card = PanelContainer.new()
	card.position = Vector2(16, 60)
	card.custom_minimum_size = Vector2(420, 0)
	var card_box := VBoxContainer.new()
	card_text = Label.new()
	card_text.add_theme_font_size_override("font_size", 18)
	card_box.add_child(card_text)
	card_box.add_child(_button("ui.close", func(): card.visible = false, Vector2(140, CELL.y)))
	card.add_child(card_box)
	card.visible = false
	root.add_child(card)

# Cancel: no mode, no category chosen, the column closed.
func cancel() -> void:
	for b in mode_group.get_buttons():
		b.set_pressed_no_signal(false)
	flyout.visible = false
	_set_mode("")

func _set_instant(on: bool) -> void:
	instant = on
	var cfg := ConfigFile.new()
	cfg.load(SETTINGS)
	cfg.set_value("build", "instant", on)
	cfg.save(SETTINGS)

func _show_tab(tab: String) -> void:
	_set_mode("")
	for c in flyout_grid.get_children():
		c.queue_free()
	var ids: Array = buildings_by_tab[tab]
	flyout_grid.columns = maxi(1, ceili(float(ids.size()) / ITEMS_PER_COLUMN))
	for id in ids:
		flyout_grid.add_child(_button("building." + id, func(): _select(id), ITEM))
	flyout.offset_right = -PANEL_W
	flyout.offset_left = -PANEL_W - flyout_grid.columns * (ITEM.x + 4) - 8
	flyout.offset_top = 0
	flyout.visible = true
	cancel_btn.disabled = false

func _select(id: String) -> void:
	selected = id
	turn = 0
	flyout.visible = false
	_set_mode("build")

func _set_mode(m: String) -> void:
	mode = m
	drag_from = Vector2i(-1, -1)
	target = Vector2i(-1, -1)
	preview = []
	game.clear_ghost()
	if m == "demolish":
		selected = ""
		flyout.visible = false  # demolishing clears any other choice
	confirm_btn.disabled = true
	rotate_btn.disabled = m != "build" or drag_kind() != ""
	cancel_btn.disabled = m == "" and not flyout.visible
	match m:
		"build":
			hint.text = tr("building." + selected) + ": " + tr("ui.drag_area" if drag_kind() != "" else "ui.tap_to_place")
		"demolish":
			hint.text = tr("ui.tap_to_demolish")
		_:
			hint.text = ""

# "rect", "line" or "" for the current choice.
func drag_kind() -> String:
	if mode == "demolish" or (mode == "build" and selected in RECT_TYPES):
		return "rect"
	if mode == "build" and selected in LINE_TYPES:
		return "line"
	return ""

func _speed(s: float) -> void:
	game.ticks_per_second = 60.0 * s

func update_status() -> void:
	status.text = tr("ui.status") % [game.sim.get_year() + 1, game.sim.get_month() + 1, game.sim.get_treasury(),
		game.sim.get_population(), game.sim.get_employed(), game.sim.get_workforce(), game.sim.get_popularity()]

# Map input from boot.gd (one finger, or the mouse); true when the UI used it, otherwise the camera pans.
func map_input(event: InputEvent) -> bool:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			pressed = true
			moved = false
			press_pos = event.position
			if drag_kind() != "":
				begin_drag(game.screen_to_tile(event.position))
				return true
			return false
		pressed = false
		if drag_kind() != "":
			end_drag()
			return true
		if not moved:
			tap(game.screen_to_tile(event.position))
			return true
		return false
	if event is InputEventMouseMotion and pressed:
		if event.position.distance_to(press_pos) > TAP_SLOP:
			moved = true
		if drag_kind() != "":
			drag_to(game.screen_to_tile(event.position))
			return true
	return false

func begin_drag(tile: Vector2i) -> void:
	drag_from = tile
	target = tile
	_refresh_preview()

func drag_to(tile: Vector2i) -> void:
	if tile.x >= 0:
		target = tile
		_refresh_preview()

func end_drag() -> void:
	if instant:
		_confirm()

# A tap: place the ghost of a single building, or open the card of a building with no mode.
func tap(tile: Vector2i) -> void:
	if mode == "":
		_open_card(tile)
		return
	begin_drag(tile)
	end_drag()

func _line_tiles() -> Array:
	var out := []
	if drag_from.x < 0:
		return out
	var xs := 1 if target.x >= drag_from.x else -1
	for x in range(drag_from.x, target.x + xs, xs):
		out.append(Vector2i(x, drag_from.y))
	var ys := 1 if target.y >= drag_from.y else -1
	for y in range(drag_from.y + ys, target.y + ys, ys):
		out.append(Vector2i(target.x, y))
	return out

func _rect_tiles(step: int) -> Array:
	var out := []
	if drag_from.x < 0:
		return out
	var x0 := mini(drag_from.x, target.x)
	var y0 := mini(drag_from.y, target.y)
	for y in range(y0, maxi(drag_from.y, target.y) + 1, step):
		for x in range(x0, maxi(drag_from.x, target.x) + 1, step):
			out.append(Vector2i(x, y))
	return out

func _refresh_preview() -> void:
	preview = []
	if target.x < 0:
		return
	var good := PackedVector2Array()
	var bad := PackedVector2Array()
	var cost := 0
	if mode == "demolish" or selected == "clear_land" or selected == "road" or selected == "roadblock":
		var tiles := _rect_tiles(1) if drag_kind() == "rect" else (_line_tiles() if drag_kind() == "line" else [target])
		for t in tiles:
			preview.append(t)
			(bad if mode == "demolish" else good).append(Vector2(t.x, t.y))
	else:
		var step := 2 if selected in ["common_housing", "elite_housing"] else 1  # 2x2 plots (S-004)
		var origins := _rect_tiles(step) if drag_kind() == "rect" else (_line_tiles() if drag_kind() == "line" else [target])
		for o in origins:
			var r: Dictionary = game.sim.check_placement(selected, o.x, o.y, turn)
			if r["ok"]:
				preview.append(o)
				cost += int(r["cost"])
				good.append_array(r["tiles"])
			else:
				bad.append_array(r["tiles"])
				if origins.size() == 1:
					hint.text = tr("building." + selected) + ": " + tr(r["reason"])
		if not preview.is_empty() or origins.size() > 1:
			hint.text = tr("building." + selected) + ": %d   " % preview.size() + tr("ui.cost") % cost
	game.show_ghost(good, bad)
	confirm_btn.disabled = preview.is_empty()

func _confirm() -> void:
	if preview.is_empty():
		return
	var done := {}
	for t in preview:
		if mode == "demolish":
			var id: int = game.sim.get_tile(t.x, t.y)["building"]
			var road: bool = game.sim.get_tile(t.x, t.y)["road"]
			if (id != 0 and done.has(id)) or (id == 0 and not road):
				continue
			done[id] = true
			game.sim.demolish(t.x, t.y)
		elif selected == "road":
			game.sim.place_road(t.x, t.y)
		elif selected == "roadblock":
			game.sim.place_roadblock(t.x, t.y)
		elif selected == "clear_land":
			game.sim.clear_land(t.x, t.y)
		else:
			game.sim.place_building(selected, t.x, t.y, turn)
	preview = []
	drag_from = Vector2i(-1, -1)
	target = Vector2i(-1, -1)
	game.clear_ghost()
	confirm_btn.disabled = true  # stay in the mode: the next drag or tap builds more

func _open_card(tile: Vector2i) -> void:
	if tile.x < 0:
		return
	var id: int = game.sim.get_tile(tile.x, tile.y)["building"]
	if id == 0:
		card.visible = false
		return
	for b in game.sim.get_buildings():
		if b["id"] != id:
			continue
		var lines := [tr("building." + b["type"])]
		if b["workers"] > 0 or b["staffing"] < 100:
			lines.append(tr("ui.workers") % [b["workers"], b["staffing"]])
		if b.has("population"):
			lines.append(tr("ui.residents") % b["population"])
			lines.append(tr("ui.level") % (b["house_level"] + 1))
		if b.has("stock_loads") and not b["stock_loads"].is_empty():
			lines.append(tr("ui.stock") % str(b["stock_loads"]))
		lines.append(tr("ui.fire_risk") % b.get("fire_risk", 0))
		lines.append(tr("ui.collapse_risk") % b.get("damage_risk", 0))
		card_text.text = "\n".join(lines)
		card.visible = true
		return
