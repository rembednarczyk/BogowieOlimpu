# Boot scene: smoke test of the whole chain (core -> GDExtension -> Mobile renderer).
# Placeholder visuals only; real presentation modules come in E1-E9 (docs/SILNIK.md).
extends Node3D

const DAYS_PER_SECOND := 4.0  # presentation speed; the simulation itself is tick based

var sim := BoSimulation.new()
var label := Label.new()
var day_accumulator := 0.0

func _ready() -> void:
	var err: String = sim.start("res://data", 20261126, "", 10000)
	if err != "":
		push_error(err)
	_build_placeholder_world()
	label.position = Vector2(16, 16)
	label.add_theme_font_size_override("font_size", 22)
	add_child(label)
	_update_label()

func _process(delta: float) -> void:
	day_accumulator += delta * DAYS_PER_SECOND
	var days := int(day_accumulator)
	if days > 0:
		day_accumulator -= days
		sim.run_days(days)
		sim.drain_events()
		_update_label()

func _update_label() -> void:
	# Player-facing text will come from translation catalogs (docs/PRAKTYKI.md ch. 12); this is a debug overlay.
	label.text = "rok %d  miesiąc %d  dzień %d\nhash %s" % [sim.get_year() + 1, sim.get_month() + 1, sim.get_day_in_month() + 1, sim.get_state_hash()]

func _build_placeholder_world() -> void:
	var ground := MeshInstance3D.new()
	var plane := PlaneMesh.new()
	plane.size = Vector2(32, 32)  # 1 tile = 1 m (docs/ASSETY.md)
	ground.mesh = plane
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.55, 0.62, 0.35)
	ground.material_override = mat
	add_child(ground)
	for i in 6:
		var box := MeshInstance3D.new()
		var bm := BoxMesh.new()
		bm.size = Vector3(2, 1 + i * 0.4, 2)
		box.mesh = bm
		box.position = Vector3(-8 + i * 3, bm.size.y / 2.0, -2 + (i % 2) * 4)
		add_child(box)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, 35, 0)
	sun.shadow_enabled = true
	add_child(sun)
	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.62, 0.78, 0.92)
	e.ambient_light_color = Color(0.8, 0.8, 0.8)
	env.environment = e
	add_child(env)
	var cam := Camera3D.new()
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	cam.size = 24
	add_child(cam)
	cam.look_at_from_position(Vector3(20, 20, 20), Vector3.ZERO)
