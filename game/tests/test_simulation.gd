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

	print("state hash ", a.get_state_hash(), " data version ", a.get_data_version())
	if failures == 0:
		print("TESTS PASSED")
	quit(1 if failures > 0 else 0)
