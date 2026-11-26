# Headless integration test of the GDExtension binding (docs/PRAKTYKI.md ch. 4).
# Run: godot --headless --path game --script res://tests/test_simulation.gd
extends SceneTree

var failures := 0

func check(cond: bool, what: String) -> void:
	if not cond:
		failures += 1
		printerr("FAIL: ", what)

func _init() -> void:
	var a := BoSimulation.new()
	var err: String = a.start("res://data", 20261126, "", 10000)
	check(err == "", "start: " + err)
	check(a.get_difficulty() == "H", "default difficulty is Hero (R-013)")
	a.set_tax_rate(4)
	a.run_days(16 * 12 + 3)
	check(a.get_year() == 1 and a.get_month() == 0 and a.get_day_in_month() == 3, "calendar")
	check(a.get_tax_rate() == 4, "tax command applied")
	var events: Array = a.drain_events()
	var years := events.filter(func(e): return e["type"] == "year_started").size()
	check(years == 2, "year events: %d" % years)

	var b := BoSimulation.new()
	b.start("res://data", 20261126, "", 10000)
	b.set_tax_rate(4)
	b.run_days(16 * 12 + 3)
	check(a.get_state_hash() == b.get_state_hash(), "determinism across instances")

	var bytes: PackedByteArray = a.save()
	var c := BoSimulation.new()
	c.start("res://data", 1, "", 0)
	var load_err: String = c.load(bytes)
	check(load_err == "", "load: " + load_err)
	check(c.get_state_hash() == a.get_state_hash(), "save/load keeps the hash")

	var d := BoSimulation.new()
	check(d.start("res://no_such_dir", 1, "", 0) != "", "missing data is reported")

	# Module B through the binding: map, build commands, events, queries, walkers.
	var rows := PackedStringArray()
	for y in 12:
		rows.append("================" if y == 5 else "................")
	var m := BoSimulation.new()
	check(m.start_with_map("res://data", 7, "", 2000, rows, true) == "", "start_with_map (full staffing: no people)")
	check(m.get_map_width() == 16 and m.get_map_height() == 12, "map size")
	m.place_building("fountain", 2, 3)
	m.place_building("fountain", 2, 3)
	m.place_road(3, 9)
	m.step(40)
	var ev: Array = m.drain_events()
	check(ev.filter(func(e): return e["type"] == "building_placed").size() == 1, "building_placed event")
	check(ev.filter(func(e): return e["type"] == "command_rejected").size() == 1, "occupied tile rejected")
	check(ev.filter(func(e): return e["type"] == "walker_spawned").size() == 1, "water bearer spawned")
	check(m.get_tile(3, 9)["road"] == true, "road tile")
	check(m.get_buildings().size() == 1 and m.get_buildings()[0]["has_road_access"], "building with road access")
	check(m.get_walkers().size() == 1, "one walker out")
	check(m.get_treasury() == 2000 - 35 - 3, "costs on Hero (S-004)")

	# Undo and agora placement through the binding.
	var u := BoSimulation.new()
	var urows := PackedStringArray()
	for y in 8:
		urows.append("....=.....")
	u.start_with_map("res://data", 9, "", 500, urows)
	u.place_building("common_agora", 4, 0)
	u.step(1)
	u.place_building("food_vendor_stall", 5, 2)
	u.step(1)
	check(u.get_buildings().size() == 2, "agora with a stall")
	check(u.get_buildings()[1]["parent"] == u.get_buildings()[0]["id"], "stall parent")
	u.undo()
	u.step(1)
	var uev: Array = u.drain_events()
	check(uev.filter(func(e): return e["type"] == "undo_applied").size() == 1, "undo_applied event")
	check(u.get_buildings().size() == 1, "undo removed the stall")
	check(u.get_treasury() == 500 - 50, "stall refunded (Hero costs, S-004)")

	# Labour and finance through the binding.
	var w := BoSimulation.new()
	var wrows := PackedStringArray()
	for y in 8:
		wrows.append("==========" if y == 4 else "..........")
	w.start_with_map("res://data", 5, "", 1000, wrows)
	w.place_building("common_housing", 0, 2)
	w.place_building("fountain", 4, 2)
	w.step(1)
	check(w.get_buildings()[0].has("population"), "house fields")
	w.run_days(1)
	check(w.get_workforce() == 0 and w.get_buildings()[1]["staffing"] == 0, "no people, no workers")
	w.run_days(16)
	var fev: Array = w.drain_events()
	check(fev.filter(func(e): return e["type"] == "monthly_finance").size() == 2, "monthly_finance on day 0 of months 1 and 2")

	# Goods and terrain height through the binding.
	var g := BoSimulation.new()
	var grows := PackedStringArray()
	var gheights := PackedStringArray()
	for y in 10:
		grows.append(",,,,,,,,,,,,,,,,,,,," if y < 4 else ("====================" if y == 4 else "...................."))
		gheights.append("00000000000000000001")
	check(g.start_with_map("res://data", 5, "", 9000, grows, true, gheights) == "", "start with heights")
	check(g.get_tile(19, 0)["elevation"] == 1, "tile elevation")
	g.place_building("carrot_farm", 0, 1)
	g.place_building("granary", 8, 5)
	g.step(1)
	var gb: Array = g.get_buildings()
	check(gb.size() == 2, "farm and granary placed")
	if gb.size() < 2:
		quit(1)
		return
	check(gb[0].has("output_loads") and gb[1].has("stock_loads"), "producer and storage fields")
	g.set_storage_order(gb[1]["id"], "carrots", 3, 0)
	g.step(1)
	check(g.drain_events().filter(func(e): return e["type"] == "command_rejected").size() == 0, "storage order accepted")

	# Trade through the binding: a partner, a trading post, a caravan from the land entry.
	var t := BoSimulation.new()
	var trows := PackedStringArray()
	for y in 10:
		trows.append("E" + "=".repeat(29) if y == 6 else ".".repeat(30))
	var partner := {"id": "corinth", "relation": "ally", "by_sea": false, "sells": {"fleece": [60, 8]}, "buys": {}}
	check(t.start_with_map("res://data", 3, "", 5000, trows, true, PackedStringArray(), [partner]) == "", "start with a partner")
	t.place_building("trading_post", 20, 2)
	t.step(1)
	var post: int = t.get_tile(20, 2)["building"]
	t.set_trade_partner(post, 0)
	t.set_trade_order(post, "fleece", true, 4, false, 0)
	t.run_days(20)
	var tev: Array = t.drain_events()
	check(tev.filter(func(e): return e["type"] == "trade_done").size() >= 1, "trade_done event")

	# Army through the binding (D3): no Palace, no companies; mustering is rejected.
	check(t.get_companies().size() == 0, "no companies without a Palace")
	t.muster_company(0)
	t.step(1)
	var mev: Array = t.drain_events().filter(func(e): return e["type"] == "command_rejected")
	check(mev.size() == 1 and mev[0]["reason"] == "military.no_palace", "muster rejected without a Palace")

	# Invasion through the binding: enter at the land entry, surrender ends it.
	t.start_invasion("greek", 5)
	t.step(1)
	check(t.get_bribe_cost() == 4 * 100 + 1 * 200, "bribe cost: 3 hoplites, 1 rabble, 1 horseman")
	t.surrender()
	t.step(1)
	var iev: Array = t.drain_events()
	check(iev.filter(func(e): return e["type"] == "invasion_started").size() == 1, "invasion_started event")
	var ends: Array = iev.filter(func(e): return e["type"] == "invasion_ended")
	check(ends.size() == 1 and ends[0]["result"] == 2, "surrender ends the invasion")

	# Sanctuary through the binding (D1): marble in storage, foundation, materials still needed.
	var sa := BoSimulation.new()
	var mrows := PackedStringArray()
	for y in 30:
		mrows.append("=".repeat(40) if y == 20 else ".".repeat(40))
	check(sa.start_with_map("res://data", 4, "", 9000, mrows, true) == "", "start for sanctuary")
	sa.place_building("sanctuary_dionysus", 2, 14)
	sa.step(1)
	var mrej: Array = sa.drain_events().filter(func(e): return e["type"] == "command_rejected")
	check(mrej.size() == 1 and mrej[0]["reason"] == "build.sanctuary_needs_marble", "sanctuary needs marble")
	sa.pray(1)
	sa.step(1)
	var prej: Array = sa.drain_events().filter(func(e): return e["type"] == "command_rejected")
	check(prej.size() == 1 and prej[0]["reason"] == "myth.no_sanctuary", "prayer needs a finished sanctuary")

	# World through the binding (D4): a money gift raises Corinth's opinion (2 units x 3, doc 24).
	t.give_gift(0, "drachmas", 1000)
	t.step(1)
	var cities: Array = t.get_cities()
	check(cities.size() == 1 and cities[0]["opinion"] == 66, "gift raises the opinion: %s" % [cities])

	# Adventure through the binding (D5): the demo file, its goals and a yearly request.
	var sc := BoSimulation.new()
	check(sc.start_scenario("res://data", "res://scenarios/demo.json") == "", "start the demo adventure")
	check(sc.get_map_width() == 80 and sc.get_cities().size() == 4, "adventure map and cities")
	check(sc.get_scenario()["goals"].size() == 5, "adventure goals")
	sc.run_days(16 * 5 + 1)
	check(sc.get_scenario()["requests"].size() == 1, "the request of month 6 is open")
	# The autopilot (tools/make_demo.py): the build order runs through the binding, day 0 builds the first town.
	var auto := BoSimulation.new()
	auto.start_scenario("res://data", "res://scenarios/demo.json")
	check(auto.load_build_order("res://scenarios/demo_build.json") == "", "load the build order")
	check(auto.apply_build_order() > 100 and auto.apply_build_order() == 0, "day 0 once, nothing again the same day")
	auto.step(1)
	check(auto.get_buildings().size() > 100, "the first town stands: %d" % auto.get_buildings().size())

	# Adventure of several episodes (doc 23): example file, parent, then a colony of the player's choice.
	var ad := BoSimulation.new()
	check(ad.start_adventure("res://data", "res://scenarios/example_adventure.json") == "", "start the example adventure")
	check(ad.get_adventure()["steps"] == 4 and ad.get_adventure()["kind"] == "parent", "adventure steps")
	check(ad.continue_adventure() == "not_won", "no next episode before the goals are met")
	# B-007: the save keeps the adventure; a new instance resumes it.
	var adv_bytes: PackedByteArray = ad.save()
	var ad2 := BoSimulation.new()
	ad2.start("res://data", 1, "", 0)
	check(ad2.load(adv_bytes) == "", "load an adventure save")
	check(ad2.get_adventure().get("steps", 0) == 4 and ad2.get_state_hash() == ad.get_state_hash(), "the adventure came back")

	print("state hash ", a.get_state_hash(), " data version ", a.get_data_version())
	if failures == 0:
		print("TESTS PASSED")
	quit(1 if failures > 0 else 0)
