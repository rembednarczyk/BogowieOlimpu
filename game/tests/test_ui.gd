# Headless test of the player interface (E7, E8): right panel, building column, ghost, confirm, dragging as a
# rectangle (houses, demolish) or a line (roads, walls), instant building, card.
# Run: godot --headless --path game --script res://tests/test_ui.gd
extends "res://tests/check.gd"

var game: Node3D
var frame := 0

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
	check(Engine.max_fps == game.MAX_FPS, "the game caps the frame rate at 60 (07-TECHNOLOGIA.md)")
	ui.instant = false
	check(ui.buildings_by_tab["hygiene_safety"].has("fountain"), "fountain in its tab")
	check(not ui.buildings_by_tab["mythology"].has("sanctuary"), "no generic sanctuary")
	for tab in ui.TABS:
		check(not ui.buildings_by_tab[tab].is_empty(), "tab %s has buildings (the column scrolls when long: test_layout.gd)" % tab)

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
	check(sc["goals"].size() == 11, "eleven goals in the demo")
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
	# Building models (docs/ASSETY.md "Parametry budynku"): a model in place of the box, its look from the core.
	game._handle_events(game.sim.drain_events())
	game._update_looks()
	var modelled := 0
	var looks_ok := true
	for b in game.sim.get_buildings():
		var node: Node3D = game.building_nodes.get(b["id"])
		if node == null or not node.has_meta("model"):
			continue
		modelled += 1
		var mi: MeshInstance3D = node.get_node("Model")
		var phase: float = mi.get_instance_shader_parameter("build")  # going up: the exact share; else done
		var phase_ok: bool = phase >= 0.999 if b["age_days"] >= game.BUILD_VISUAL_DAYS else (phase > 0.0 and phase <= 1.0)
		looks_ok = looks_ok and phase_ok \
			and node.get_meta("model") == game._model_name(b["type"], b.get("house_level", 0))
	check(modelled > 20 and looks_ok, "modelled buildings follow their age and level: %d" % modelled)
	check(game._model("granary") != null and game._model("palace") == null, "a granary has a model, the palace keeps its box")
	# Two views of a house: the far one at the start, brick by brick once the camera comes close.
	var house_node: Node3D = null
	for id in game.building_nodes:
		if game.building_nodes[id].has_meta("model") and String(game.building_nodes[id].get_meta("model")).begins_with("common_housing"):
			house_node = game.building_nodes[id]
			break
	var far_tris: int = (house_node.get_node("Model") as MeshInstance3D).mesh.get_faces().size() / 3
	game.cam.size = 14.0
	game._place_camera()
	var close_tris: int = (house_node.get_node("Model") as MeshInstance3D).mesh.get_faces().size() / 3
	check(game.lod_close and close_tris > 5 * far_tris, "close up the house shows its bricks: %d vs %d triangles" % [close_tris, far_tris])
	game.cam.size = 44.0
	game._place_camera()
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
	var trow: Container = aw.army_box.get_child(2)
	check(trow.get_child(1) is OptionButton, "a tactic picker in each company row")
	aw.start_move(0)
	check(not aw.army.visible, "the panel steps aside for the tap on the map")
	ui.tap(Vector2i(40, 20))
	game.sim.step(1)
	var c0: Dictionary = game.sim.get_companies()[0]
	check(c0["x"] == 40 and c0["y"] == 20, "the tap moved the standard: %s" % [c0])
	# Groups 1-5 (Z:137): the first company in group 2, the group's row moves it by one tap.
	aw.toggle_army()
	aw.assign_group([0], 2)
	check(aw.group_members(2) == [0], "the company is in group 2")
	var grow: Container = aw.army_box.get_child(2)
	check((grow.get_child(0) as Label).text.contains("2"), "a row for group 2 above the companies")
	(grow.get_child(2) as Button).pressed.emit()
	check(aw.moving == [0] and not aw.army.visible, "the group's Move takes its companies")
	ui.tap(Vector2i(42, 22))
	game.sim.step(1)
	c0 = game.sim.get_companies()[0]
	check(c0["x"] == 42 and c0["y"] == 22, "the group moved by one tap: %s" % [c0])
	aw.assign_group([0], 0)
	check(aw.group_members(2).is_empty(), "the company out of the group")
	# A rectangle on the map picks the companies whose standards stand in it (Z:137); another drops them.
	aw.start_select(false)
	check(ui.mode == "select" and not aw.army.visible and ui.drag_kind() == "rect", "the next drag on the map selects")
	ui.begin_drag(Vector2i(38, 18))
	ui.drag_to(Vector2i(44, 24))
	ui.end_drag()
	check(aw.chosen.has(0) and aw.army.visible and ui.mode == "", "the company at (42, 22) is selected: %s" % [aw.chosen])
	var crow: Container = aw.army_box.get_child(2)
	check((crow.get_child(0) as Label).text == tr("ui.chosen_line") % aw.chosen.size(), "a row for the selection leads the list")
	aw.start_select(true)
	ui.begin_drag(Vector2i(41, 21))
	ui.drag_to(Vector2i(43, 23))
	ui.end_drag()
	check(not aw.chosen.has(0), "a second rectangle drops it from the selection")
	aw.toggle_world()
	check(aw.world.visible and aw.world_box.get_child(1) == aw.world_map, "the world panel opens on the map (Z:170)")
	check(aw.world_map.cities.size() == game.sim.get_cities().size(), "every city on the map")
	aw.world_map.size = Vector2(800, 340)
	aw.world_map.picked.call(1)
	check(aw.city == 1 and (aw.world_box.get_child(2) as Label).text.begins_with(ui._city_name(1)), "a tap on a city shows its row")
	check(aw.world_map.city_at(aw.world_map.city_pos(1)) == 1, "the tap finds the city under the finger")
	aw.world_map.picked.call(0)
	var panel_text := ""
	for n in aw.world_box.get_children():
		panel_text += (n.text + "\n") if n is Label else ""
	check(panel_text.contains(tr("ui.city_history")), "the city panel lists its history (Z:171): %s" % panel_text)
	check(game.sim.get_cities()[0]["map_x"] >= 0, "the demo cities have their places on the map (L-135)")
	# The buttons themselves: under the city's line, Gift, the good to pick, Ask, Help, Strike, Attack, Raid.
	var row: HFlowContainer = aw.world_orders()
	var before: int = game.sim.get_cities()[0]["opinion"]
	var treasury: int = game.sim.get_treasury()
	(row.get_child(0) as Button).pressed.emit()
	game.sim.step(1)
	check(game.sim.get_cities()[0]["opinion"] > before or game.sim.get_treasury() < treasury, "the Gift button gives 500 dr")
	row = aw.world_orders()
	var pick: OptionButton = row.get_child(1)
	check(pick.item_count > 1, "the goods of Corinth to choose from: %d" % pick.item_count)
	pick.select(1)
	var asked_before: int = game.sim.get_cities()[0]["opinion"]
	(row.get_child(2) as Button).pressed.emit()
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

	# The Atlantis demo (docs/DECYZJE.md 2026-11-27): its own map, an Atlantean city, Atlantean buildings only.
	var atl = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	atl.use_save = false
	atl.demo = "atlantis"
	root.add_child(atl)
	check(atl.sim.get_map_width() == 48 and atl.sim.get_scenario()["atlantean"], "the Atlantis demo starts")
	check(atl.ui.buildings_by_tab["science"].has("bibliotheke"), "the science tab in place of culture (P:10-13)")
	check(not atl.ui.buildings_by_tab.values().any(func(ids): return ids.has("theater")), "no Greek-only theater there")
	check(not game.ui.buildings_by_tab.values().any(func(ids): return ids.has("bibliotheke")), "no Atlantean ones in Greece")
	# A crosswalk over a straight stade (P:14) is drawn as a walkway above the track.
	for p in [Vector2i(13, 23), Vector2i(17, 23), Vector2i(21, 23), Vector2i(21, 27), Vector2i(21, 31), Vector2i(17, 31),
			Vector2i(13, 31), Vector2i(13, 27)]:  # where the demo's autopilot puts its ring
		atl.sim.place_building("hippodrome", p.x, p.y, 0)
	atl.sim.step(1)
	atl._handle_events(atl.sim.drain_events())
	var stade: int = atl.sim.get_tile(17, 23)["building"]
	var raised := func(id) -> int:  # parts above the track: the walkway
		return atl.building_nodes[id].get_children().filter(func(c): return c.position.y > 0.2).size() \
			if atl.building_nodes.has(id) else -1
	var plain: int = raised.call(stade)
	atl.sim.place_building("crosswalk", 18, 23, 0)
	atl.sim.step(1)
	atl._handle_events(atl.sim.drain_events())
	check(atl.sim.get_tile(18, 25)["road"], "the crosswalk's tiles are road")
	check(plain == 0 and raised.call(stade) == 4, "the stade is drawn again with a 4-tile walkway above the track")

	# The gods' demo (DECYZJE.md 2026-11-27): an adventure, the parent city's autopilot first, then the colony's.
	var gods = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	gods.use_save = false
	gods.demo = "gods"
	root.add_child(gods)
	var ad: Dictionary = gods.sim.get_adventure()
	check(ad.get("id", "") == "demo_gods" and ad["steps"] == 3 and not ad["in_colony"], "the gods' adventure starts in the parent city")
	check(gods.sim.get_map_width() == 92 and gods._build_order().ends_with("demo_gods_build.json"), "its map and autopilot")
	check(gods.DEMO_ORDER == ["greek", "atlantis", "gods", "atlantis_army", "atlantis_gods", "atlantis_elite", "heroes"], "the demo button goes through all seven")
	# The Atlantean army demo (P:15-19): its own city by the sea, Atlantean.
	var army = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	army.use_save = false
	army.demo = "atlantis_army"
	root.add_child(army)
	check(army.sim.get_map_width() == 72 and army._build_order().ends_with("demo_atlantis_army_build.json"), "the army demo's map and autopilot")
	var agods = (load("res://scenes/boot.tscn") as PackedScene).instantiate()
	agods.use_save = false
	agods.demo = "atlantis_gods"
	root.add_child(agods)
	check(agods.sim.get_map_width() == 64 and agods._build_order().ends_with("demo_atlantis_gods_build.json"), "the Atlantean gods demo's map and autopilot")
	# Special views (See ..., docs/gameplay/16): a column over each house in the water view, the rest flat.
	agods.set_overlay("water")
	var houses := 0
	for b in agods.sim.get_buildings():
		houses += 1 if b.has("population") and b["population"] > 0 else 0
	check(agods.overlay_nodes.size() == agods.sim.get_overlay("water").size(), "a column for every house in the water view")
	check(agods.ui.legend.visible and agods.ui.legend_text.text == tr("legend.water"), "the legend tells what the columns measure")
	check(agods.ui.view_pick.selected == agods.OVERLAY_VIEWS.find("water") + 1, "the panel's view picker follows the view")
	agods.ui.view_pick.item_selected.emit(agods.OVERLAY_VIEWS.find("fire") + 1)
	check(agods.overlay == "fire" and agods.ui.legend_high.text == tr("legend.high_risk"), "picked on the panel: the fire view, its risk scale")
	agods.set_overlay("appeal")
	check(agods.overlay_nodes.is_empty(), "the appeal view tints the tiles, no columns")
	agods.set_overlay("")
	check(agods.overlay == "" and agods.overlay_nodes.is_empty(), "back to the normal view")
	check(not agods.ui.legend.visible, "no legend in the normal view")
	# A monument (P:26-28): its card holds the building, its layers show as it goes up.
	agods.sim.place_building("pyramid_modest", 30, 21, 0)
	agods.sim.step(1)
	agods._handle_events(agods.sim.drain_events())
	var pid: int = agods.sim.get_tile(30, 21)["building"]
	check(pid != 0, "a pyramid placed in the Atlantean gods demo")
	agods.ui._open_card(Vector2i(30, 21))
	check(agods.ui.hold_btn.visible and not agods.ui.hold_btn.button_pressed, "the monument's card offers to hold its building")
	agods.ui.hold_btn.button_pressed = true
	agods.ui._toggle_hold()
	agods.sim.step(1)
	var held := false
	for b in agods.sim.get_buildings():
		if b["id"] == pid:
			held = b["construction_paused"] and b["monument_levels"] == 2
	check(held, "held from the card; two layers for a modest pyramid")
	agods._update_monuments()
	check(agods.building_nodes[pid].get_meta("levels_shown", -1) == 0, "a bare site draws its foundation only")
	finish("UI TESTS PASSED")
	return true
