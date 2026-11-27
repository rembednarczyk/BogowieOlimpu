# Boot scene: smoke test of the whole chain (core -> GDExtension -> Mobile renderer) on a demo city.
# Placeholder visuals only (boxes and capsules); real presentation modules come in E1-E9 (docs/SILNIK.md).
# 1 tile = 1 m (docs/ASSETY.md). Tile (x, y) maps to world (x + 0.5, 0, y + 0.5).
extends Node3D

const TICKS_PER_DAY := 51  # core/include/bo/core/time.h
var ticks_per_second := 60.0  # presentation speed; the simulation itself is tick based (harness: speed=)
const TERRAIN_COLORS := {
	"grassland": Color(0.55, 0.62, 0.35), "meadow": Color(0.62, 0.72, 0.30), "forest": Color(0.22, 0.40, 0.20),
	"marsh": Color(0.40, 0.46, 0.36), "beach": Color(0.86, 0.80, 0.58), "rock": Color(0.50, 0.48, 0.45),
	"water": Color(0.22, 0.42, 0.66),
}
const ROAD_COLOR := Color(0.72, 0.64, 0.50)
const ROADBLOCK_COLOR := Color(0.70, 0.30, 0.20)
const BUILDING_COLORS := {
	"hippodrome": Color(0.7, 0.55, 0.35),
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
	"wall": Color(0.62, 0.60, 0.55), "tower": Color(0.55, 0.53, 0.50), "gatehouse": Color(0.58, 0.56, 0.52),
	"sanctuary_hermes": Color(0.70, 0.80, 0.95), "sanctuary_athena": Color(0.85, 0.85, 0.60),
	"heros_hall": Color(0.95, 0.75, 0.35), "elite_housing": Color(0.95, 0.85, 0.70), "college": Color(0.80, 0.80, 0.95),
	"podium": Color(0.70, 0.70, 0.90), "drama_school": Color(0.85, 0.60, 0.75), "theater": Color(0.90, 0.65, 0.80),
	"tax_office": Color(0.85, 0.80, 0.30), "timber_mill": Color(0.45, 0.32, 0.20), "foundry": Color(0.70, 0.40, 0.25),
	"mint": Color(0.80, 0.80, 0.82), "masonry_shop": Color(0.85, 0.85, 0.80), "sculpture_studio": Color(0.90, 0.90, 0.88),
	"armory": Color(0.45, 0.45, 0.50), "olive_press": Color(0.55, 0.60, 0.25), "winery": Color(0.50, 0.20, 0.35),
	"growers_lodge": Color(0.50, 0.60, 0.30), "olive_tree": Color(0.40, 0.55, 0.25), "grapevine": Color(0.45, 0.25, 0.45),
	"dairy": Color(0.95, 0.92, 0.80), "goat": Color(0.85, 0.80, 0.70), "wheat_farm": Color(0.90, 0.80, 0.35),
	"fishery": Color(0.40, 0.55, 0.70), "urchin_quay": Color(0.45, 0.35, 0.55), "pier": Color(0.50, 0.40, 0.30),
	"trireme_wharf": Color(0.40, 0.35, 0.30), "flower_garden": Color(0.60, 0.80, 0.40), "gazebo": Color(0.85, 0.85, 0.75),
	"column": Color(0.92, 0.92, 0.90), "boar_hunters_lodge": Color(0.45, 0.30, 0.25), "hunting_lodge": Color(0.50, 0.40, 0.30),
}
const CROSSWALK_COLOR := Color(0.85, 0.75, 0.55)
const FLAT_BUILDINGS := {"common_agora": 0.06, "grand_agora": 0.06, "boulevard": 0.25, "avenue": 0.25, "ruins": 0.2,
	"carrot_farm": 0.15, "onion_farm": 0.15, "wheat_farm": 0.15, "sheep": 0.3, "goat": 0.3, "olive_tree": 0.8,
	"grapevine": 0.5, "flower_garden": 0.2, "hippodrome": 0.12}
const WALKER_COLORS := {
	"deliveryman": Color(0.55, 0.35, 0.15), "food_vendor": Color(0.95, 0.80, 0.30), "fleece_vendor": Color(0.9, 0.9, 0.9),
	"shepherd": Color(0.3, 0.7, 0.3), "trade_caravan": Color(0.6, 0.3, 0.6),
	"water_bearer": Color(0.25, 0.55, 1.0), "superintendent": Color(1.0, 0.75, 0.2),
	"watchman": Color(0.9, 0.2, 0.2), "athlete": Color(1.0, 1.0, 1.0), "peddler": Color(0.95, 0.6, 0.1),
	"wolf": Color(0.35, 0.35, 0.38), "artisan": Color(0.85, 0.5, 0.2), "ox_cart": Color(0.5, 0.4, 0.3),
	"priest": Color(0.95, 0.95, 0.6), "hoplite": Color(0.6, 0.1, 0.1), "rabble": Color(0.7, 0.3, 0.2),
	"horseman": Color(0.4, 0.05, 0.05), "immigrant": Color(0.8, 0.8, 0.6),
	"calydonian_boar": Color(0.25, 0.15, 0.1), "ares_warriors": Color(0.5, 0.0, 0.0),
	"trireme": Color(0.85, 0.9, 1.0), "transport": Color(0.45, 0.3, 0.15), "warship": Color(0.15, 0.15, 0.2),
}

var sim := BoSimulation.new()
var label := Label.new()  # developer overlay (hash, demo script); player texts are in build_ui.gd
var ui: CanvasLayer
var ghost := Node3D.new()
var touches := {}  # finger index -> position, for the pinch zoom
var tick_accumulator := 0.0
var tiles := MultiMeshInstance3D.new()
var building_nodes := {}  # id -> Node3D
# Building models (docs/ASSETY.md, tools/blender/models.py): one mesh per model, one shared material; a building
# without a model keeps its box. Houses have a model per level: common_housing_<level>.
const MODELS_DIR := "res://models/"
const BUILD_VISUAL_DAYS := 4.0  # the visual building phase (O-005); the building works from the first tick
const AGE_FULL_DAYS := 1920.0   # ten years of weathering [ZAŁOŻENIE]
var model_meshes := {}  # model name -> Mesh, or null when there is none
var building_material: ShaderMaterial
var walker_nodes := {}  # id -> MeshInstance3D
var cam := Camera3D.new()
var cam_target := Vector3(30, 0, 24)

# The demo (scenarios/demo.json and demo_build.json, generated by tools/make_demo.py): an 80x60 map with every
# terrain and spawn point; the autopilot builds the city in stages, day by day, like the player (the owner's
# rule: the demo can be won and shows every mechanic; the core gate test core/tests/test_demo.cpp wins it).
var save_path := "user://autosave.bo"  # tests use their own file
var use_save := true  # tests and the screenshot harness switch it off
# Which demo (docs/DECYZJE.md 2026-11-27): "greek" (tools/make_demo.py) or "atlantis" (tools/make_demo_atlantis.py,
# Atlantean buildings only there, P:16-17); kept in user://demo.cfg, each with its own autosave.
const DEMO_CFG := "user://demo.cfg"
const DEMOS := {"greek": ["res://scenarios/demo.json", "res://scenarios/demo_build.json", "user://autosave.bo"],
	"atlantis": ["res://scenarios/demo_atlantis.json", "res://scenarios/demo_atlantis_build.json",
		"user://autosave_atlantis.bo"],
	# The gods' adventure (doc 23): the parent city, then its colony, each with its own autopilot.
	"gods": ["res://scenarios/demo_gods.json", "res://scenarios/demo_gods_build.json", "user://autosave_gods.bo",
		"res://scenarios/demo_gods_colony_build.json"],
	# The expansion's army (tools/make_demo_atlantis_army.py, P:15-19): its own city by the sea.
	"atlantis_army": ["res://scenarios/demo_atlantis_army.json", "res://scenarios/demo_atlantis_army_build.json",
		"user://autosave_atlantis_army.bo"],
	# The expansion's gods (tools/make_demo_atlantis_gods.py, P:20-26): Hera, Atlas, the Citadel of Poseidon.
	"atlantis_gods": ["res://scenarios/demo_atlantis_gods.json", "res://scenarios/demo_atlantis_gods_build.json",
		"user://autosave_atlantis_gods.bo"],
	# The Atlantean elite (tools/make_demo_atlantis_elite.py, P:15, P:18-19): spearmen, charioteers, chariots.
	"atlantis_elite": ["res://scenarios/demo_atlantis_elite.json", "res://scenarios/demo_atlantis_elite_build.json",
		"user://autosave_atlantis_elite.bo"],
	# The expansion's heroes (tools/make_demo_heroes.py, P:23-24): Atalanta and Bellerophon at work.
	"heroes": ["res://scenarios/demo_heroes.json", "res://scenarios/demo_heroes_build.json", "user://autosave_heroes.bo"]}
const DEMO_ORDER := ["greek", "atlantis", "gods", "atlantis_army", "atlantis_gods", "atlantis_elite", "heroes"]
var demo := "greek"

static func chosen_demo() -> String:
	var cfg := ConfigFile.new()
	return cfg.get_value("demo", "id", "greek") if cfg.load(DEMO_CFG) == OK else "greek"

# A new game in the other demo: the choice is kept, the scene starts again.
func switch_demo(id: String) -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("demo", "id", id)
	cfg.save(DEMO_CFG)
	get_tree().reload_current_scene()
const MAX_FPS := 60
var demo_status := ""
var demo_note := ""

func _ready() -> void:
	Engine.max_fps = MAX_FPS  # docs/07-TECHNOLOGIA.md: 60 steady, the 120 Hz screen not used in full (heat)
	demo = chosen_demo() if use_save else demo
	if save_path == "user://autosave.bo":  # tests set their own
		save_path = DEMOS[demo][2]
	var err: String = _start_demo()
	if err != "":
		push_error(err)
	var restored := use_save and _load_game()
	var order_err: String = sim.load_build_order(_build_order())  # after a load: days to come
	if order_err != "":
		push_error(order_err)
	if not restored:
		sim.apply_build_order()  # day 0 of the build order
	_build_environment()
	add_child(tiles)
	label.add_theme_font_size_override("font_size", 13)
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	ui = preload("res://scenes/ui/build_ui.gd").new()
	add_child(ui)
	ui.setup(self)
	ui.dev_slot.add_child(label)  # the developer line in the left column, under the status
	apply_ui_scale()
	if not get_window().size_changed.is_connected(apply_ui_scale):
		get_window().size_changed.connect(apply_ui_scale)
	ghost.name = "Ghost"
	add_child(ghost)
	if restored:
		_restore_buildings()
	sim.step(1)
	_handle_events(sim.drain_events())
	_rebuild_tiles()
	_update_label()

# The interface in dp (ui_scale.gd): from the screen's density or the system scale and the player's slider.
# A headless run (tests) keeps the canvas the test sets.
func apply_ui_scale() -> void:
	if DisplayServer.get_name() == "headless":
		return
	var f := UiScale.device_factor(UiScale.saved_percent())
	UiScale.apply(get_window(), f)
	if ui != null:
		ui.set_safe_insets(UiScale.safe_insets(get_window(), f))

func _adventure() -> bool:
	return DEMOS[demo].size() > 3

func _start_demo() -> String:
	return sim.start_adventure("res://data", DEMOS[demo][0]) if _adventure() else \
		sim.start_scenario("res://data", DEMOS[demo][0])

# The autopilot of the episode being played: the colony has its own (scenarios/README.md).
func _build_order() -> String:
	return DEMOS[demo][3] if _adventure() and sim.get_adventure().get("in_colony", false) else DEMOS[demo][1]

# A won episode of the adventure: on to the next (the demo founds the first colony offered); the new map comes
# with a fresh scene from the save.
func _next_episode() -> void:
	var next: String = sim.continue_adventure()
	if next == "choose_colony":
		var choices: PackedStringArray = sim.get_adventure()["colony_choices"]
		if choices.is_empty() or sim.found_colony(choices[0]) != "":
			return
	if next == "victory":
		demo_note = tr("msg.adventure_won")
		return
	save_game()
	get_tree().reload_current_scene()

# Save and autosave (06-MOBILE-UX: pause and autosave when leaving the app): the whole state in one file.
func save_game() -> void:
	if not use_save:
		return
	var f := FileAccess.open(save_path, FileAccess.WRITE)
	if f != null:
		f.store_buffer(sim.save())

func _load_game() -> bool:
	if not FileAccess.file_exists(save_path):
		return false
	var demo_size := Vector2i(sim.get_map_width(), sim.get_map_height())
	var err: String = sim.load(FileAccess.get_file_as_bytes(save_path))
	if err == "" and not _adventure() and Vector2i(sim.get_map_width(), sim.get_map_height()) != demo_size:
		err = "a save of an older demo map"  # the autopilot builds on the current map only
	if err != "" and err != "save.data_version_mismatch":
		push_warning("save not loaded: " + err)
		_start_demo()
		return false
	return true

# A new game: the save goes, the scene starts again.
func new_game() -> void:
	if FileAccess.file_exists(save_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(save_path))
	get_tree().reload_current_scene()

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_APPLICATION_PAUSED or \
			what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		save_game()

# Buildings of a loaded state (no placement events come for them).
func _restore_buildings() -> void:
	for b in sim.get_buildings():
		_add_building({"id": b["id"], "building_type": b["type"], "x": b["x"], "y": b["y"], "width": b["width"],
			"height": b["height"], "parent": b["parent"], "age_days": b.get("age_days", 0),
			"house_level": b.get("house_level", 0)})

# Demo script: the autopilot's commands for the days that passed; a line of progress for the overlay.
func _demo_day() -> void:
	sim.apply_build_order()
	var scenario: Dictionary = sim.get_scenario()
	var met := 0
	for g in scenario["goals"]:
		met += 1 if g["met"] else 0
	demo_status = "cele %d/%d   prośby %d   kompanie %d" % [met, scenario["goals"].size(),
		scenario["requests"].size(), sim.get_companies().size()]
	for b in sim.get_buildings():
		if b.has("sanctuary_complete"):
			demo_status += "   %s: %s" % [b["type"], "gotowe" if b["sanctuary_complete"] else "budowa"]
	if demo_note != "":
		demo_status += "\n" + demo_note
	var cities: Array = sim.get_cities()
	if cities.size() > 1 and cities[1]["tribute_demand"] > 0:
		demo_status += "   trybut dla Sparty %d dr/rok" % cities[1]["tribute_demand"]

func _process(delta: float) -> void:
	tick_accumulator += delta * ticks_per_second
	var ticks := int(tick_accumulator)
	if ticks > 0:
		tick_accumulator -= ticks
		# Day by day: the autopilot issues its commands at each day start, as in the core gate test, however
		# many days one frame covers at high speed.
		while ticks > 0:
			var n: int = mini(ticks, TICKS_PER_DAY - sim.get_tick() % TICKS_PER_DAY)
			sim.step(n)
			ticks -= n
			_handle_events(sim.drain_events())
			if sim.get_tick() % TICKS_PER_DAY == 0:
				_demo_day()
				_update_monuments()
				_update_looks()
				if overlay != "":
					set_overlay(overlay)  # the columns follow the day
				if sim.get_day_in_month() == 0:
					save_game()  # once a month of game time
		_update_label()
		ui.update_status()
		_update_race()
	_sync_walkers()
	_run_race(delta)

func _unhandled_input(event: InputEvent) -> void:
	# Two fingers: pinch zoom (06-MOBILE-UX). The mouse wheel zooms on PC.
	if event is InputEventScreenTouch:
		if event.pressed:
			touches[event.index] = event.position
		else:
			touches.erase(event.index)
		return
	if event is InputEventScreenDrag:
		if touches.size() >= 2 and touches.has(event.index):
			# Two fingers: the gap zooms, the move pans (one finger draws in the drag-building modes).
			var other: Vector2 = touches[touches.keys()[0] if touches.keys()[0] != event.index else touches.keys()[1]]
			var before: float = (touches[event.index] as Vector2).distance_to(other)
			touches[event.index] = event.position
			var after: float = event.position.distance_to(other)
			if before > 1.0 and after > 1.0:
				_zoom(before / after)
			_pan(event.relative * 0.5)
		return
	if event is InputEventMouseButton and event.pressed and event.button_index in [MOUSE_BUTTON_WHEEL_UP, MOUSE_BUTTON_WHEEL_DOWN]:
		_zoom(0.9 if event.button_index == MOUSE_BUTTON_WHEEL_UP else 1.1)
		return
	if touches.size() >= 2:
		return  # the second finger zooms; the mouse events emulated from the first do not pan meanwhile
	if ui != null and not (event is InputEventMouseMotion and event.button_mask & MOUSE_BUTTON_MASK_RIGHT) and ui.map_input(event):
		return
	# One finger (mouse events emulated from touch) pans the camera; the right mouse button too on PC.
	if event is InputEventMouseMotion and event.button_mask & (MOUSE_BUTTON_MASK_LEFT | MOUSE_BUTTON_MASK_RIGHT):
		_pan(event.relative)

func _pan(rel: Vector2) -> void:
	var k := cam.size / float(get_viewport().get_visible_rect().size.y)
	var right := cam.global_transform.basis.x
	var fwd := Vector3(cam.global_transform.basis.z.x, 0, cam.global_transform.basis.z.z).normalized()
	cam_target -= (right * rel.x + fwd * rel.y * 1.4) * k
	_place_camera()

func _zoom(factor: float) -> void:
	cam.size = clampf(cam.size * factor, 12.0, 80.0)
	cam.far = 36.0 + 2.0 * cam.size + 20.0  # the depth range of _place_camera

# The tile under a screen point: the camera ray meets the ground plane (y = 0).
func screen_to_tile(pos: Vector2) -> Vector2i:
	var origin := cam.project_ray_origin(pos)
	var dir := cam.project_ray_normal(pos)
	if absf(dir.y) < 0.0001:
		return Vector2i(-1, -1)
	var p := origin + dir * (-origin.y / dir.y)
	var t := Vector2i(int(floor(p.x)), int(floor(p.z)))
	if t.x < 0 or t.y < 0 or t.x >= sim.get_map_width() or t.y >= sim.get_map_height():
		return Vector2i(-1, -1)
	return t

# The placement ghost (E7): one translucent box a tile, green where it may stand, red where not (or what
# will be demolished).
func show_ghost(good: PackedVector2Array, bad: PackedVector2Array) -> void:
	clear_ghost()
	for pair in [[good, Color(0.2, 0.9, 0.3, 0.55)], [bad, Color(0.95, 0.2, 0.2, 0.55)]]:
		var mat := StandardMaterial3D.new()
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		mat.albedo_color = pair[1]
		var box := BoxMesh.new()
		box.size = Vector3(0.95, 0.4, 0.95)
		box.material = mat
		for t in pair[0]:
			var m := MeshInstance3D.new()
			m.mesh = box
			m.position = Vector3(t.x + 0.5, 0.2, t.y + 0.5)
			ghost.add_child(m)

func clear_ghost() -> void:
	for c in ghost.get_children():
		c.queue_free()

func _update_label() -> void:
	# Player-facing text will come from translation catalogs (docs/PRAKTYKI.md ch. 12); this is a debug overlay.
	label.text = "walkers %d   hash %s\n%s" % [walker_nodes.size(), sim.get_state_hash(), demo_status]

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
				_redraw_track_at(e["x"], e["y"])
			"walker_spawned":
				_add_walker(e["id"], e["walker_type"])
			"walker_removed":
				if walker_nodes.has(e["id"]):
					walker_nodes[e["id"]].queue_free()
					walker_nodes.erase(e["id"])
			"monster_appeared":
				demo_note = "potwór: %s" % e["monster"]
			"monster_slain":
				demo_note = "potwór pokonany: %s" % e["monster"]
			"request_closed", "city_request", "episode_won", "episode_lost", "debt_notice":
				if ui != null and ui.report.visible:
					ui.refresh_report()
				if e["type"] == "episode_won" and _adventure():
					_next_episode.call_deferred()
			"games_held":
				demo_note = "igrzyska %s: %s%s" % [e["game"], "wygrane" if e["won"] else ("przegrane" if e["invited"] else "bez zaproszenia"),
					", miasto gospodarzem" if e["hosted"] else (", prawo do organizacji przepadło" if e["right_lost"] else "")]
			"oracle_spoke":
				if ui != null:
					ui.show_prophecy(e)
			"city_rebelled":
				demo_note = "bunt: %s" % sim.get_cities()[e["city"]]["id"]
			"city_gift":
				demo_note = "dar od %s: %d dr" % [sim.get_cities()[e["city"]]["id"], e["amount"]]
			"walker_petrified":  # Z:128: Medusa's gaze
				demo_note = tr("msg.walker_petrified")
			"god_helped":  # Z:119-122: a blessing without a prayer
				demo_note = tr("msg.god_helped") % tr("god.%s" % e["god"])
			"god_attacked":
				demo_note = "gniew boga: %s%s" % [e["god"], " (odparty)" if e["repelled"] else ""]
			"disaster":
				if e["kind"] == "fire_started" and building_nodes.has(e["building"]):
					_tint(building_nodes[e["building"]], Color(0.9, 0.25, 0.1))  # placeholder for fire effects (E)
	if tiles_dirty:
		_rebuild_tiles()

func _tint(node: Node3D, color: Color) -> void:
	for child in node.get_children():
		if child is MeshInstance3D and child.name != "Model":  # a model shows the fire through its shader
			var mat := StandardMaterial3D.new()
			mat.albedo_color = color
			child.material_override = mat

# Special views (See ..., docs/gameplay/16-interfejs-referencyjny.md; docs/06-MOBILE-UX.md): the buildings
# the view is not about lie flat, those it is about carry a column of their value; "appeal" tints the tiles.
# Presentation only: the values come from the core (BoSimulation.get_overlay).
const OVERLAY_VIEWS := ["water", "hygiene", "fire", "collapse", "unrest", "taxes", "culture", "supplies",
	"employment", "security", "appeal"]
const RISK_VIEWS := ["fire", "collapse", "unrest"]  # high is bad
var overlay := ""
var overlay_nodes: Array = []

func set_overlay(view: String) -> void:
	var was_appeal := overlay == "appeal"
	if ui != null and view != overlay:
		ui.update_legend(view)
	overlay = view
	for n in overlay_nodes:
		n.queue_free()
	overlay_nodes.clear()
	for id in building_nodes:
		building_nodes[id].scale = Vector3.ONE
	if view == "appeal":
		_tint_appeal(true)
	elif was_appeal:
		_rebuild_tiles()
	if view == "" or view == "appeal":
		return
	var values := {}
	for e in sim.get_overlay(view):
		values[e["id"]] = e["value"]
	for id in building_nodes:
		if not values.has(id):
			building_nodes[id].scale = Vector3(1, 0.12, 1)  # not this view's: flat
	for b in sim.get_buildings():
		if not values.has(b["id"]):
			continue
		var v: float = values[b["id"]] / 100.0
		var good := 1.0 - v if view in RISK_VIEWS else v
		var column := MeshInstance3D.new()
		var box := BoxMesh.new()
		var h := 0.15 + 2.5 * v
		box.size = Vector3(0.45, h, 0.45)
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color(0.85, 0.2, 0.15).lerp(Color(0.2, 0.75, 0.3), good)
		box.material = mat
		column.mesh = box
		column.position = Vector3(b["x"] + b["width"] / 2.0, 1.0 + h / 2.0, b["y"] + b["height"] / 2.0)
		add_child(column)
		overlay_nodes.append(column)

# See Appeal (Z:47): the tiles from dark (repelling) to bright (attractive).
func _tint_appeal(_on: bool) -> void:
	var w: int = sim.get_map_width()
	for i in tiles.multimesh.instance_count:
		var a: float = clampf((sim.get_appeal(i % w, i / w) + 30) / 80.0, 0.0, 1.0)
		tiles.multimesh.set_instance_color(i, Color(0.15, 0.12, 0.25).lerp(Color(1.0, 0.95, 0.6), a))

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
	var bridges: Array = []  # crosswalk tiles over a hippodrome stade (P:14)
	if e["building_type"] == "hippodrome":
		for y in range(e["y"], e["y"] + e["height"]):
			for x in range(e["x"], e["x"] + e["width"]):
				(bridges if sim.get_tile(x, y)["road"] else parts).append(Rect2(x, y, 1, 1))
	elif e["building_type"] in ["boulevard", "avenue"]:
		for y in range(e["y"], e["y"] + e["height"]):
			for x in range(e["x"], e["x"] + e["width"]):
				if sim.get_tile(x, y)["building"] == e["id"]:
					parts.append(Rect2(x, y, 1, 1))
	elif e.has("levels"):  # a monument going up layer by layer (P:26-28, L-128)
		mat.albedo_color = BUILDING_COLORS.get(e["building_type"], Color(0.86, 0.82, 0.70))  # stone
		_add_layers(node, e, mat)
	elif e["parent"] == 0 and _model(_model_name(e["building_type"], e.get("house_level", 0))) != null:
		var mi := MeshInstance3D.new()  # the building's model, its state set by _update_looks
		mi.name = "Model"
		mi.mesh = _model(_model_name(e["building_type"], e.get("house_level", 0)))
		mi.material_override = _building_material()
		mi.position = Vector3(e["x"], 0, e["y"])
		node.add_child(mi)
		node.set_meta("model", _model_name(e["building_type"], e.get("house_level", 0)))
		_set_look(mi, {"age_days": e.get("age_days", 0)})
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
	var bridge_mat := StandardMaterial3D.new()
	bridge_mat.albedo_color = CROSSWALK_COLOR
	for r in bridges:  # a raised walkway above the track
		var mesh := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(0.9, 0.12, 0.9)
		box.material = bridge_mat
		mesh.mesh = box
		mesh.position = Vector3(r.position.x + 0.5, height + 0.25, r.position.y + 0.5)
		node.add_child(mesh)
	node.set_meta("placed", e)
	add_child(node)
	building_nodes[e["id"]] = node

# A house takes the model of its level, or of the highest level below it that has one (until every level has its
# own); other buildings the model of their type.
func _model_name(type: String, house_level: int) -> String:
	if type != "common_housing":
		return type
	for level in range(house_level + 1, 0, -1):
		if _model("common_housing_%d" % level) != null:
			return "common_housing_%d" % level
	return "common_housing_1"

func _model(name: String) -> Mesh:
	if not model_meshes.has(name):
		var path := MODELS_DIR + name + ".glb"
		var mesh: Mesh = null
		if ResourceLoader.exists(path):
			var scene: Node = (load(path) as PackedScene).instantiate()
			for n in scene.find_children("*", "MeshInstance3D", true, false):
				mesh = (n as MeshInstance3D).mesh
				break
			scene.free()
		model_meshes[name] = mesh
	return model_meshes[name]

func _building_material() -> ShaderMaterial:
	if building_material == null:
		building_material = ShaderMaterial.new()
		building_material.shader = load("res://shaders/building.gdshader")
	return building_material

# A building's look from the core's state (docs/ASSETY.md "Parametry budynku"): the building phase and the
# weathering from its age, damage from the collapse counter, the fire's phase, the moving parts while it works.
func _set_look(mi: MeshInstance3D, b: Dictionary) -> void:
	var days: float = b.get("age_days", 0)
	mi.set_instance_shader_parameter("build", clampf(days / BUILD_VISUAL_DAYS, 0.0, 1.0))
	mi.set_instance_shader_parameter("age", clampf(days / AGE_FULL_DAYS, 0.0, 1.0))
	var risk: float = b.get("damage_risk", 0) / 960.0  # of the threshold (data/risks.json)
	mi.set_instance_shader_parameter("damage", 3.0 if risk >= 0.9 else (2.0 if risk >= 0.7 else (1.0 if risk >= 0.4 else 0.0)))
	var burning: int = b.get("burning_days", 0)
	var phase := 0.0 if burning == 0 else (1.0 if burning > 21 else (2.0 if burning > 10 else 3.0))  # 32 days a fire
	mi.set_instance_shader_parameter("fire", phase)
	mi.set_instance_shader_parameter("working", 1.0 if b.get("workers", 0) > 0 else 0.0)

# Daily: every modelled building follows its state; a house that changed its level changes its model.
func _update_looks() -> void:
	for b in sim.get_buildings():
		var node: Node3D = building_nodes.get(b["id"])
		if node == null or not node.has_meta("model"):
			continue
		var mi: MeshInstance3D = node.get_node("Model")
		var name := _model_name(b["type"], b.get("house_level", 0))
		if name != node.get_meta("model") and _model(name) != null:
			mi.mesh = _model(name)
			node.set_meta("model", name)
		_set_look(mi, b)

# A monument as a stepped pyramid: `built` of its `levels` layers, each a tile narrower on every side (S-008);
# a bare site shows its foundation only.
func _add_layers(node: Node3D, e: Dictionary, mat: StandardMaterial3D) -> void:
	var levels: int = e["levels"]
	var built: int = e.get("built", 0)
	for k in maxi(built, 1):
		var inset: float = minf(k, (minf(e["width"], e["height"]) - 1.0) / 2.0)
		var mesh := MeshInstance3D.new()
		var box := BoxMesh.new()
		var h := 0.8 if built > 0 else 0.06
		box.size = Vector3(e["width"] - 2.0 * inset - 0.15, h, e["height"] - 2.0 * inset - 0.15)
		var layer_mat := StandardMaterial3D.new()  # each layer a shade lighter: the steps read in flat light
		layer_mat.albedo_color = mat.albedo_color.darkened(0.25).lightened(0.12 * k)
		box.material = layer_mat
		mesh.mesh = box
		mesh.position = Vector3(e["x"] + e["width"] / 2.0, h / 2.0 + 0.8 * k, e["y"] + e["height"] / 2.0)
		node.add_child(mesh)
	node.set_meta("levels_shown", built if built <= levels else levels)

# Daily: a monument's layers follow its building (presentation only).
func _update_monuments() -> void:
	for b in sim.get_buildings():
		if not b.get("monument", false) or not building_nodes.has(b["id"]):
			continue
		var total: int = maxi(b["sanctuary_loads_total"], 1)
		var levels: int = b["monument_levels"]
		var built: int = levels if b["sanctuary_complete"] else int(levels * (total - b["sanctuary_loads_needed"]) / total)
		var node: Node3D = building_nodes[b["id"]]
		if node.get_meta("levels_shown", -1) == built:
			continue
		var e: Dictionary = node.get_meta("placed", {}).duplicate()
		e["levels"] = levels
		e["built"] = built
		node.queue_free()
		building_nodes.erase(b["id"])
		_add_building(e)

# The race (P:14; presentation only, as in eZeus): while the track races, its horses run round the loop of
# stade centres; the core keeps only how many there are.
var race_loop: Array = []  # Vector3 centres of the stades in track order
var race_horses: Array = []  # MeshInstance3D
var race_t := 0.0

func _update_race() -> void:
	var sc: Dictionary = sim.get_scenario()
	var n: int = mini(int(sc.get("hippodrome_horses", 0)), 24) if sc.get("hippodrome_working", 0) > 0 else 0
	if n == race_horses.size() and (n == 0 or race_loop.size() == int(sc["hippodrome_stades"])):
		return
	for h in race_horses:
		h.queue_free()
	race_horses.clear()
	race_loop.clear()
	if n == 0:
		return
	var stades := []
	for b in sim.get_buildings():
		if b["type"] == "hippodrome":
			stades.append(Vector2i(b["x"], b["y"]))
	var order := [stades[0]]
	while order.size() < stades.size():
		var last: Vector2i = order[-1]
		var next = stades.filter(func(s): return not order.has(s) and absi(s.x - last.x) + absi(s.y - last.y) == 4 \
			and (s.x == last.x or s.y == last.y))
		if next.is_empty():
			break
		order.append(next[0])
	for s in order:
		race_loop.append(Vector3(s.x + 2.0, 0.3, s.y + 2.0))
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.45, 0.28, 0.12)
	for i in n:
		var horse := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(0.6, 0.35, 0.25)
		box.material = mat
		horse.mesh = box
		add_child(horse)
		race_horses.append(horse)

func _run_race(delta: float) -> void:
	if race_horses.is_empty() or race_loop.size() < 2:
		return
	race_t += delta * 0.8  # stades a second
	var m := race_loop.size()
	for i in race_horses.size():
		var p := fmod(race_t + float(i) * m / race_horses.size(), m)
		var a: Vector3 = race_loop[int(p) % m]
		var b: Vector3 = race_loop[(int(p) + 1) % m]
		race_horses[i].position = a.lerp(b, p - floorf(p)) + Vector3(0, 0, 0.6 if i % 2 else -0.6)

# A crosswalk was built (P:14): its stade is drawn again with the walkway.
func _redraw_track_at(x: int, y: int) -> void:
	var id: int = sim.get_tile(x, y)["building"]
	if id == 0 or not building_nodes.has(id):
		return
	var e: Dictionary = building_nodes[id].get_meta("placed", {})
	if e.get("building_type", "") != "hippodrome":
		return
	building_nodes[id].queue_free()
	building_nodes.erase(id)
	_add_building(e)

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
			_add_walker(w["id"], w["type"])  # after a load no spawn events come
			node = walker_nodes[w["id"]]
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

# The camera's depth range follows what it sees: an orthographic camera at the default far plane (4 000 m)
# spreads the sun's shadow map so thin that the shadows blur away (checked 2026-11-27: crisp at 120 m).
func _place_camera() -> void:
	cam.look_at_from_position(cam_target + Vector3(20, 22, 20), cam_target)
	cam.near = 1.0
	cam.far = 36.0 + 2.0 * cam.size + 20.0
