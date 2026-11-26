# Deterministic screenshot harness (docs/PRAKTYKI.md ch. 8). Usage:
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --resolution 1280x720 \
#   --script res://tests/screenshot.gd -- out=/abs/path.png frames=10
extends SceneTree

var frames_left := 10
var out_path := "user://shot.png"

func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("out="):
			out_path = arg.substr(4)
		elif arg.begins_with("frames="):
			frames_left = int(arg.substr(7))
	var scene: PackedScene = load("res://scenes/boot.tscn")
	root.add_child(scene.instantiate())

func _process(_delta: float) -> bool:
	frames_left -= 1
	if frames_left == 0:
		var img := root.get_texture().get_image()
		img.save_png(out_path)
		print("SHOT ", out_path, " ", img.get_width(), "x", img.get_height(), " ", RenderingServer.get_video_adapter_name())
		quit()
	return false
