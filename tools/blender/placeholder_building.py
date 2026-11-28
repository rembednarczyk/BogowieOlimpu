"""Placeholder building mesh from data/building_stats.json (pipeline test, not final art).

Usage: blender -b --factory-startup --python tools/blender/placeholder_building.py -- <building_id> <out.glb>
Conventions (docs/ASSETY.md): 1 tile = 1 m, front faces -Y, origin at the footprint corner (min x, min y).
"""
import json
import pathlib
import sys

import bpy

REPO = pathlib.Path(__file__).resolve().parents[2]


def footprint(building_id: str):
    stats = {o["id"]: o for o in json.loads((REPO / "data" / "building_stats.json").read_text())}
    size = stats[building_id]["size"].replace("×", "x")
    w, h = size.split("x")[0], size.split("x")[1]
    return int(w), int("".join(c for c in h if c.isdigit()))


def main():
    args = sys.argv[sys.argv.index("--") + 1:]
    building_id, out = args[0], args[1]
    w, h = footprint(building_id)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.active_object
    obj.name = building_id
    obj.scale = (w, h, 1.0)
    obj.location = (w / 2.0, h / 2.0, 0.5)  # origin of the footprint at (0, 0)
    bpy.ops.object.transform_apply(location=True, scale=True)
    bpy.ops.export_scene.gltf(filepath=out, export_yup=True)
    print(f"placeholder {building_id} {w}x{h} -> {out}")


main()
