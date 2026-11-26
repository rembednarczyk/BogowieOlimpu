# Headless test of the player interface (E7, E8): right panel, building column, ghost, confirm, dragging as a
# rectangle (houses, demolish) or a line (roads, walls), instant building, card.
# Run: godot --headless --path game --script res://tests/test_ui.gd
extends SceneTree

var failures := 0
var game: Node3D
var frame := 0

func check(cond: bool, what: String) -> void:
	if not cond:
		failures += 1
		printerr("FAIL: ", what)

func _init() -> void:
	game = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	root.add_child(game)

func building(x: int, y: int) -> int:
	return game.sim.get_tile(x, y)["building"]

func _process(_d: float) -> bool:
	frame += 1
	if frame < 2:
		return false
	var ui = game.ui
	game.ticks_per_second = 0.0
	ui.instant = false
	check(ui.buildings_by_tab["hygiene_safety"].has("fountain"), "fountain in its tab")
	check(not ui.buildings_by_tab["mythology"].has("sanctuary"), "no generic sanctuary")
	for tab in ui.TABS:
		check(ui.buildings_by_tab[tab].size() <= 2 * ui.ITEMS_PER_COLUMN, "tab %s fits two columns" % tab)

	# One building: tab, building, tap, ghost, confirm.
	ui._show_tab("hygiene_safety")
	check(ui.flyout.visible, "the column slides out")
	ui._select("fountain")
	check(not ui.flyout.visible, "the column closes on a choice")
	check(ui.confirm_btn.disabled, "nothing to confirm yet")
	ui.tap(Vector2i(44, 26))
	check(game.ghost.get_child_count() == 4 and not ui.confirm_btn.disabled, "a 2x2 ghost and Confirm")
	ui._confirm()
	game.sim.step(1)
	check(building(44, 26) != 0, "the fountain was built")

	# Demolishing clears the other choice.
	ui._set_mode("demolish")
	check(ui.selected == "" and not ui.flyout.visible, "demolish deselects")
	ui.begin_drag(Vector2i(44, 26))
	ui.drag_to(Vector2i(45, 27))
	ui._confirm()
	game.sim.step(1)
	check(building(44, 26) == 0, "the area was demolished")

	# Roads as a line.
	ui._show_tab("general")
	ui._select("road")
	ui.begin_drag(Vector2i(30, 36))
	ui.drag_to(Vector2i(34, 38))
	check(ui._line_tiles().size() == 7, "an L of 7 tiles")
	ui.end_drag()
	ui._confirm()
	game.sim.step(1)
	check(game.sim.get_tile(34, 38)["road"], "the road was built")

	# Houses as a rectangle of 2x2 plots: an 8x2 strip over the new road (x 30-34); the fourth plot has no road.
	ui._show_tab("population")
	ui._select("common_housing")
	ui.begin_drag(Vector2i(30, 34))
	ui.drag_to(Vector2i(37, 35))
	check(ui.preview.size() == 3, "three plots by the road: %d" % ui.preview.size())
	ui._confirm()
	game.sim.step(1)
	check(building(30, 34) != 0 and building(34, 34) != 0 and building(36, 34) == 0, "the plots were built")

	# Build at once: the drag builds when the finger lifts.
	ui.instant = true
	ui._show_tab("general")
	ui._select("road")
	ui.begin_drag(Vector2i(30, 40))
	ui.drag_to(Vector2i(33, 40))
	ui.end_drag()
	game.sim.step(1)
	check(game.sim.get_tile(33, 40)["road"], "instant road")
	ui.instant = false

	# Cancel clears everything; a tap with no mode opens the card.
	ui.cancel()
	check(ui.mode == "" and ui.mode_group.get_pressed_button() == null, "cancel")
	ui.tap(Vector2i(43, 6))
	check(ui.card.visible and ui.card_text.text != "", "the card of the Palace")
	check(game.screen_to_tile(Vector2(500, 360)).x >= 0, "screen to tile")
	if failures == 0:
		print("UI TESTS PASSED")
	quit(1 if failures > 0 else 0)
	return true
