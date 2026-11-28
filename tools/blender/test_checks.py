"""Proof that the export checks catch what the owner found by eye (R-018 point 7).
Usage: blender -b --factory-startup --python tools/blender/test_checks.py  (exit 1 on a failure)."""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import bo_asset as A  # noqa: E402
from bo_asset import M  # noqa: E402

failures = []


def expect(name, build, fails):
    A.reset()
    b = A.Builder(name, 2, 2)
    build(b)
    try:
        b.check_props()
        raised = None
    except ValueError as e:
        raised = str(e)
    if fails and (raised is None or fails not in raised):
        failures.append(f"{name}: expected '{fails}', got {raised}")
    if not fails and raised:
        failures.append(f"{name}: expected no violation, got {raised}")


def wing(b, footing):  # the flat's wing of 2026-11-28: walls from 0.3 m, a footing under them or not
    if footing:
        b.solid("footing", 0.3 * M, 1.85 * M, 0, 1.45 * M, 3.75 * M, 0.3 * M, block="wing")
    b.solid("wall", 0.35 * M, 1.85 * M, 0.3 * M, 1.4 * M, 3.7 * M, 2.1 * M, block="wing")


expect("wing without footing", lambda b: wing(b, False), "floats")
expect("wing on its footing", lambda b: wing(b, True), None)
expect("a prop in the air", lambda b: b.prop("jar", 1 * M, 1 * M, 1 * M, 1.2 * M, 1.2 * M, 1.4 * M), "jar floats")
expect("a prop on the ground", lambda b: b.prop("jar", 1 * M, 1 * M, 0, 1.2 * M, 1.2 * M, 0.4 * M), None)
expect("a box hung on a wall", lambda b: (b.solid("wall", 0, 0, 0, 4 * M, 0.4 * M, 2 * M),
                                          b.prop("box", 1 * M, 0.41 * M, 1 * M, 1.5 * M, 0.6 * M, 1.2 * M)), None)
expect("a box off the wall", lambda b: (b.solid("wall", 0, 0, 0, 4 * M, 0.4 * M, 2 * M),
                                        b.prop("box", 1 * M, 0.47 * M, 1 * M, 1.5 * M, 0.6 * M, 1.2 * M)), "box floats")
expect("a prop in a wall", lambda b: (b.solid("wall", 0, 0, 0, 4 * M, 0.4 * M, 2 * M),
                                      b.prop("jar", 1 * M, 0.3 * M, 0, 1.2 * M, 0.5 * M, 0.4 * M)), "jar goes into wall")
print("\n".join(failures) or "test_checks: all passed")
sys.exit(1 if failures else 0)
