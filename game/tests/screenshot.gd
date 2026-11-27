# Deterministic screenshot harness (docs/PRAKTYKI.md ch. 8). Usage:
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 \
#   --script res://tests/screenshot.gd -- out=/abs/path.png frames=10 [speed=ticks_per_second]
extends SceneTree

var frames_left := 10
var out_path := "user://shot.png"
var speed := 0.0  # ticks per second of the demo; 0 keeps the scene default
var build_view := 0  # 1: a category column open, 2: houses dragged as a rectangle (E7), 3: the city report, 4: the army, 5: the world, 6: monuments layer by layer (P:26-28)
var demo := ""  # demo=atlantis_gods and the like
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
	var scene: PackedScene = load("res://scenes/boot.tscn")
	node = scene.instantiate()
	node.set("use_save", false)  # never the player's save
	if demo != "":
		node.set("demo", demo)
	if speed > 0.0:
		node.set("ticks_per_second", speed)
	root.add_child(node)

func _process(_delta: float) -> bool:
	if build_view > 0 and frames_left == 5:
		TranslationServer.set_locale("pl")
		if build_view == 3:
			node.ui.toggle_report()
		elif build_view == 4:
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
