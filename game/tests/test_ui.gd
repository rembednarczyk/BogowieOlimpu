# Headless test of the player interface (E7, E8): build bar, ghost, confirm, road drag, demolish, card.
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

func _process(_d: float) -> bool:
	frame += 1
	if frame < 2:
		return false  # _ready has run
	var ui = game.ui
	game.ticks_per_second = 0.0  # paused: the test drives the ticks
	check(ui.buildings_by_tab["hygiene_safety"].has("fountain"), "fountain in the health and safety tab")
	check(not ui.buildings_by_tab["mythology"].has("sanctuary"), "no generic sanctuary in the bar")

	# Build: tab, building, tap, ghost, confirm (06-MOBILE-UX: at most two taps to a building).
	ui._show_tab("hygiene_safety")
	ui._select("fountain")
	ui.tap(Vector2i(44, 26))
	check(game.ghost.get_child_count() == 4, "a 2x2 ghost")
	var r: Dictionary = game.sim.check_placement("fountain", 44, 26, 0)
	check(r["ok"], "the fountain may stand there: %s" % r["reason"])
	var occupied: Dictionary = game.sim.check_placement("fountain", 42, 5, 0)
	check(not occupied["ok"] and occupied["reason"] != "", "the Palace tile is taken")
	ui._confirm()
	game.sim.step(1)
	check(game.sim.get_tile(44, 26)["building"] != 0, "the fountain was built")

	# Road: drag from one tile to another, confirm.
	ui._set_mode("road")
	ui.road_from = Vector2i(30, 36)
	ui.target = Vector2i(34, 38)
	check(ui._road_tiles().size() == 5 + 2, "an L of 7 tiles")
	ui._confirm()
	game.sim.step(1)
	check(game.sim.get_tile(34, 38)["road"], "the road was built")

	# Demolish with confirmation.
	ui._set_mode("demolish")
	ui.tap(Vector2i(44, 26))
	ui._confirm()
	game.sim.step(1)
	check(game.sim.get_tile(44, 26)["building"] == 0, "the fountain was demolished")

	# Tap a building with no mode: its card.
	ui._set_mode("")
	ui.tap(Vector2i(43, 6))
	check(ui.card.visible and ui.card_text.text != "", "the card of the Palace")

	# Screen to tile: the centre of the screen hits the map.
	var t: Vector2i = game.screen_to_tile(Vector2(640, 360))
	check(t.x >= 0 and t.y >= 0, "screen to tile")
	if failures == 0:
		print("UI TESTS PASSED")
	quit(1 if failures > 0 else 0)
	return true
