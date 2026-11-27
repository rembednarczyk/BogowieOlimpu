# World map (Z:170-172, docs/gameplay/14-swiat.md): the player's city in the middle, the others around it with
# what the reference shows on the map: the trade route (blue by sea, brown by land), coins for economic strength,
# shields for military strength, a flag by the relation and a torch for a revolt. A tap picks a city; its orders
# are in the row under the map (army_world_ui.gd).
# Positions: the scenarios have none yet, so the map lays the cities out itself, by sea to the east and by land
# to the west, in their order [ZAŁOŻENIE] (L-135, positions on the world map).
# Presentation only: nothing here touches the simulation.
extends Control

const LAND := Color(0.80, 0.74, 0.58)
const SEA := Color(0.42, 0.62, 0.80)
const ROUTE_SEA := Color(0.15, 0.35, 0.85)
const ROUTE_LAND := Color(0.50, 0.32, 0.16)
const RED := Color(0.80, 0.12, 0.10)
const GOLD := Color(0.95, 0.78, 0.20)
const SHIELD := Color(0.55, 0.58, 0.64)
const PICK_RADIUS := 44.0

var cities: Array = []
var names: Array = []
var selected := 0
var picked: Callable  # called with the city index on a tap

func show_cities(c: Array, n: Array, sel: int) -> void:
	cities = c
	names = n
	selected = sel
	queue_redraw()

func home() -> Vector2:
	return Vector2(size.x * 0.5, size.y * 0.5)

# Sea cities on the eastern half-ring, land cities on the western one, each half spread evenly.
func city_pos(i: int) -> Vector2:
	var by_sea: bool = cities[i]["by_sea"]
	var side := []
	for j in cities.size():
		if cities[j]["by_sea"] == by_sea:
			side.append(j)
	var k := side.find(i)
	var t := (k + 1.0) / (side.size() + 1.0)  # 0..1 from north to south
	var angle := -PI / 2 + PI * t  # east half: -90..90 degrees
	var r := Vector2(size.x * 0.40, size.y * 0.40)
	var p := Vector2(cos(angle) * r.x, sin(angle) * r.y)
	if not by_sea:
		p.x = -p.x
	return home() + p

func city_at(pos: Vector2) -> int:
	var best := -1
	var best_d := PICK_RADIUS
	for i in cities.size():
		var d := pos.distance_to(city_pos(i))
		if d < best_d:
			best_d = d
			best = i
	return best

func _gui_input(event: InputEvent) -> void:
	var tap: bool = (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT) \
		or (event is InputEventScreenTouch and event.pressed)
	if not tap:
		return
	var i := city_at(event.position)
	if i >= 0 and picked.is_valid():
		accept_event()
		picked.call(i)

func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, Vector2(size.x * 0.5, size.y)), LAND)
	draw_rect(Rect2(Vector2(size.x * 0.5, 0), Vector2(size.x * 0.5, size.y)), SEA)
	var font := get_theme_default_font()
	for i in cities.size():
		var c: Dictionary = cities[i]
		var trades: bool = c["relation"] != "rival" and (not (c["sells"] as PackedStringArray).is_empty() or not (c.get("buys", []) as Array).is_empty())
		if trades:
			draw_line(home(), city_pos(i), ROUTE_SEA if c["by_sea"] else ROUTE_LAND, 4.0)
	_flag(home(), RED, 1.6)  # the player's city: the big red banner
	for i in cities.size():
		var c: Dictionary = cities[i]
		var p := city_pos(i)
		draw_circle(p, 16.0, Color(0.93, 0.90, 0.82))
		draw_arc(p, 16.0, 0, TAU, 24, Color.WHITE if i == selected else Color(0.3, 0.3, 0.3), 5.0 if i == selected else 2.0)
		match c["relation"]:
			"vassal", "colony", "parent":
				_flag(p, RED, 0.8, GOLD)  # the player's small banner
			"ally":
				_flag(p, RED, 0.8)
		if c["rebelling"]:
			draw_line(p + Vector2(-18, 0), p + Vector2(-18, -26), ROUTE_LAND, 4.0)  # the torch
			draw_circle(p + Vector2(-18, -30), 7.0, Color(1.0, 0.55, 0.1))
		for k in int(c["economy"]):
			draw_circle(p + Vector2(-20 + 9 * k, 26), 4.5, GOLD)
		for k in int(c["military"]):
			var s := p + Vector2(-20 + 9 * k, 38)
			draw_colored_polygon(PackedVector2Array([s + Vector2(-4, -4), s + Vector2(4, -4), s + Vector2(0, 5)]), SHIELD)
		var name: String = names[i] if i < names.size() else ""
		draw_string(font, p + Vector2(-60, -42), name, HORIZONTAL_ALIGNMENT_CENTER, 120, 18, Color(0.08, 0.08, 0.1))

func _flag(at: Vector2, color: Color, scale_by: float, edge := Color(0, 0, 0, 0)) -> void:
	var top := at + Vector2(0, -34) * scale_by
	draw_line(at, top, Color(0.25, 0.2, 0.15), 3.0)
	var cloth := PackedVector2Array([top, top + Vector2(18, 5) * scale_by, top + Vector2(0, 12) * scale_by])
	draw_colored_polygon(cloth, color)
	if edge.a > 0:
		draw_polyline(cloth + PackedVector2Array([top]), edge, 2.0)
