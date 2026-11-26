# Boot scene: smoke test of the whole chain (core -> GDExtension -> Mobile renderer) on a demo city.
# Placeholder visuals only (boxes and capsules); real presentation modules come in E1-E9 (docs/SILNIK.md).
# 1 tile = 1 m (docs/ASSETY.md). Tile (x, y) maps to world (x + 0.5, 0, y + 0.5).
extends Node3D

var ticks_per_second := 60.0  # presentation speed; the simulation itself is tick based (harness: speed=)
const TERRAIN_COLORS := {
	"grassland": Color(0.55, 0.62, 0.35), "meadow": Color(0.62, 0.72, 0.30), "forest": Color(0.22, 0.40, 0.20),
	"marsh": Color(0.40, 0.46, 0.36), "beach": Color(0.86, 0.80, 0.58), "rock": Color(0.50, 0.48, 0.45),
	"water": Color(0.22, 0.42, 0.66),
}
const ROAD_COLOR := Color(0.72, 0.64, 0.50)
const ROADBLOCK_COLOR := Color(0.70, 0.30, 0.20)
const BUILDING_COLORS := {
	"fountain": Color(0.55, 0.75, 0.95), "maintenance_office": Color(0.85, 0.70, 0.45),
	"watchpost": Color(0.80, 0.45, 0.40), "gymnasium": Color(0.92, 0.90, 0.85),
	"common_agora": Color(0.80, 0.78, 0.72), "boulevard": Color(0.35, 0.60, 0.30), "avenue": Color(0.35, 0.60, 0.30),
	"food_vendor_stall": Color(0.95, 0.80, 0.30), "oil_vendor_stall": Color(0.70, 0.75, 0.25),
	"wine_vendor_stall": Color(0.60, 0.20, 0.45), "common_housing": Color(0.85, 0.72, 0.55),
	"fleece_vendor_stall": Color(0.95, 0.95, 0.90), "carrot_farm": Color(0.90, 0.55, 0.20),
	"onion_farm": Color(0.85, 0.80, 0.55), "carding_shed": Color(0.75, 0.65, 0.50), "granary": Color(0.80, 0.70, 0.40),
	"storehouse": Color(0.55, 0.45, 0.35), "sheep": Color(1.0, 1.0, 1.0), "ruins": Color(0.35, 0.33, 0.30),
	"infirmary": Color(0.95, 0.95, 0.98), "trading_post": Color(0.55, 0.40, 0.60),
	"horse_ranch": Color(0.60, 0.42, 0.25), "palace": Color(0.93, 0.88, 0.70),
	"sanctuary_dionysus": Color(0.75, 0.55, 0.85), "artisans_guild": Color(0.65, 0.50, 0.45),
}
const FLAT_BUILDINGS := {"common_agora": 0.06, "grand_agora": 0.06, "boulevard": 0.25, "avenue": 0.25, "ruins": 0.2,
	"carrot_farm": 0.15, "onion_farm": 0.15, "sheep": 0.3}
const WALKER_COLORS := {
	"deliveryman": Color(0.55, 0.35, 0.15), "food_vendor": Color(0.95, 0.80, 0.30), "fleece_vendor": Color(0.9, 0.9, 0.9),
	"shepherd": Color(0.3, 0.7, 0.3), "trade_caravan": Color(0.6, 0.3, 0.6),
	"water_bearer": Color(0.25, 0.55, 1.0), "superintendent": Color(1.0, 0.75, 0.2),
	"watchman": Color(0.9, 0.2, 0.2), "athlete": Color(1.0, 1.0, 1.0), "peddler": Color(0.95, 0.6, 0.1),
	"wolf": Color(0.35, 0.35, 0.38), "artisan": Color(0.85, 0.5, 0.2), "ox_cart": Color(0.5, 0.4, 0.3),
	"priest": Color(0.95, 0.95, 0.6), "hoplite": Color(0.6, 0.1, 0.1), "rabble": Color(0.7, 0.3, 0.2),
	"horseman": Color(0.4, 0.05, 0.05), "immigrant": Color(0.8, 0.8, 0.6),
}

var sim := BoSimulation.new()
var label := Label.new()
var tick_accumulator := 0.0
var tiles := MultiMeshInstance3D.new()
var building_nodes := {}  # id -> Node3D
var walker_nodes := {}  # id -> MeshInstance3D
var cam := Camera3D.new()
var cam_target := Vector3(32, 0, 24)

# The demo adventure (scenarios/demo.json, D5): a 64x48 map with the first town in the west, room for the
# Palace, a sanctuary, an Artisans' Guild and a horse ranch in the east, an invasion point and the sea; Corinth
# (ally) and Sparta (rival) send a gift, a yearly request and an army. This script plays the player.
var sanctuary_placed := false
var demo_status := ""

func _ready() -> void:
	var err: String = sim.start_scenario("res://data", "res://scenarios/demo.json")
	if err != "":
		push_error(err)
	sim.place_road(22, 16)  # demo roadblock below
	sim.place_roadblock(22, 16)
	for b in [["fountain", 6, 5, 0], ["maintenance_office", 17, 17, 0], ["watchpost", 10, 14, 0],
			["gymnasium", 24, 13, 0], ["fountain", 5, 17, 0], ["fountain", 17, 5, 0], ["common_agora", 26, 6, 0],
			["boulevard", 8, 4, 1], ["boulevard", 9, 4, 1], ["boulevard", 10, 4, 1], ["avenue", 4, 22, 3],
			["palace", 42, 5, 0]]:
		sim.place_building(b[0], b[1], b[2], b[3])
	for x in range(7, 17, 2):  # house plots; immigrants arrive daily and become the workforce
		sim.place_building("common_housing", x, 17)
	for x in range(18, 28, 2):
		sim.place_building("common_housing", x, 2)
	for x in range(30, 40, 2):
		sim.place_building("common_housing", x, 17)
	# Food and fleece chains (C4, C5): farms and a carding shed on the meadow, granary and storehouse by the road.
	for b in [["carrot_farm", 6, 25], ["onion_farm", 9, 25], ["carding_shed", 12, 26], ["granary", 17, 24],
			["storehouse", 21, 25], ["trading_post", 24, 24], ["horse_ranch", 42, 29],
			["storehouse", 53, 13], ["artisans_guild", 53, 17], ["maintenance_office", 54, 5]]:
		sim.place_building(b[0], b[1], b[2])
	sim.set_sector_priority(2, 5)  # distribution first: the granary takes food only when fully staffed
	sim.set_sector_priority(3, 5)  # maintenance too: unattended buildings burn and collapse (C7)
	sim.set_sector_priority(6, 5)  # mythology: the Artisans' Guild builds the sanctuary (D1)
	for b in [["maintenance_office", 13, 5], ["maintenance_office", 6, 14], ["infirmary", 9, 9]]:
		sim.place_building(b[0], b[1], b[2])
	sim.step(1)
	var post: int = sim.get_tile(24, 24)["building"]
	sim.set_trade_partner(post, 0)
	sim.set_trade_order(post, "olive_oil", true, 4, false, 0)
	for good in ["marble", "wood", "sculpture"]:  # materials for the sanctuary (D1)
		sim.set_trade_order(post, good, true, 12, false, 0)
	for i in 8:
		sim.place_building("sheep", 7 + i % 4, 21 + i / 4)
	for s in [["food_vendor_stall", 26, 6], ["fleece_vendor_stall", 26, 8], ["oil_vendor_stall", 26, 10]]:
		sim.place_building(s[0], s[1], s[2])
	_build_environment()
	add_child(tiles)
	label.position = Vector2(16, 16)
	label.add_theme_font_size_override("font_size", 22)
	add_child(label)
	sim.step(1)
	_handle_events(sim.drain_events())
	_rebuild_tiles()
	_update_label()

# Demo script: the sanctuary is founded once the imported marble is in storage; an army comes in month 6.
func _demo_day() -> void:
	if not sanctuary_placed and _city_stock("marble") >= 8:
		sim.place_building("sanctuary_dionysus", 41, 17)
	var scenario: Dictionary = sim.get_scenario()
	var requests: Array = scenario["requests"]
	if requests.size() > 0 and _city_stock(requests[0]["good"]) >= requests[0]["amount"]:
		sim.fulfill_request(0)  # the player sends Corinth what it asked for (Z:162)
	demo_status = "marmur %d" % _city_stock("marble")
	for b in sim.get_buildings():
		if b.has("sanctuary_complete"):
			demo_status = "sanktuarium: %s, ładunki do wbudowania %d" % [
				"gotowe" if b["sanctuary_complete"] else "budowa", b["sanctuary_loads_needed"]]
	demo_status += "   kompanie %d" % sim.get_companies().size()
	var met := 0
	for g in scenario["goals"]:
		met += 1 if g["met"] else 0
	demo_status += "   cele %d/%d   prośby %d" % [met, scenario["goals"].size(), requests.size()]
	var cities: Array = sim.get_cities()
	if cities.size() > 1 and cities[1]["tribute_demand"] > 0:
		demo_status += "   trybut dla Sparty %d dr/rok" % cities[1]["tribute_demand"]

func _city_stock(good: String) -> int:
	var total := 0
	for b in sim.get_buildings():
		if b.has("stock_loads") and b["type"] == "storehouse":
			total += int(b["stock_loads"].get(good, 0))
	return total

func _process(delta: float) -> void:
	tick_accumulator += delta * ticks_per_second
	var ticks := int(tick_accumulator)
	if ticks > 0:
		tick_accumulator -= ticks
		var day_before: int = sim.get_day_in_month()
		sim.step(ticks)
		_handle_events(sim.drain_events())
		if sim.get_day_in_month() != day_before:
			_demo_day()
		_update_label()
	_sync_walkers()

func _unhandled_input(event: InputEvent) -> void:
	# One-finger or mouse drag pans the camera (touch is emulated from mouse on PC).
	if event is InputEventScreenDrag or (event is InputEventMouseMotion and event.button_mask & MOUSE_BUTTON_MASK_LEFT):
		var rel: Vector2 = event.relative
		var k := cam.size / float(get_viewport().get_visible_rect().size.y)
		var right := cam.global_transform.basis.x
		var fwd := Vector3(cam.global_transform.basis.z.x, 0, cam.global_transform.basis.z.z).normalized()
		cam_target -= (right * rel.x + fwd * rel.y * 1.4) * k
		_place_camera()

func _update_label() -> void:
	# Player-facing text will come from translation catalogs (docs/PRAKTYKI.md ch. 12); this is a debug overlay.
	label.text = "rok %d  miesiąc %d  dzień %d   skarbiec %d\nludność %d   pracownicy %d/%d   popularność %d\nspacerujący %d   hash %s\n%s" % [
		sim.get_year() + 1, sim.get_month() + 1, sim.get_day_in_month() + 1, sim.get_treasury(),
		sim.get_population(), sim.get_employed(), sim.get_workforce(), sim.get_popularity(),
		walker_nodes.size(), sim.get_state_hash(), demo_status]

func _handle_events(events: Array) -> void:
	var tiles_dirty := false
	for e in events:
		match e["type"]:
			"building_placed":
				_add_building(e)
				if e["building_type"].begins_with("sanctuary_"):
					sanctuary_placed = true
			"building_removed":
				if building_nodes.has(e["id"]):
					building_nodes[e["id"]].queue_free()
					building_nodes.erase(e["id"])
				tiles_dirty = true
			"tile_changed":
				tiles_dirty = true
			"walker_spawned":
				_add_walker(e["id"], e["walker_type"])
			"walker_removed":
				if walker_nodes.has(e["id"]):
					walker_nodes[e["id"]].queue_free()
					walker_nodes.erase(e["id"])
			"disaster":
				if e["kind"] == "fire_started" and building_nodes.has(e["building"]):
					_tint(building_nodes[e["building"]], Color(0.9, 0.25, 0.1))  # placeholder for fire effects (E)
	if tiles_dirty:
		_rebuild_tiles()

func _tint(node: Node3D, color: Color) -> void:
	for child in node.get_children():
		if child is MeshInstance3D:
			var mat := StandardMaterial3D.new()
			mat.albedo_color = color
			child.material_override = mat

func _rebuild_tiles() -> void:
	var w: int = sim.get_map_width()
	var h: int = sim.get_map_height()
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	var quad := BoxMesh.new()
	quad.size = Vector3(0.98, 0.1, 0.98)
	var mat := StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	quad.material = mat
	mm.mesh = quad
	mm.instance_count = w * h
	for y in h:
		for x in w:
			var t: Dictionary = sim.get_tile(x, y)
			var color: Color = TERRAIN_COLORS.get(t["terrain"], Color(0.6, 0.6, 0.6))
			var height: float = -0.05 + 0.5 * float(t["elevation"])
			if t["terrain"] == "water":
				height = -0.2
			if t["road"]:
				color = ROADBLOCK_COLOR if t["roadblock"] else ROAD_COLOR
			var i := y * w + x
			mm.set_instance_transform(i, Transform3D(Basis(), Vector3(x + 0.5, height, y + 0.5)))
			mm.set_instance_color(i, color)
	tiles.multimesh = mm

func _add_building(e: Dictionary) -> void:
	# One box per building; boulevards and avenues get a box per ornament tile (the road between stays visible).
	var node := Node3D.new()
	var bw: float = e["width"]
	var bh: float = e["height"]
	var height: float = FLAT_BUILDINGS.get(e["building_type"], 0.6 + 0.3 * bw)
	if e["parent"] != 0:
		height = 0.5  # stall on an agora
	var mat := StandardMaterial3D.new()
	mat.albedo_color = BUILDING_COLORS.get(e["building_type"], Color(0.8, 0.8, 0.8))
	var parts: Array = []
	if e["building_type"] in ["boulevard", "avenue"]:
		for y in range(e["y"], e["y"] + e["height"]):
			for x in range(e["x"], e["x"] + e["width"]):
				if sim.get_tile(x, y)["building"] == e["id"]:
					parts.append(Rect2(x, y, 1, 1))
	else:
		parts.append(Rect2(e["x"], e["y"], bw, bh))
	for r in parts:
		var mesh := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(r.size.x - 0.15, height, r.size.y - 0.15)
		box.material = mat
		mesh.mesh = box
		mesh.position = Vector3(r.position.x + r.size.x / 2.0, height / 2.0, r.position.y + r.size.y / 2.0)
		node.add_child(mesh)
	add_child(node)
	building_nodes[e["id"]] = node

func _add_walker(id: int, type: String) -> void:
	var node := MeshInstance3D.new()
	var capsule := CapsuleMesh.new()
	capsule.radius = 0.18
	capsule.height = 0.7
	var mat := StandardMaterial3D.new()
	mat.albedo_color = WALKER_COLORS.get(type, Color(0.9, 0.9, 0.9))
	capsule.material = mat
	node.mesh = capsule
	add_child(node)
	walker_nodes[id] = node

func _sync_walkers() -> void:
	# Interpolation between the current tile and the next one along the facing direction (presentation only).
	const STEP := [Vector2(0, -1), Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0)]
	for w in sim.get_walkers():
		var node: MeshInstance3D = walker_nodes.get(w["id"])
		if node == null:
			continue
		# Entered the current tile at progress 0; approximate the next tile by the last direction.
		var p: Vector2 = Vector2(w["x"] + 0.5, w["y"] + 0.5) + STEP[w["dir"]] * clampf(w["progress"], 0.0, 1.0) * 0.5
		node.position = Vector3(p.x, 0.35, p.y)

func _build_environment() -> void:
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, 35, 0)
	sun.shadow_enabled = true
	add_child(sun)
	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.62, 0.78, 0.92)
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color(0.75, 0.75, 0.8)
	env.environment = e
	add_child(env)
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	cam.size = 44
	add_child(cam)
	_place_camera()

func _place_camera() -> void:
	cam.look_at_from_position(cam_target + Vector3(20, 22, 20), cam_target)
