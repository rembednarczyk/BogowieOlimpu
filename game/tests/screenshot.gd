# Deterministic screenshot harness (docs/PRAKTYKI.md ch. 8). Usage:
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 \
#   --script res://tests/screenshot.gd -- out=/abs/path.png frames=10 [speed=ticks_per_second]
extends SceneTree

var frames_left := 10
var out_path := "user://shot.png"
var speed := 0.0  # ticks per second of the demo; 0 keeps the scene default
var build_view := 0  # 1: a category column open, 2: houses dragged as a rectangle (E7), 3: the city report
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
	var scene: PackedScene = load("res://scenes/boot.tscn")
	node = scene.instantiate()
	node.set("use_save", false)  # never the player's save
	if speed > 0.0:
		node.set("ticks_per_second", speed)
	root.add_child(node)

func _process(_delta: float) -> bool:
	if build_view > 0 and frames_left == 5:
		TranslationServer.set_locale("pl")
		if build_view == 3:
			node.ui.toggle_report()
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
