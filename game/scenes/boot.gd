# Boot scene: smoke test of the whole chain (core -> GDExtension -> Mobile renderer) on a demo city.
# Placeholder visuals only (boxes and capsules); real presentation modules come in E1-E9 (docs/SILNIK.md).
# 1 tile = 1 m (docs/ASSETY.md). Tile (x, y) maps to world (x + 0.5, 0, y + 0.5).
extends Node3D

const TICKS_PER_SECOND := 60.0  # presentation speed; the simulation itself is tick based
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
	"wine_vendor_stall": Color(0.60, 0.20, 0.45),
}
const FLAT_BUILDINGS := {"common_agora": 0.06, "grand_agora": 0.06, "boulevard": 0.25, "avenue": 0.25}
const WALKER_COLORS := {
	"water_bearer": Color(0.25, 0.55, 1.0), "superintendent": Color(1.0, 0.75, 0.2),
	"watchman": Color(0.9, 0.2, 0.2), "athlete": Color(1.0, 1.0, 1.0), "peddler": Color(0.95, 0.6, 0.1),
}

var sim := BoSimulation.new()
var label := Label.new()
var tick_accumulator := 0.0
var tiles := MultiMeshInstance3D.new()
var building_nodes := {}  # id -> Node3D
var walker_nodes := {}  # id -> MeshInstance3D
var cam := Camera3D.new()
var cam_target := Vector3(16, 0, 16)

static func demo_map() -> PackedStringArray:
	var rows := PackedStringArray()
	for y in 32:
		var row := ""
		for x in 32:
			var c := "."
			if (y == 4 or y == 16 or y == 28 or x == 4 or x == 16 or x == 28) and x >= 2 and x < 30 and y >= 2 and y < 30:
				c = "="
			elif y == 8 and x >= 20 and x < 26:
				c = "T"
			elif y == 22 and x >= 20 and x < 26:
				c = ","
			elif x >= 30 or y >= 30:
				c = "~"
			row += c
		rows.append(row)
	return rows

func _ready() -> void:
	var err: String = sim.start_with_map("res://data", 20261126, "", 3000, demo_map(), true)  # full staffing until immigration (C8)
	if err != "":
		push_error(err)
	sim.place_road(22, 16)  # demo roadblock below
	sim.place_roadblock(22, 16)
	for b in [["fountain", 6, 5, 0], ["maintenance_office", 17, 17, 0], ["watchpost", 10, 14, 0],
			["gymnasium", 24, 13, 0], ["fountain", 5, 17, 0], ["fountain", 17, 5, 0], ["common_agora", 26, 6, 0],
			["boulevard", 8, 4, 1], ["boulevard", 9, 4, 1], ["boulevard", 10, 4, 1], ["avenue", 4, 22, 3]]:
		sim.place_building(b[0], b[1], b[2], b[3])
	sim.step(1)
	for s in [["food_vendor_stall", 26, 6], ["oil_vendor_stall", 26, 8], ["wine_vendor_stall", 26, 10]]:
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

func _process(delta: float) -> void:
	tick_accumulator += delta * TICKS_PER_SECOND
	var ticks := int(tick_accumulator)
	if ticks > 0:
		tick_accumulator -= ticks
		sim.step(ticks)
		_handle_events(sim.drain_events())
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
	label.text = "rok %d  miesiąc %d  dzień %d   skarbiec %d\nspacerujący %d   hash %s" % [
		sim.get_year() + 1, sim.get_month() + 1, sim.get_day_in_month() + 1, sim.get_treasury(),
		walker_nodes.size(), sim.get_state_hash()]

func _handle_events(events: Array) -> void:
	var tiles_dirty := false
	for e in events:
		match e["type"]:
			"building_placed":
				_add_building(e)
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
	if tiles_dirty:
		_rebuild_tiles()

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
			var height := -0.05
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
	cam.size = 30
	add_child(cam)
	_place_camera()

func _place_camera() -> void:
	cam.look_at_from_position(cam_target + Vector3(20, 22, 20), cam_target)
