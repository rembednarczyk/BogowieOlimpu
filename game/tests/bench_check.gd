# Gate of prototype E4 (docs/07-TECHNOLOGIA.md "Bramka: liczby deterministyczne"): under the software Vulkan of
# the container the draw calls and instances of the crowd scene are counted; frame times are a report only.
# xvfb-run -a godot --path game --rendering-driver vulkan --rendering-method mobile --script res://tests/bench_check.gd
extends SceneTree

var node: Node
var frames := 0
var failures := 0

func _init() -> void:
	node = (load("res://scenes/bench_crowd.tscn") as PackedScene).instantiate()
	root.add_child(node)

func check(ok: bool, what: String) -> void:
	if not ok:
		failures += 1
		print("FAIL: ", what)

func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		node._set_crowd(3000)
		node.close = true
		node._place_camera()
	if frames == 40:
		var calls: int = node.draw_calls()
		print("BENCH crowd 3000 buildings 2000: draw calls %d, walkers %d, fps %d (llvmpipe, a report)" % [calls,
			node.walkers.multimesh.instance_count, Engine.get_frames_per_second()])
		check(calls > 0 and calls < 600, "draw calls under the budget of 600 (07-TECHNOLOGIA.md): %d" % calls)
		check(node.walkers.multimesh.instance_count == 3000, "3 000 walkers in one MultiMesh")
		check(node.walkers.multimesh.buffer.size() == 3000 * 16, "one buffer of transforms and custom data")
		for arg in OS.get_cmdline_user_args():  # out=/abs/path.png: a picture of the close view
			if arg.begins_with("out="):
				root.get_texture().get_image().save_png(arg.substr(4))
		if failures == 0:
			print("BENCH CHECK PASSED")
		quit(failures)
	return false
