# The shared part of the headless Godot tests (docs/PRAKTYKI.md ch. 4): a test is a SceneTree script that
# extends this one, calls check() for each expectation and finish() at the end (exit code 1 on any failure).
# A plain SceneTree instead of gdUnit4 (docs/DECYZJE.md, 2026-11-27): no dependency, the same run in CI.
extends SceneTree

var failures := 0

func check(cond: bool, what: String) -> void:
	if not cond:
		failures += 1
		printerr("FAIL: ", what)

func finish(passed_line: String) -> void:
	if failures == 0:
		print(passed_line)
	quit(1 if failures > 0 else 0)
