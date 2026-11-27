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
	ui._toggle_stockpile("fleece")  # Z:80: the report's switch keeps a good from use
	game.sim.step(1)
	check(game.sim.get_stockpiled().has("fleece"), "fleece stockpiled from the report")
	ui._toggle_stockpile("fleece")
	game.sim.step(1)
	check(not game.sim.get_stockpiled().has("fleece"), "and released")

	# Army and world panels (Z:135-178): the autopilot builds on until there are companies.
	for d in 400:
		if game.sim.get_companies().size() > 0:
			break
		game.sim.apply_build_order()
		game.sim.run_days(1)
	check(game.sim.get_companies().size() > 0, "companies formed in the demo")
	var aw = ui.army_world
	aw.toggle_army()
	check(aw.army.visible and aw.army_box.get_child_count() > 2, "the army panel lists the companies")
	aw.call_all()
	game.sim.step(1)
	check(game.sim.get_companies()[0]["status"] != 0, "Call All mustered the first company")
	game.sim.set_company_tactic(0, 1)
	game.sim.step(1)
	check(game.sim.get_companies()[0]["tactic"] == 1, "the tactic of a company changed (Z:138)")
	aw.refresh_army()
	var trow: HBoxContainer = aw.army_box.get_child(2)
	check(trow.get_child(1) is OptionButton, "a tactic picker in each company row")
	aw.start_move(0)
	check(not aw.army.visible, "the panel steps aside for the tap on the map")
	ui.tap(Vector2i(40, 20))
	game.sim.step(1)
	var c0: Dictionary = game.sim.get_companies()[0]
	check(c0["x"] == 40 and c0["y"] == 20, "the tap moved the standard: %s" % [c0])
	aw.toggle_world()
	check(aw.world.visible and aw.world_box.get_child_count() > 2, "the world panel lists the cities")
	# The buttons themselves: the first city's row is label, Gift, the good to pick, Ask, Attack, Raid.
	var row: HBoxContainer = aw.world_box.get_child(1)
	var before: int = game.sim.get_cities()[0]["opinion"]
	var treasury: int = game.sim.get_treasury()
	(row.get_child(1) as Button).pressed.emit()
	game.sim.step(1)
	check(game.sim.get_cities()[0]["opinion"] > before or game.sim.get_treasury() < treasury, "the Gift button gives 500 dr")
	row = aw.world_box.get_child(1)
	var pick: OptionButton = row.get_child(2)
	check(pick.item_count > 1, "the goods of Corinth to choose from: %d" % pick.item_count)
	pick.select(1)
	var asked_before: int = game.sim.get_cities()[0]["opinion"]
	(row.get_child(3) as Button).pressed.emit()
	game.sim.step(1)
	check(game.sim.get_cities()[0]["opinion"] < asked_before, "the Ask button sent a request (every city's opinion -10)")
	aw.toggle_world()
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
