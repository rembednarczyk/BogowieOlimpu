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
	game.use_save = false  # never the player's save
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
	ui.tap(Vector2i(60, 38))
	check(game.ghost.get_child_count() == 4 and not ui.confirm_btn.disabled, "a 2x2 ghost and Confirm")
	ui._confirm()
	game.sim.step(1)
	check(building(60, 38) != 0, "the fountain was built")

	# Demolishing clears the other choice.
	ui._set_mode("demolish")
	check(ui.selected == "" and not ui.flyout.visible, "demolish deselects")
	ui.begin_drag(Vector2i(60, 38))
	ui.drag_to(Vector2i(61, 39))
	ui._confirm()
	game.sim.step(1)
	check(building(60, 38) == 0, "the area was demolished")

	# Roads as a line.
	ui._show_tab("general")
	ui._select("road")
	ui.begin_drag(Vector2i(56, 40))
	ui.drag_to(Vector2i(60, 42))
	check(ui._line_tiles().size() == 7, "an L of 7 tiles")
	ui.end_drag()
	ui._confirm()
	game.sim.step(1)
	check(game.sim.get_tile(60, 42)["road"], "the road was built")

	# Houses as a rectangle of 2x2 plots: an 8x2 strip over the new road (x 56-60); the fourth plot has no road.
	ui._show_tab("population")
	ui._select("common_housing")
	ui.begin_drag(Vector2i(56, 38))
	ui.drag_to(Vector2i(63, 39))
	check(ui.preview.size() == 3, "three plots by the road: %d" % ui.preview.size())
	ui._confirm()
	game.sim.step(1)
	check(building(56, 38) != 0 and building(60, 38) != 0 and building(62, 38) == 0, "the plots were built")

	# Build at once: the drag builds when the finger lifts.
	ui.instant = true
	ui._show_tab("general")
	ui._select("road")
	ui.begin_drag(Vector2i(56, 44))
	ui.drag_to(Vector2i(59, 44))
	ui.end_drag()
	game.sim.step(1)
	check(game.sim.get_tile(59, 44)["road"], "instant road")
	ui.instant = false

	# Cancel clears everything; a tap with no mode opens the card.
	ui.cancel()
	check(ui.mode == "" and ui.mode_group.get_pressed_button() == null, "cancel")
	ui.tap(Vector2i(55, 14))
	check(ui.card.visible and ui.card_text.text != "", "the card of the Palace")
	check(not ui.pray_btn.visible, "no prayer on the card of a Palace")
	check(game.screen_to_tile(Vector2(500, 360)).x >= 0, "screen to tile")
	# City report: goals of the episode, requests with a Send button that needs the goods in storage.
	ui.toggle_report()
	check(ui.report.visible, "the report opens")
	var sc: Dictionary = game.sim.get_scenario()
	check(sc["goals"].size() == 5, "five goals in the demo")
	check(ui._goal_text(sc["goals"][0]).length() > 0, "a goal line")
	for d in 16 * 5 + 1:  # Corinth asks for fleece in month 6; the autopilot builds on meanwhile
		game.sim.apply_build_order()
		game.sim.run_days(1)
	ui.refresh_report()
	check(game.sim.get_scenario()["requests"].size() == 1, "the request of Corinth is open")
	# The carding sheds of the FLEECE stage (day 48) shear their first fleece about half a year later.
	for d in 200:
		if ui.city_stock("fleece") >= 2:
			break
		game.sim.apply_build_order()
		game.sim.run_days(1)
	ui.refresh_report()
	check(ui.city_stock("fleece") >= 2, "fleece in storage for Corinth: %d" % ui.city_stock("fleece"))
	check(game.sim.get_scenario()["requests"].size() == 1, "the request is still open (overdue at most)")
	ui.send_request(0)
	game.sim.step(1)
	check(game.sim.get_scenario()["requests"].size() == 0, "one tap sent the fleece")
	ui.toggle_report()

	# Save and restore: a second scene loads the file and shows the same buildings.
	game.use_save = true
	game.save_path = "user://test_autosave.bo"
	game.save_game()
	var again = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	again.save_path = "user://test_autosave.bo"
	root.add_child(again)
	var saved_hash: String = game.sim.get_state_hash()
	check(again.sim.get_tick() == game.sim.get_tick() + 1, "the save was loaded (one tick on after _ready)")
	check(again.building_nodes.size() == again.sim.get_buildings().size(), "every building restored on screen: %d vs %d" % [again.building_nodes.size(), again.sim.get_buildings().size()])
	var copy := BoSimulation.new()
	copy.start_scenario("res://data", "res://scenarios/demo.json")
	copy.load(FileAccess.get_file_as_bytes("user://test_autosave.bo"))
	check(copy.get_state_hash() == saved_hash, "the file holds the state exactly")
	DirAccess.remove_absolute(ProjectSettings.globalize_path("user://test_autosave.bo"))
	if failures == 0:
		print("UI TESTS PASSED")
	quit(1 if failures > 0 else 0)
	return true
