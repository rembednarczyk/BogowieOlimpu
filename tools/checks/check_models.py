#!/usr/bin/env python3
"""Guards for the building models (docs/ASSETY.md, R-017), without Blender:
  - every game/models/*.glb has a provenance entry, and every entry a file;
  - one mesh with one primitive (one draw call), with POSITION, NORMAL, TEXCOORD_0 (palette), TEXCOORD_1 (pivot)
    and COLOR_0 (the masks of the building's parameters);
  - the mesh lies on the ground inside its footprint from data/building_stats.json (x 0..w, z 0..h, y >= 0);
  - the palette of tools/blender/bo_asset.py matches PALETTE in game/shaders/building.gdshader, and so does
    PALETTE_SIZE (the UV slots);
  - triangles within the budget: the close view (LOD0) at most 120 000, the far view (LOD1) at most 3 000
    (hundreds of houses at once on the phone, docs/07-TECHNOLOGIA.md).
Exit code 1 on any violation. Usage: python3 tools/checks/check_models.py [repo_root]
"""
import json
import pathlib
import re
import struct
import sys

EPS = 1e-3
TRIS = {0: 120000, 1: 3000}


def glb_json(path):
    data = path.read_bytes()
    magic, _version, _length = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise ValueError("not a glb")
    chunk_len, chunk_type = struct.unpack_from("<II", data, 12)
    if chunk_type != 0x4E4F534A:
        raise ValueError("first chunk is not JSON")
    return json.loads(data[20:20 + chunk_len])


def footprint(root, building):
    if building == "common_housing":
        return 2, 2
    stats = {o["id"]: o for o in json.loads((root / "data" / "building_stats.json").read_text())}
    w, h = stats[building]["size"].replace("×", "x").split("x")[:2]
    return int(w), int("".join(c for c in h if c.isdigit()))


def palette_blender(root):
    text = (root / "tools" / "blender" / "bo_asset.py").read_text()
    body = text[text.index("PALETTE = ["):text.index("]\nPALETTE_INDEX")]
    return re.findall(r'\("(\w+)", \(([\d., ]+)\)\)', body)


def palette_shader(root):
    text = (root / "game" / "shaders" / "building.gdshader").read_text()
    body = text[text.index("const vec3 PALETTE"):]
    body = body[:body.index(");")]
    return re.findall(r"vec3\(([\d., ]+)\),?\s*// (\w+)", body)


def main():
    args = [a for a in sys.argv[1:] if a != "-v"]
    root = pathlib.Path(args[0] if args else ".")
    errors = []
    models = root / "game" / "models"
    provenance = json.loads((models / "provenance.json").read_text())
    files = sorted(p.name for p in models.glob("*.glb"))
    sizes = []
    for name in files:
        if name not in provenance:
            errors.append(f"{name}: no provenance entry (R-017)")
    for name in provenance:
        if name not in files:
            errors.append(f"provenance.json: {name} has no file")
    for name in files:
        entry = provenance.get(name, {})
        g = glb_json(models / name)
        meshes = g.get("meshes", [])
        if len(meshes) != 1 or len(meshes[0]["primitives"]) != 1:
            errors.append(f"{name}: {len(meshes)} meshes; one mesh with one primitive (one draw call) expected")
            continue
        attrs = meshes[0]["primitives"][0]["attributes"]
        for a in ("POSITION", "NORMAL", "TEXCOORD_0", "TEXCOORD_1", "COLOR_0"):
            if a not in attrs:
                errors.append(f"{name}: no {a}")
        prim = meshes[0]["primitives"][0]
        tris = (g["accessors"][prim["indices"]]["count"] if "indices" in prim else g["accessors"][attrs["POSITION"]]["count"]) // 3
        lod = entry.get("lod", 0)
        sizes.append(f"{name} {tris}")
        if tris > TRIS[lod]:
            errors.append(f"{name}: {tris} triangles > {TRIS[lod]} for LOD{lod}")
        if "building" not in entry:
            continue
        w, h = footprint(root, entry["building"])
        acc = g["accessors"][attrs["POSITION"]]
        lo, hi = acc["min"], acc["max"]
        if lo[0] < -EPS or lo[2] < -EPS or hi[0] > w + EPS or hi[2] > h + EPS or lo[1] < -EPS:
            errors.append(f"{name}: bounds {lo}..{hi} leave the {w}x{h} footprint or go under the ground")
        if entry.get("ruin"):  # its fall (tools/blender/bo_collapse.py) beside it: "BORU", frames, chunks, fps, rows
            fall = models / name.replace(".glb", ".bin")
            data = fall.read_bytes() if fall.exists() else b""
            if data[:4] != b"BORU":
                errors.append(f"{name}: no fall {fall.name} (BORU)")
            else:
                frames, chunks, _fps = struct.unpack_from("<iii", data, 4)
                if len(data) != 16 + frames * chunks * 48:
                    errors.append(f"{fall.name}: {len(data)} bytes, {frames} frames x {chunks} chunks expected")
        if entry.get("footprint") != [w, h]:
            errors.append(f"{name}: provenance footprint {entry.get('footprint')} is not {w}x{h}")
    pb = palette_blender(root)
    ps = palette_shader(root)
    size_b = re.search(r"^PALETTE_SIZE = (\d+)", (root / "tools" / "blender" / "bo_asset.py").read_text(), re.M)
    size_s = re.search(r"const int PALETTE_SIZE = (\d+);", (root / "game" / "shaders" / "building.gdshader").read_text())
    if not size_b or not size_s or size_b.group(1) != size_s.group(1):
        errors.append(f"PALETTE_SIZE differs: blender {size_b and size_b.group(1)} shader {size_s and size_s.group(1)}")
    elif len(pb) > int(size_b.group(1)):
        errors.append(f"{len(pb)} colours do not fit PALETTE_SIZE {size_b.group(1)}")
    if [n for n, _ in pb] != [n for _, n in ps]:
        errors.append(f"palette names differ: blender {[n for n, _ in pb]} shader {[n for _, n in ps]}")
    else:
        for (n, cb), (cs, _) in zip(pb, ps):
            if [float(v) for v in cb.split(",")] != [float(v) for v in cs.split(",")]:
                errors.append(f"palette {n}: blender ({cb}) shader ({cs})")
    if "-v" in sys.argv:
        print("\n".join(sizes))
    for e in errors:
        print(e)
    print(f"check_models: {len(files)} model(s), {len(errors)} violation(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
