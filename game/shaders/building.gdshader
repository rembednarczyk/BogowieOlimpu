// One material for every building model (docs/ASSETY.md "Parametry budynku", O-005): the part's colour from the
// palette (UV.x) and each element's shade (UV.y), the building's state from per-instance parameters set by
// boot.gd from the core, the masks of the parts from the vertex colours (R the share of the building phase after
// which the element shows, so a wall rises course by course and a roof row by row; 1 the scaffold; G plaster,
// B loose, A animated; UV2 the pivot).
// Presentation only. Stage 1a: plain colours by part; CC0 textures (stage 1b) will be sampled here by world
// position, the models stay as they are.
shader_type spatial;
render_mode cull_back;

instance uniform float build = 1.0;    // 0-1: foundation, walls, roof; the scaffold until 1
instance uniform float age = 0.0;      // 0-1: ten years of weathering
instance uniform float damage = 0.0;   // 0-3: cracks, plaster gone, loose parts tilting
instance uniform float fire = 0.0;     // 0-3: soot and embers, flames, charred
instance uniform float working = 0.0;  // 0/1: the animated parts move
instance uniform float goods = 15.0;   // bits: food, fleece, olive oil, wine the house holds; their props show

const int PALETTE_SIZE = 32;
const vec3 PALETTE[23] = vec3[23](
	vec3(0.93, 0.89, 0.80),  // plaster
	vec3(0.78, 0.74, 0.66),  // stone
	vec3(0.72, 0.36, 0.24),  // roof_tile
	vec3(0.52, 0.36, 0.22),  // wood
	vec3(0.74, 0.60, 0.44),  // mudbrick
	vec3(0.70, 0.60, 0.36),  // thatch
	vec3(0.30, 0.55, 0.72),  // water
	vec3(0.94, 0.93, 0.90),  // marble
	vec3(0.62, 0.45, 0.22),  // bronze
	vec3(0.70, 0.22, 0.18),  // cloth
	vec3(0.32, 0.22, 0.14),  // dark_wood
	vec3(0.86, 0.72, 0.38),  // grain
	vec3(0.66, 0.60, 0.50),  // mortar
	vec3(0.78, 0.48, 0.24),  // ochre
	vec3(0.30, 0.42, 0.56),  // blue_paint
	vec3(0.40, 0.48, 0.26),  // leaves
	vec3(0.80, 0.66, 0.46),  // end_grain
	vec3(0.36, 0.28, 0.20),  // bark
	vec3(0.86, 0.80, 0.66),  // linen
	vec3(0.58, 0.44, 0.30),  // hide
	vec3(0.66, 0.56, 0.38),  // rope
	vec3(0.28, 0.24, 0.08),  // oil
	vec3(0.30, 0.30, 0.32)  // iron
);
const int STONE = 1;
const int WATER = 6;

varying vec4 v_mask;
varying vec3 v_local;
varying float v_hidden;

float hash(vec3 p) {
	p = fract(p * 0.3183099 + vec3(0.71, 0.113, 0.419));
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x), mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
		mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x), mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

void vertex() {
	v_mask = COLOR;
	bool scaffold = COLOR.r > 0.995;
	bool shown = scaffold ? build < 1.0 : build >= COLOR.r - 0.0001 && build > 0.0;
	if (UV2.x < -0.5) {  // a good's prop (UV2.x = -1 - the good): only while the house holds it
		int good = int(round(-UV2.x)) - 1;
		shown = shown && ((int(goods) >> good) & 1) == 1;
	}
	v_hidden = shown ? 0.0 : 1.0;
	if (!shown) {
		VERTEX = vec3(0.0);  // a part not built yet (or the scaffold once done): no area
	}
	if (COLOR.b > 0.5 && damage > 2.5) {
		VERTEX.x += VERTEX.y * 0.25;  // a loose part leans
	}
	if (COLOR.a > 0.75 && working > 0.5) {  // an animated part turns about its pivot (UV2, x and z in tiles)
		vec2 pivot = vec2(UV2.x, 1.0 - UV2.y);  // glTF stores V flipped
		float a = TIME * 0.35;
		vec2 d = VERTEX.xz - pivot;
		VERTEX.xz = pivot + vec2(d.x * cos(a) - d.y * sin(a), d.x * sin(a) + d.y * cos(a));
	}
	v_local = VERTEX;
}

void fragment() {
	if (v_hidden > 0.5) {
		discard;
	}
	int part = clamp(int(UV.x * float(PALETTE_SIZE)), 0, 22);
	vec3 base = pow(PALETTE[part], vec3(2.2));  // the palette is in sRGB, ALBEDO linear
	base *= 0.84 + 0.32 * UV.y;  // no two bricks or tiles alike
	base *= mix(0.72, 1.0, clamp(v_local.y / 0.18, 0.0, 1.0));  // the shade where the walls meet the ground
	float n = noise(v_local * 9.0);
	float plaster = v_mask.g;
	// Age: the colours dull, grime rises from the ground, moss on what faces the sky.
	base = mix(base, base * vec3(0.84, 0.81, 0.76), age * 0.7);
	base *= 1.0 - age * 0.3 * clamp(1.0 - v_local.y / 0.35, 0.0, 1.0);
	float up = clamp(dot(NORMAL, (VIEW_MATRIX * vec4(0.0, 1.0, 0.0, 0.0)).xyz), 0.0, 1.0);
	base = mix(base, pow(vec3(0.42, 0.50, 0.28), vec3(2.2)), age * 0.35 * up * step(0.55, n));
	// Damage: cracks in the plaster, then the plaster falls off and shows the stone.
	if (plaster > 0.5 && damage >= 1.0) {
		float crack = 1.0 - smoothstep(0.0, 0.025 * damage, abs(noise(v_local * 4.0) - 0.5));
		base *= 1.0 - 0.55 * crack;
	}
	if (plaster > 0.5 && damage >= 2.0 && noise(v_local * 5.0) > 0.58) {
		discard;  // the plaster has fallen: the bricks under it show
	}
	// Fire: soot and embers, then charred.
	vec3 glow = vec3(0.0);
	if (fire > 0.5) {
		base *= mix(1.0, 0.45, clamp(fire / 3.0 + 0.2, 0.0, 1.0));
		if (fire < 2.5) {
			glow = vec3(1.0, 0.35, 0.05) * smoothstep(0.7, 0.9, n) * 1.5;
		} else {
			base *= 0.4;
		}
	}
	// Water flows while the fountain works.
	if (part == WATER && working > 0.5) {
		base *= 0.9 + 0.2 * sin(TIME * 3.0 + v_local.x * 9.0 + v_local.z * 7.0);
	}
	ALBEDO = base;
	ROUGHNESS = part == WATER ? 0.15 : 0.9;
	EMISSION = glow;
}
