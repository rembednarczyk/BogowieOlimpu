# Deterministic screenshot harness (docs/PRAKTYKI.md ch. 8). Usage:
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 \
#   --script res://tests/screenshot.gd -- out=/abs/path.png frames=10 [speed=ticks_per_second]
extends SceneTree

var frames_left := 10
var out_path := "user://shot.png"
var speed := 0.0  # ticks per second of the demo; 0 keeps the scene default
var build_view := false  # show the build bar with a ghost (E7)
var node: Node

func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("out="):
			out_path = arg.substr(4)
		elif arg.begins_with("frames="):
			frames_left = int(arg.substr(7))
		elif arg.begins_with("speed="):
			speed = float(arg.substr(6))
		elif arg == "build_view=1":
			build_view = true
	var scene: PackedScene = load("res://scenes/boot.tscn")
	node = scene.instantiate()
	if speed > 0.0:
		node.set("ticks_per_second", speed)
	root.add_child(node)

func _process(_delta: float) -> bool:
	if build_view and frames_left == 5:
		TranslationServer.set_locale("pl")
		node.ui._show_tab("hygiene_safety")
		node.ui._select("fountain")
		node.ui.tap(Vector2i(44, 26))
	frames_left -= 1
	if frames_left == 0:
		var img := root.get_texture().get_image()
		img.save_png(out_path)
		print("SHOT ", out_path, " ", img.get_width(), "x", img.get_height(), " ", RenderingServer.get_video_adapter_name())
		quit()
	return false
