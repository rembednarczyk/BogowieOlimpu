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
