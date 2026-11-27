# Prototype E4 (docs/SILNIK.md module E4, budgets docs/07-TECHNOLOGIA.md): a crowd of characters and a town of
# buildings drawn the way the game will draw them, to measure the reference phone (Xiaomi 15T Pro).
# - buildings: a few MultiMeshes of boxes, one per kind (instancing, E3);
# - characters: one MultiMesh of a low-poly figure; its walk is a vertex animation texture (VAT) sampled in the
#   vertex shader by VERTEX_ID and frame (the Blender pipeline will bake real ones, F3); the positions move on
#   the CPU and go to the GPU in one buffer write a frame, as walkers from the core will.
# - the frame's GPU time and the renderer's CPU time are measured apart from the frame rate: the screen's refresh
#   (120 Hz on the reference phone) caps the rate and hides the headroom;
# - the sun's shadows can be switched on, to price them before the game uses them.
# The bench runs without the game's 60 fps cap (docs/07-TECHNOLOGIA.md budgets).
# Presentation only: nothing here touches the simulation.
extends Node3D

const GRID := 120  # tiles a side
const BUILDINGS := 2000
const VAT_FRAMES := 16
const CROWDS := [800, 3000, 6000]
const KINDS := [Color(0.80, 0.62, 0.45), Color(0.70, 0.72, 0.78), Color(0.55, 0.70, 0.50), Color(0.85, 0.80, 0.62)]

var crowd := 3000
var animate := true
var close := false
var walkers := MultiMeshInstance3D.new()
var positions := PackedVector2Array()
var targets := PackedVector2Array()
var phases := PackedFloat32Array()
var buffer := PackedFloat32Array()
var cam := Camera3D.new()
var label := Label.new()
var frame_ms: Array[float] = []
var vat_material := ShaderMaterial.new()
var rng := RandomNumberGenerator.new()
var seconds := 0.0
var sun := DirectionalLight3D.new()
var shadows := false
var gpu_ms: Array[float] = []
var cpu_ms: Array[float] = []

func _ready() -> void:
	rng.seed = 20261128  # the same town every run
	Engine.max_fps = 0  # no cap here: the game's 60 is set again by boot.gd
	RenderingServer.viewport_set_measure_render_time(get_viewport().get_viewport_rid(), true)
	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.background_mode = Environment.BG_COLOR
	env.environment.background_color = Color(0.62, 0.78, 0.92)
	env.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.environment.ambient_light_color = Color(0.7, 0.7, 0.7)
	add_child(env)
	sun.rotation_degrees = Vector3(-55, 35, 0)
	sun.directional_shadow_max_distance = 200.0  # the far view holds the whole town
	add_child(sun)
	_ground()
	_buildings()
	_make_vat()
	walkers.multimesh = MultiMesh.new()
	walkers.multimesh.transform_format = MultiMesh.TRANSFORM_3D
	walkers.multimesh.use_custom_data = true
	walkers.multimesh.mesh = _figure()
	walkers.material_override = vat_material
	add_child(walkers)
	_set_crowd(crowd)
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	add_child(cam)
	_place_camera()
	_hud()

func _ground() -> void:
	var plane := MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(GRID, GRID)
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.62, 0.68, 0.45)
	pm.material = mat
	plane.mesh = pm
	plane.position = Vector3(GRID / 2.0, 0, GRID / 2.0)
	add_child(plane)

# 2 000 boxes of 1-4 tiles on a grid with streets every 6 tiles, in four MultiMeshes (one per kind).
func _buildings() -> void:
	var per_kind: Array = [[], [], [], []]
	var placed := 0
	var y := 1
	while placed < BUILDINGS and y < GRID - 4:
		var x := 1
		while placed < BUILDINGS and x < GRID - 4:
			var size := rng.randi_range(1, 3)
			var height := 0.4 + 0.35 * size + rng.randf() * 0.6
			per_kind[rng.randi() % KINDS.size()].append(Transform3D(Basis().scaled(Vector3(size - 0.15, height, size - 0.15)),
				Vector3(x + size / 2.0, height / 2.0, y + size / 2.0)))
			placed += 1
			x += size + (2 if (x / 6) % 2 == 0 else 1)
		y += 5
	for k in KINDS.size():
		var mm := MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_3D
		var box := BoxMesh.new()
		box.size = Vector3.ONE
		var mat := StandardMaterial3D.new()
		mat.albedo_color = KINDS[k]
		box.material = mat
		mm.mesh = box
		mm.instance_count = per_kind[k].size()
		for i in per_kind[k].size():
			mm.set_instance_transform(i, per_kind[k][i])
		var inst := MultiMeshInstance3D.new()
		inst.multimesh = mm
		add_child(inst)

# A low-poly figure: body, head and two legs (about 100 vertices), the legs below 0.35 swing in the VAT.
func _figure() -> ArrayMesh:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for part in [[Vector3(0, 0.55, 0), Vector3(0.22, 0.32, 0.14)], [Vector3(0, 0.82, 0), Vector3(0.13, 0.13, 0.13)],
			[Vector3(-0.06, 0.2, 0), Vector3(0.07, 0.38, 0.08)], [Vector3(0.06, 0.2, 0), Vector3(0.07, 0.38, 0.08)]]:
		var box := BoxMesh.new()
		box.size = part[1]
		var arrays := box.get_mesh_arrays()
		var verts: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
		var normals: PackedVector3Array = arrays[Mesh.ARRAY_NORMAL]
		var idx: PackedInt32Array = arrays[Mesh.ARRAY_INDEX]
		for i in idx:
			st.set_normal(normals[i])
			st.add_vertex(verts[i] + part[0])
	return st.commit()

# The walk as a texture: for each frame and vertex its offset (legs swing front and back, the body bobs).
func _make_vat() -> void:
	var mesh := _figure()
	var verts: PackedVector3Array = mesh.surface_get_arrays(0)[Mesh.ARRAY_VERTEX]
	var img := Image.create(verts.size(), VAT_FRAMES, false, Image.FORMAT_RGBAF)
	for f in VAT_FRAMES:
		var t := TAU * f / VAT_FRAMES
		for v in verts.size():
			var p := verts[v]
			var off := Vector3(0, 0.03 * absf(sin(t)), 0)
			if p.y < 0.38:
				var side := -1.0 if p.x < 0 else 1.0
				off.z += side * sin(t) * 0.18 * (0.38 - p.y) / 0.38
			img.set_pixel(v, f, Color(off.x, off.y, off.z, 0))
	var shader := Shader.new()
	shader.code = """
shader_type spatial;
uniform sampler2D vat : filter_nearest;
uniform float frames = 16.0;
uniform float fps = 12.0;
uniform bool animate = true;
void vertex() {
	float frame = animate ? mod(floor(TIME * fps + INSTANCE_CUSTOM.r * frames), frames) : 0.0;
	VERTEX += texelFetch(vat, ivec2(VERTEX_ID, int(frame)), 0).xyz;
	COLOR = vec4(INSTANCE_CUSTOM.gba, 1.0);
}
void fragment() {
	ALBEDO = COLOR.rgb;
}
"""
	vat_material.shader = shader
	vat_material.set_shader_parameter("vat", ImageTexture.create_from_image(img))
	vat_material.set_shader_parameter("frames", float(VAT_FRAMES))

func _set_crowd(n: int) -> void:
	crowd = n
	positions.resize(n)
	targets.resize(n)
	phases.resize(n)
	for i in n:
		positions[i] = Vector2(rng.randf() * GRID, rng.randf() * GRID)
		targets[i] = _street_point()
		phases[i] = rng.randf()
	walkers.multimesh.instance_count = n
	buffer.resize(n * 16)  # 12 floats of transform, 4 of custom data
	for i in n:
		var c := Color.from_hsv(rng.randf(), 0.5, 0.85)
		buffer[i * 16 + 12] = phases[i]
		buffer[i * 16 + 13] = c.r
		buffer[i * 16 + 14] = c.g
		buffer[i * 16 + 15] = c.b
	_move(0.0)

func _street_point() -> Vector2:
	return Vector2(rng.randi_range(0, GRID / 6 - 1) * 6 + 0.5, rng.randf() * GRID) if rng.randf() < 0.5 \
		else Vector2(rng.randf() * GRID, rng.randi_range(0, GRID / 5 - 1) * 5 + 0.5)

# Walkers head for a point on a street at 1.5 tiles a second and pick another there; one buffer write.
func _move(delta: float) -> void:
	for i in crowd:
		var p := positions[i]
		var to := targets[i] - p
		var d := to.length()
		if d < 0.2:
			targets[i] = _street_point()
		else:
			p += to / d * minf(d, 1.5 * delta)
			positions[i] = p
		var yaw := atan2(to.x, to.y)
		var c := cos(yaw)
		var s := sin(yaw)
		var o := i * 16
		buffer[o] = c; buffer[o + 1] = 0.0; buffer[o + 2] = s; buffer[o + 3] = p.x
		buffer[o + 4] = 0.0; buffer[o + 5] = 1.0; buffer[o + 6] = 0.0; buffer[o + 7] = 0.0
		buffer[o + 8] = -s; buffer[o + 9] = 0.0; buffer[o + 10] = c; buffer[o + 11] = p.y
	walkers.multimesh.buffer = buffer

func _place_camera() -> void:
	cam.size = 18.0 if close else 60.0
	var target := Vector3(GRID / 2.0, 0, GRID / 2.0)
	cam.look_at_from_position(target + Vector3(20, 22, 20) * (0.5 if close else 1.5), target)
	cam.near = 1.0
	cam.far = 36.0 * (0.5 if close else 1.5) + 2.0 * cam.size + 20.0  # as boot.gd: crisp shadows

func _hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var box := VBoxContainer.new()
	box.position = Vector2(16, 16)
	layer.add_child(box)
	label.add_theme_font_size_override("font_size", 18)
	box.add_child(label)
	var row := HFlowContainer.new()  # wraps on a narrow screen
	row.custom_minimum_size = Vector2(get_viewport().get_visible_rect().size.x - 32, 0)
	box.add_child(row)
	for n in CROWDS:
		row.add_child(_button(str(n), func(): _set_crowd(n)))
	row.add_child(_button("ui.bench_animation", func(): animate = not animate; vat_material.set_shader_parameter("animate", animate)))
	row.add_child(_button("ui.bench_zoom", func(): close = not close; _place_camera()))
	row.add_child(_button("ui.bench_shadows", func(): set_shadows(not shadows)))
	row.add_child(_button("ui.close", func(): get_tree().change_scene_to_file("res://scenes/boot.tscn")))

func set_shadows(on: bool) -> void:
	shadows = on
	sun.shadow_enabled = on

func _button(text: String, cb: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.custom_minimum_size = Vector2(120, 56)
	b.add_theme_font_size_override("font_size", 18)
	b.pressed.connect(cb)
	return b

func _process(delta: float) -> void:
	seconds += delta
	_move(delta)
	frame_ms.append(delta * 1000.0)
	var rid := get_viewport().get_viewport_rid()
	gpu_ms.append(RenderingServer.viewport_get_measured_render_time_gpu(rid))
	cpu_ms.append(RenderingServer.viewport_get_measured_render_time_cpu(rid) + Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0)
	for a in [frame_ms, gpu_ms, cpu_ms]:
		if a.size() > 120:
			a.pop_front()
	var avg := 0.0
	var worst := 0.0
	for ms in frame_ms:
		avg += ms
		worst = maxf(worst, ms)
	avg /= frame_ms.size()
	label.text = "\n".join([tr("ui.bench_fps") % [Engine.get_frames_per_second(), avg, worst],
		tr("ui.bench_scene") % [crowd, BUILDINGS, draw_calls()],
		tr("ui.bench_split") % [_mean(gpu_ms), _worst(gpu_ms), _mean(cpu_ms), _worst(cpu_ms),
			tr("ui.bench_shadows_on") if shadows else tr("ui.bench_shadows_off")],
		tr("ui.bench_device") % [Performance.get_monitor(Performance.MEMORY_STATIC) / 1048576.0,
			RenderingServer.get_video_adapter_name()]])

func _mean(a: Array[float]) -> float:
	var t := 0.0
	for v in a:
		t += v
	return t / maxi(a.size(), 1)

func _worst(a: Array[float]) -> float:
	var w := 0.0
	for v in a:
		w = maxf(w, v)
	return w

# Draw calls of the frame, the interface's included (comparable with the phone's 19 in 0.0.43). The Mobile
# renderer of Godot 4.7 reports no shadow pass in the render info (checked on a bare scene, 2026-11-27): the
# shadows' price is the GPU time.
func draw_calls() -> int:
	return RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME)
