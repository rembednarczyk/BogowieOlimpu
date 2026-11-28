# Deterministic screenshot harness (docs/PRAKTYKI.md ch. 8). Usage:
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 \
#   --script res://tests/screenshot.gd -- out=/abs/path.png frames=10 [speed=ticks_per_second]
extends SceneTree

var frames_left := 10
var out_path := "user://shot.png"
var speed := 0.0  # ticks per second of the demo; 0 keeps the scene default
var build_view := 0  # 1: a category column open, 2: houses dragged as a rectangle (E7), 3: the city report, 4: the army, 5: the world, 6: monuments layer by layer (P:26-28)
var demo := ""  # demo=atlantis_gods and the like
var zoom := 0.0
var at := Vector2(-1, -1)
var view := ""  # view=water and the like: a special view (docs/gameplay/16)
var side := false  # side=1: build_view 9 from the side (heights)
var compare := ["res://models/", "res://models_b/"]  # compare=b,c: the two sets of build_view 9 (a = game/models)
var ruin_level := 3  # ruin_level=N: build_view 10 shows that level's fall
var ruin_story := ""  # ruin_story=potters: that look's own ruin (the shelters, level 1)
var outline := -1  # outline=0 off, 1 thin (the default), 2 strong (game/shaders/outline.gdshader)
var node: Node

func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("out="):
			out_path = arg.substr(4)
		elif arg.begins_with("frames="):
			frames_left = int(arg.substr(7))
		elif arg.begins_with("speed="):
			speed = float(arg.substr(6))
		elif arg.begins_with("build_view="):
			build_view = int(arg.substr(11))
		elif arg.begins_with("demo="):
			demo = arg.substr(5)
		elif arg.begins_with("view="):
			view = arg.substr(5)
		elif arg.begins_with("zoom="):  # the camera's size in tiles (closer: smaller)
			zoom = float(arg.substr(5))
		elif arg.begins_with("at="):  # the tile the camera looks at
			var xy := arg.substr(3).split(",")
			at = Vector2(float(xy[0]), float(xy[1]))
		elif arg.begins_with("compare="):
			compare = []
			for k in arg.substr(8).split(","):
				compare.append("res://models/" if k == "a" else "res://models_%s/" % k)
		elif arg == "side=1":
			side = true
		elif arg.begins_with("ruin_story="):
			ruin_story = arg.substr(11)
		elif arg.begins_with("ruin_level="):
			ruin_level = int(arg.substr(11))
		elif arg.begins_with("outline="):
			outline = int(arg.substr(8))
		elif arg.begins_with("dpi="):  # a phone's density: the interface in dp as there (ui_scale.gd)
			UiScale.forced_dpi = int(arg.substr(4))
	var scene: PackedScene = load("res://scenes/boot.tscn")
	node = scene.instantiate()
	node.set("use_save", false)  # never the player's save
	if demo != "":
		node.set("demo", demo)
	if speed > 0.0:
		node.set("ticks_per_second", speed)
	root.add_child(node)

func _process(_delta: float) -> bool:
	if frames_left == 4 and outline >= 0:
		node.set_outline(outline)
	if frames_left == 4 and (zoom > 0.0 or at.x >= 0):
		if zoom > 0.0:
			node.cam.size = zoom
		if at.x >= 0:
			node.cam_target = Vector3(at.x, 0, at.y)
		node._place_camera()
	if view != "" and frames_left == 5:
		TranslationServer.set_locale("pl")
		node.set_overlay(view)
	if build_view > 0 and frames_left == 5:
		TranslationServer.set_locale("pl")
		if build_view == 3:
			node.ui.toggle_report()
		elif build_view == 4:
			if node.sim.get_companies().size() > 1:
				node.ui.army_world.assign_group([0, 1], 1)  # a group row above the companies (Z:137)
				node.ui.army_world.chosen = [0, 1]  # and a selection by rectangle
			node.ui.army_world.toggle_army()
		elif build_view == 5:
			node.ui.army_world.toggle_world()
		elif build_view == 6:  # four pyramids of 2 to 5 layers, each part built and whole (presentation only)
			var x := 4
			for p in [["pyramid_modest", 3, 2], ["pyramid_standard", 5, 3], ["pyramid_great", 7, 4], ["pyramid_majestic", 9, 5]]:
				for built in [p[2] / 2, p[2]]:
					node._add_building({"id": -x - built, "building_type": p[0], "x": x, "y": 46 - (0 if built == p[2] else p[1] + 1),
						"width": p[1], "height": p[1], "parent": 0, "levels": p[2], "built": built})
				x += p[1] + 1
			node.cam_target = Vector3(16, 0, 42)
			node.cam.size = 22.0
			node._place_camera()
		elif build_view == 7:  # a granary in each state of docs/ASSETY.md "Parametry budynku" (presentation only)
			var looks := [{"age_days": 0.6}, {"age_days": 1.6}, {"age_days": 3.2}, {"age_days": 40},
				{"age_days": 1920}, {"age_days": 40, "damage_risk": 500}, {"age_days": 40, "damage_risk": 700},
				{"age_days": 40, "damage_risk": 900}, {"age_days": 40, "burning_days": 25}, {"age_days": 40, "burning_days": 5}]
			for i in looks.size():
				var mi := MeshInstance3D.new()
				mi.mesh = node._model("granary")
				mi.material_override = node._building_material()
				mi.position = Vector3(2 + (i % 5) * 5, 0.2, 40 + (i / 5) * 6)
				node.add_child(mi)
				node._set_look(mi, looks[i])
			for k in 2:  # an observatory working (its dome and telescope turn) and one idle
				var obs := MeshInstance3D.new()
				obs.mesh = node._model("observatory")
				obs.material_override = node._building_material()
				obs.position = Vector3(30 + 6 * k, 0.2, 40)
				node.add_child(obs)
				node._set_look(obs, {"age_days": 40, "workers": 1 - k})
			if zoom == 0.0:
				node.cam_target = Vector3(14, 0, 45)
				node.cam.size = 26.0
				node._place_camera()
		elif build_view == 8:  # the seven levels of the house, close up; then a city house going up and falling apart
			node.lod_close = true
			var rows := []
			for level in 7:
				rows.append(["common_housing_%d" % (level + 1), {"age_days": 40}, Vector2(2 + level * 2.6, 40)])
			var phases := [0.15, 0.3, 0.45, 0.6, 0.75, 0.9]
			for i in phases.size():
				rows.append(["common_housing_7", {"age_days": phases[i] * 4.0}, Vector2(2 + i * 2.6, 44)])
			rows.append(["common_housing_7", {"age_days": 1920, "damage_risk": 900}, Vector2(2 + 6 * 2.6, 44)])
			for r in rows:
				var mi := MeshInstance3D.new()
				mi.mesh = node._model(r[0])
				mi.material_override = node._building_material()
				mi.position = Vector3(r[2].x, 0.2, r[2].y)
				node.add_child(mi)
				node._set_look(mi, r[1])
			if zoom == 0.0:
				node.cam_target = Vector3(10, 0, 42.5)
				node.cam.size = 16.0
				node._place_camera()
			node.lod_close = true
		elif build_view == 11:  # granaries with food in their 8 bins (boot.gd store_slots): empty, a little, mixed, full
			node.ui.visible = false
			var fills := [{}, {"wheat": 3}, {"wheat": 6, "carrots": 3, "cheese": 4, "fish": 8, "onions": 2},
				{"wheat": 4, "carrots": 4, "onions": 4, "cheese": 4, "fish": 4, "urchins": 4, "pork": 4, "meat": 4},
				{"oranges": 5, "meat": 7, "pork": 1, "urchins": 12}, {"wheat": 6, "carrots": 3, "cheese": 4, "fish": 8, "onions": 2}]
			for i in fills.size():
				var mi := MeshInstance3D.new()
				mi.mesh = node._model("granary_lod1" if i == 5 else "granary")
				mi.material_override = node._building_material()
				mi.position = Vector3(2 + (i % 3) * 5, 0.2, 40 + (i / 3) * 5)
				node.add_child(mi)
				node._set_look(mi, {"age_days": 40})
				mi.set_instance_shader_parameter("store", node.store_slots(fills[i]))
			if zoom == 0.0:
				node.cam_target = Vector3(9, 0, 44.5)
				node.cam.size = 14.0
				node._place_camera()
		elif build_view == 10:  # a house falling (bo_collapse.py): the fall frame by frame, then each level's ruin
			node.lod_close = true
			node.ui.visible = false
			var name := ("common_housing_%d_%s_ruin" % [ruin_level, ruin_story]) if ruin_story != "" \
				else "common_housing_%d_ruin" % ruin_level
			var times := [0.0, 0.25, 0.5, 0.75, 1.0, 1.5, -1.0]
			var rows := []
			for i in times.size():
				rows.append([name, times[i], Vector2(2 + i * 2.6, 40)])
			for level in 7:
				rows.append(["common_housing_%d_ruin" % (level + 1), -1.0, Vector2(2 + level * 2.6, 44)])
			for r in rows:
				var mi := MeshInstance3D.new()
				mi.mesh = node._model(r[0])
				mi.material_override = node._ruin_material(r[0])
				mi.position = Vector3(r[2].x, 0.2, r[2].y)
				node.add_child(mi)
				mi.set_instance_shader_parameter("collapse", r[1])
				if r[1] > 0.0 and not OS.get_cmdline_user_args().has("dust=0"):  # the fall's dust as it is that long after the start
					var dust: CPUParticles3D = node.make_dust(Vector3(r[2].x + 1.0, 0.2, r[2].y + 1.0), Vector2(2, 2), false)
					dust.preprocess = r[1]
					node.add_child(dust)
				mi.set_instance_shader_parameter("damage", 2.0)
				mi.set_instance_shader_parameter("age", 0.5)
			if zoom == 0.0:
				node.cam_target = Vector3(10, 0, 42.5)
				node.cam.size = 16.0
				node._place_camera()
		elif build_view == 9:  # proportions A (game/models) and B (game/models_b, BO_PROPORTION=B), a person by each
			node.lod_close = true
			node.ui.visible = false
			for n in node.building_nodes.values():  # an empty stage: the city's buildings hidden
				n.visible = false
			for row in 2:
				var h7 := 0.0
				var dir: String = compare[row]
				for level in 7:
					var path := dir + "common_housing_%d.glb" % (level + 1)
					if not ResourceLoader.exists(path):
						continue
					var mesh: Mesh = null
					var sc: Node = (load(path) as PackedScene).instantiate()
					for n in sc.find_children("*", "MeshInstance3D", true, false):
						mesh = (n as MeshInstance3D).mesh
						break
					sc.free()
					# From the side: each level's A and B as a pair; in the game's view: A in one street, B in the next.
					var at := Vector3(4 + level * 5.2 + row * 2.3, 0.2, 40) if side else Vector3(4 + level * 2.9, 0.2, 36 + row * 7)
					var mi := MeshInstance3D.new()
					mi.mesh = mesh
					mi.material_override = node._building_material()
					mi.position = at
					node.add_child(mi)
					node._set_look(mi, {"age_days": 40})
					h7 = mesh.get_aabb().end.y
					_person(at + Vector3(2.15, 0, 1.7))
				# Stand-ins for each set (owner, 2026-11-28): the palace 1.6x and a god's statue 2.5x its city house.
				var lx := 40.5 + row * 2.3 if side else 2.0 + row * 9.0  # in the game's view behind the streets
				var lz := 40.0 if side else 27.0
				_landmark(Vector3(lx, 0.2, lz), Vector3(2.0 if side else 4.0, h7 * 1.62, 4), Color(0.86, 0.84, 0.80))
				_landmark(Vector3(lx + (5.0 if side else 5.0), 0.2, lz + 1), Vector3(1.2, h7 * 2.5, 1.2), Color(0.72, 0.55, 0.30))
			if zoom == 0.0:
				if side:
					node.cam.size = 25.0
					node.cam.keep_aspect = Camera3D.KEEP_HEIGHT
					node.cam.look_at_from_position(Vector3(25.5, 11.0, 90), Vector3(25.5, 11.0, 40))
					node.cam.near = 1.0
					node.cam.far = 200.0
				else:
					node.cam_target = Vector3(13, 0, 37)
					node.cam.size = 26.0
					node._place_camera()
		elif build_view == 1:
			node.ui._show_tab("husbandry")
		else:
			node.ui._show_tab("population")
			node.ui._select("common_housing")
			node.ui.begin_drag(Vector2i(30, 30))
			node.ui.drag_to(Vector2i(39, 33))
	frames_left -= 1
	if frames_left == 0:
		var img := root.get_texture().get_image()
		img.save_png(out_path)
		print("SHOT ", out_path, " ", img.get_width(), "x", img.get_height(), " ", RenderingServer.get_video_adapter_name())
		quit()
	return false

# A person for scale in the comparisons: 1.75 m (0.875 tiles), a capsule in a cloak's red.
func _person(at: Vector3) -> void:
	var mi := MeshInstance3D.new()
	var cap := CapsuleMesh.new()
	cap.radius = 0.12
	cap.height = 0.875
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.75, 0.2, 0.15)
	cap.material = mat
	mi.mesh = cap
	mi.position = at + Vector3(0, 0.4375, 0)
	node.add_child(mi)

# A stand-in for a building still unmodelled: a plain block of its height.
func _landmark(corner: Vector3, size: Vector3, colour: Color) -> void:
	var mi := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	var mat := StandardMaterial3D.new()
	mat.albedo_color = colour
	box.material = mat
	mi.mesh = box
	mi.position = corner + size / 2.0
	node.add_child(mi)
