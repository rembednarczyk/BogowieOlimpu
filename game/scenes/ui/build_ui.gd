# Player interface on the phone (E7 building by touch, E8 UI; docs/06-MOBILE-UX.md). Placeholder look: plain
# Godot controls; every player-facing text comes from the language catalogs (game/i18n, rule 7).
# The game rules stay in the core: the ghost asks BoSimulation.check_placement, actions are commands.
extends CanvasLayer

const TABS := ["population", "husbandry", "industry", "distribution", "hygiene_safety", "administration",
	"culture", "mythology", "military", "aesthetics"]
const BUTTON := Vector2(120, 64)  # at least 44 pt on a 6" phone at the 720 px base height (06-MOBILE-UX)
const TAP_SLOP := 14.0            # px a finger may move and still count as a tap
const SPEEDS := [0.0, 1.0, 2.0, 4.0]

var game: Node3D  # boot.gd: sim, screen_to_tile(), show_ghost(), clear_ghost(), ticks_per_second
var buildings_by_tab := {}
var mode := ""        # "", "build", "road", "demolish"
var selected := ""
var turn := 0
var target := Vector2i(-1, -1)
var road_from := Vector2i(-1, -1)
var press_pos := Vector2.ZERO
var pressed := false
var moved := false

var items_box: HBoxContainer
var actions: HBoxContainer
var hint: Label
var status: Label
var card: PanelContainer
var card_text: Label

func setup(g: Node3D) -> void:
	game = g
	# The phone's language when we have it, Polish otherwise (project fallback).
	TranslationServer.set_locale("en" if OS.get_locale_language() == "en" else "pl")
	_load_catalog()
	_build()
	_set_mode("")

func _load_catalog() -> void:
	var text := FileAccess.get_file_as_string("res://data/buildings.json")
	var list = JSON.parse_string(text)
	for tab in TABS:
		buildings_by_tab[tab] = []
	for b in list:
		var tab: String = b.get("tab", "")
		if not buildings_by_tab.has(tab) or not b.get("player_placeable", true) or b.get("culture", "any") == "atlantean":
			continue
		if b.has("per_god"):
			continue  # the generic sanctuary is only a base of the per-god ones
		buildings_by_tab[tab].append(b["id"])

func _button(text: String, cb: Callable, size := BUTTON) -> Button:
	var b := Button.new()
	b.text = text  # a catalog key: the button translates itself and follows a change of language
	b.custom_minimum_size = size
	b.add_theme_font_size_override("font_size", 20)
	b.pressed.connect(cb)
	return b

func _build() -> void:
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	status = Label.new()
	status.position = Vector2(16, 12)
	status.add_theme_font_size_override("font_size", 20)
	root.add_child(status)

	var top := HBoxContainer.new()
	top.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	top.position = Vector2(-110 - 3 * 76 - 140 - 24, 12)
	top.add_child(_button("ui.pause", func(): _speed(0.0), Vector2(110, 64)))
	for s in [1.0, 2.0, 4.0]:
		top.add_child(_button("%dx" % int(s), func(): _speed(s), Vector2(72, 64)))
	top.add_child(_button("ui.undo", func(): game.sim.undo(), Vector2(140, 64)))
	root.add_child(top)

	var bottom := VBoxContainer.new()
	bottom.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	bottom.grow_vertical = Control.GROW_DIRECTION_BEGIN
	bottom.offset_top = -3 * (BUTTON.y + 6) - 40
	root.add_child(bottom)

	hint = Label.new()
	hint.add_theme_font_size_override("font_size", 20)
	bottom.add_child(hint)

	actions = HBoxContainer.new()
	actions.add_child(_button("ui.confirm", _confirm))
	actions.add_child(_button("ui.rotate", func(): turn = (turn + 1) % 4; _refresh_ghost()))
	actions.add_child(_button("ui.cancel", func(): _set_mode("")))
	bottom.add_child(actions)

	var items_scroll := ScrollContainer.new()
	items_scroll.custom_minimum_size = Vector2(0, BUTTON.y + 8)
	items_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	items_box = HBoxContainer.new()
	items_scroll.add_child(items_box)
	bottom.add_child(items_scroll)

	var tabs_scroll := ScrollContainer.new()
	tabs_scroll.custom_minimum_size = Vector2(0, BUTTON.y + 8)
	tabs_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	var tabs := HBoxContainer.new()
	tabs.add_child(_button("ui.road", func(): _set_mode("road")))
	tabs.add_child(_button("ui.demolish", func(): _set_mode("demolish")))
	for tab in TABS:
		tabs.add_child(_button("tab." + tab, func(): _show_tab(tab)))
	tabs_scroll.add_child(tabs)
	bottom.add_child(tabs_scroll)

	card = PanelContainer.new()
	card.position = Vector2(16, 120)
	card.custom_minimum_size = Vector2(420, 0)
	var card_box := VBoxContainer.new()
	card_text = Label.new()
	card_text.add_theme_font_size_override("font_size", 20)
	card_box.add_child(card_text)
	card_box.add_child(_button("ui.close", func(): card.visible = false))
	card.add_child(card_box)
	card.visible = false
	root.add_child(card)

func _show_tab(tab: String) -> void:
	for c in items_box.get_children():
		c.queue_free()
	for id in buildings_by_tab[tab]:
		items_box.add_child(_button("building." + id, func(): _select(id), Vector2(170, BUTTON.y)))

func _select(id: String) -> void:
	selected = id
	turn = 0
	_set_mode("build")

func _set_mode(m: String) -> void:
	mode = m
	target = Vector2i(-1, -1)
	road_from = Vector2i(-1, -1)
	game.clear_ghost()
	actions.visible = m != ""
	match m:
		"build":
			hint.text = tr("building." + selected) + ": " + tr("ui.tap_to_place")
		"road":
			hint.text = tr("ui.drag_road")
		"demolish":
			hint.text = tr("ui.tap_to_demolish")
		_:
			hint.text = ""

func _speed(s: float) -> void:
	game.ticks_per_second = 60.0 * s

func update_status() -> void:
	status.text = tr("ui.status") % [game.sim.get_year() + 1, game.sim.get_month() + 1, game.sim.get_treasury(),
		game.sim.get_population(), game.sim.get_employed(), game.sim.get_workforce(), game.sim.get_popularity()]

# Map input from boot.gd; returns true when the UI used the event (otherwise the camera pans).
func map_input(event: InputEvent) -> bool:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			pressed = true
			moved = false
			press_pos = event.position
			if mode == "road":
				road_from = game.screen_to_tile(event.position)
				target = road_from
				_refresh_ghost()
				return true
			return false
		pressed = false
		if not moved:
			tap(game.screen_to_tile(event.position))
			return true
		return mode == "road"
	if event is InputEventMouseMotion and pressed:
		if event.position.distance_to(press_pos) > TAP_SLOP:
			moved = true
		if mode == "road":
			target = game.screen_to_tile(event.position)
			_refresh_ghost()
			return true
	return false

# A tap on a tile: place the ghost, mark what to demolish, or open the card of a building.
func tap(tile: Vector2i) -> void:
	match mode:
		"build", "demolish":
			target = tile
			_refresh_ghost()
		"road":
			road_from = tile
			target = tile
			_refresh_ghost()
		_:
			_open_card(tile)

func _road_tiles() -> Array:
	# An L: along x first, then along y (06-MOBILE-UX: drag, see the cost, confirm).
	var out := []
	if road_from.x < 0:
		return out
	var x_step := 1 if target.x >= road_from.x else -1
	for x in range(road_from.x, target.x + x_step, x_step):
		out.append(Vector2i(x, road_from.y))
	var y_step := 1 if target.y >= road_from.y else -1
	for y in range(road_from.y + y_step, target.y + y_step, y_step):
		out.append(Vector2i(target.x, y))
	return out

func _refresh_ghost() -> void:
	if target.x < 0:
		return
	match mode:
		"build":
			var r: Dictionary = game.sim.check_placement(selected, target.x, target.y, turn)
			game.show_ghost(r["tiles"], r["ok"])
			hint.text = tr("building." + selected) + ": " + (tr("ui.place_ok") if r["ok"] else tr(r["reason"])) + \
				"   " + tr("ui.cost") % int(r["cost"])
		"road":
			var tiles := PackedVector2Array()
			for t in _road_tiles():
				tiles.append(Vector2(t.x, t.y))
			game.show_ghost(tiles, true)
			hint.text = tr("ui.road") + ": %d" % tiles.size()
		"demolish":
			game.show_ghost(PackedVector2Array([Vector2(target.x, target.y)]), false)

func _confirm() -> void:
	if target.x < 0:
		return
	match mode:
		"build":
			game.sim.place_building(selected, target.x, target.y, turn)
			target = Vector2i(-1, -1)  # stay in the mode: the next tap places another one
			game.clear_ghost()
		"road":
			for t in _road_tiles():
				game.sim.place_road(t.x, t.y)
			road_from = Vector2i(-1, -1)
			target = Vector2i(-1, -1)
			game.clear_ghost()
		"demolish":
			game.sim.demolish(target.x, target.y)
			target = Vector2i(-1, -1)
			game.clear_ghost()

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
