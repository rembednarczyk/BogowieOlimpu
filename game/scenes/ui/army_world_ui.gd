# Army and world panels (module E, docs/06-MOBILE-UX.md): what the core can already do with companies, invasions
# and the other cities, one tap each. Rules: docs/gameplay/11-wojsko.md (Z:135-150), 14-swiat.md (Z:167-178).
# Built by build_ui.gd, which owns the shared helpers (_label, _button) and the map taps.
extends RefCounted

const UNIT_RABBLE := "rabble"

var ui  # build_ui.gd
var game  # boot.gd
var army: PanelContainer
var army_box: VBoxContainer
var world: PanelContainer
var world_box: VBoxContainer
var moving: Array = []  # companies whose standards the next map tap moves (Z:137)
var posting := -1  # the trireme whose station the next map tap sets (Z:142)

func setup(owner_ui, root: Control) -> void:
	ui = owner_ui
	game = owner_ui.game
	army = _panel(root)
	army_box = army.get_child(0)
	world = _panel(root)
	world_box = world.get_child(0)

func _panel(root: Control) -> PanelContainer:
	var p := PanelContainer.new()
	p.set_anchors_preset(Control.PRESET_FULL_RECT)
	p.offset_left = 40
	p.offset_top = 40
	p.offset_right = -ui.PANEL_W - 40
	p.offset_bottom = -40
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.13, 0.14, 0.17, 0.96)
	bg.set_content_margin_all(16)
	p.add_theme_stylebox_override("panel", bg)
	var scroll := VBoxContainer.new()
	p.add_child(scroll)
	p.visible = false
	root.add_child(p)
	return p

func toggle_army() -> void:
	world.visible = false
	army.visible = not army.visible
	if army.visible:
		refresh_army()

func toggle_world() -> void:
	army.visible = false
	world.visible = not world.visible
	if world.visible:
		refresh_world()

func refresh() -> void:
	if army.visible:
		refresh_army()
	if world.visible:
		refresh_world()

func _clear(box: VBoxContainer) -> void:
	for c in box.get_children():
		box.remove_child(c)  # at once: the new rows take the indices now, not after the frame
		c.queue_free()

func _row(box: VBoxContainer, text: String, buttons: Array) -> void:
	var row := HBoxContainer.new()
	var l: Label = ui._label(text)
	l.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(l)
	for b in buttons:
		row.add_child(b)
	box.add_child(row)

func _small(key: String, cb: Callable, enabled := true) -> Button:
	var b: Button = ui._button(key, cb, Vector2(120, ui.CELL.y))
	b.disabled = not enabled
	return b

# --- Army (Z:135-150) -------------------------------------------------------------------------------------

func refresh_army() -> void:
	_clear(army_box)
	army_box.add_child(ui._label(tr("ui.army_title"), 24))
	var inv: Dictionary = game.sim.get_invasion()
	if inv.get("active", false):  # an army in the city: fight, buy them off or give in (Z:137)
		_row(army_box, tr("ui.invasion_line") % [inv["invaders"], inv["bribe"]], [
			_small("ui.bribe", func(): game.sim.bribe_invaders(); refresh_army()),
			_small("ui.surrender", func(): game.sim.surrender(); refresh_army())])
		if inv.get("by_sea", false):
			army_box.add_child(ui._label("  " + tr("ui.invasion_by_sea")))
	var sc: Dictionary = game.sim.get_scenario()
	var man: Button = ui._button("ui.man_all", func(): game.sim.man_towers(not game.sim.get_scenario()["towers_manned"]),
		Vector2(240, ui.CELL.y))  # Man All (Z:146): sentries to every tower
	man.toggle_mode = true
	man.set_pressed_no_signal(sc.get("towers_manned", false))
	var all: Button = ui._button("ui.call_all", func(): call_all(), Vector2(240, ui.CELL.y))
	var top := HBoxContainer.new()
	top.add_child(all)
	top.add_child(man)
	army_box.add_child(top)
	var companies: Array = game.sim.get_companies()
	if companies.is_empty():
		army_box.add_child(ui._label("  " + tr("ui.no_companies")))
	# Alike companies (unit, god, status) in one row: twenty rows of rabble would not fit a phone screen.
	var groups := {}
	var order := []
	for i in companies.size():
		var c: Dictionary = companies[i]
		var key := "%s|%s|%d|%s|%d" % [c["unit"], c["nation"], c["status"], c["fled"], c.get("tactic", 0)]
		if not groups.has(key):
			groups[key] = []
			order.append(key)
		groups[key].append(i)
	for key in order:
		var ids: Array = groups[key]
		var c: Dictionary = companies[ids[0]]
		var soldiers := 0
		var morale := 0
		for i in ids:
			soldiers += int(companies[i]["soldiers"])
			morale += int(companies[i]["morale"])
		var home: bool = c["status"] == 0
		var abroad: bool = c["status"] == 6
		var text := "%s%s: %s, %s %d%%, %s" % [tr("unit." + c["unit"]), "" if c["nation"] == "" else " ★",
			tr("ui.companies_soldiers") % [ids.size(), soldiers], tr("ui.morale"), morale / ids.size(),
			tr("company.status.%d" % c["status"])]
		var tactic := OptionButton.new()  # defensive, offensive, special (Z:138-139), for the whole group
		tactic.custom_minimum_size = Vector2(170, ui.CELL.y)
		tactic.add_theme_font_size_override("font_size", 18)
		for t in 3:
			tactic.add_item(tr("tactic.%d" % t))
		tactic.select(int(c.get("tactic", 0)))
		tactic.item_selected.connect(func(t): _each(ids, func(i): game.sim.set_company_tactic(i, t)))
		_row(army_box, text, [
			tactic,
			_small("ui.muster", func(): _each(ids, func(i): game.sim.muster_company(i)), home and not c["fled"]),
			_small("ui.move", func(): start_move(ids), not home and not abroad),
			_small("ui.go_home", func(): _each(ids, func(i): game.sim.send_company_home(i)), not home and not abroad)])
	_ships()

# Triremes (Z:141-144): the order of each, its hull, and a new station by a tap on the water.
func _ships() -> void:
	var ships: Array = game.sim.get_ships()
	if ships.is_empty():
		return
	army_box.add_child(ui._label(tr("ui.ships"), 20))
	for i in ships.size():
		var sh: Dictionary = ships[i]
		var order := OptionButton.new()
		order.custom_minimum_size = Vector2(200, ui.CELL.y)
		order.add_theme_font_size_override("font_size", 18)
		for o in 3:
			order.add_item(tr("ship.order.%d" % o))
		order.select(int(sh["order"]))
		order.item_selected.connect(func(o): game.sim.set_ship_order(i, o); refresh_army())
		_row(army_box, tr("ui.ship_line") % [i + 1, sh["hp"], sh["max_hp"]], [order,
			_small("ui.ship_post", func(): posting = i; army.visible = false)])

func _each(ids: Array, f: Callable) -> void:
	for i in ids:
		f.call(i)
	refresh_army()

# Call All (Z:137): every company at home that may march.
func call_all() -> void:
	var companies: Array = game.sim.get_companies()
	for i in companies.size():
		if companies[i]["status"] == 0 and not companies[i]["fled"]:
			game.sim.muster_company(i)
	refresh_army()

func start_move(ids) -> void:
	moving = ids if ids is Array else [ids]
	army.visible = false  # the next tap on the map is the new place of the standards

# Called by build_ui.tap: true when the tap moved a company.
func map_tap(tile: Vector2i) -> bool:
	if posting >= 0 and tile.x >= 0:
		game.sim.set_ship_order(posting, 0, tile.x, tile.y)  # Hold station there (rejected off the water)
		posting = -1
		return true
	if moving.is_empty() or tile.x < 0:
		return false
	for i in moving:
		game.sim.move_company(i, tile.x, tile.y)
	moving = []
	return true

# --- World (Z:167-178) ------------------------------------------------------------------------------------

func refresh_world() -> void:
	_clear(world_box)
	world_box.add_child(ui._label(tr("ui.world_title"), 24))
	var cities: Array = game.sim.get_cities()
	var ready := _companies_for_abroad()
	for i in cities.size():
		var c: Dictionary = cities[i]
		var rel: String = tr("relation." + c["relation"]) + (", " + tr("ui.rebelling") if c["rebelling"] else "")
		var text := "%s (%s): %s %d, %s %d, %s %d" % [ui._city_name(i), rel, tr("ui.opinion"), c["opinion"],
			tr("ui.military"), c["military"], tr("ui.economy"), c["economy"]]
		if c["tribute_demand"] > 0:
			text += ", " + tr("ui.tribute_demand") % c["tribute_demand"]
		var sells: PackedStringArray = c["sells"]
		var pick := OptionButton.new()  # the good to ask for or to raid (Z:174-175, Z:178)
		pick.custom_minimum_size = Vector2(150, ui.CELL.y)
		pick.add_theme_font_size_override("font_size", 18)
		for g in sells:
			pick.add_item(tr("good." + g))
		pick.disabled = sells.is_empty()
		var chosen := func() -> String: return sells[pick.selected] if pick.selected >= 0 and sells.size() > 0 else ""
		var friendly: bool = c["relation"] in ["ally", "vassal", "colony", "parent"]
		var at_war: bool = c["relation"] in ["ally", "rival"] or c["rebelling"]
		_row(world_box, text, [
			_small("ui.gift", func(): game.sim.give_gift(i, "drachmas", 500); refresh_world(), c["relation"] != "distant"),
			pick,
			_small("ui.ask", func(): game.sim.request_from_city(i, chosen.call()); refresh_world(),
				not sells.is_empty() and friendly),
			_small("ui.attack", func(): game.sim.dispatch_army(i, false, "", ready); refresh_world(),
				at_war and not ready.is_empty()),
			_small("ui.raid", func(): game.sim.dispatch_army(i, true, chosen.call() if not sells.is_empty() else "drachmas",
				ready); refresh_world(), at_war and not ready.is_empty())])
	var exps: Array = game.sim.get_expeditions()
	if not exps.is_empty():
		world_box.add_child(ui._label(tr("ui.expeditions"), 20))
		for e in exps:
			var kind := "ui.aid" if e["aid"] else ("ui.raid" if e["raid"] else "ui.attack")
			world_box.add_child(ui._label("  " + tr("ui.expedition_line") % [tr(kind), ui._city_name(e["city"]),
				e["companies"], tr("ui.returning") if e["returning"] else tr("ui.marching"), e["days_left"]]))

# Companies that may go abroad (Z:136, Z:177): at home, not rabble.
func _companies_for_abroad() -> Array:
	var out := []
	var companies: Array = game.sim.get_companies()
	for i in companies.size():
		if companies[i]["status"] == 0 and companies[i]["unit"] != UNIT_RABBLE and companies[i]["soldiers"] > 0:
			out.append(i)
	return out
