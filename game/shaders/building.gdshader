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

const int PALETTE_SIZE = 64;  // the UV slots (tools/blender/bo_asset.py); PALETTE holds the colours in use
const vec3 PALETTE[34] = vec3[34](
	vec3(0.96, 0.91, 0.80),  // plaster
	vec3(0.82, 0.77, 0.66),  // stone
	vec3(0.82, 0.40, 0.26),  // roof_tile
	vec3(0.60, 0.42, 0.26),  // wood
	vec3(0.80, 0.64, 0.46),  // mudbrick
	vec3(0.80, 0.68, 0.40),  // thatch
	vec3(0.32, 0.62, 0.78),  // water
	vec3(0.95, 0.94, 0.90),  // marble
	vec3(0.66, 0.48, 0.24),  // bronze
	vec3(0.78, 0.26, 0.20),  // cloth
	vec3(0.36, 0.25, 0.16),  // dark_wood
	vec3(0.90, 0.76, 0.40),  // grain
	vec3(0.70, 0.63, 0.52),  // mortar
	vec3(0.88, 0.56, 0.26),  // ochre
	vec3(0.22, 0.52, 0.56),  // blue_paint
	vec3(0.38, 0.58, 0.26),  // leaves
	vec3(0.84, 0.70, 0.48),  // end_grain
	vec3(0.40, 0.31, 0.22),  // bark
	vec3(0.90, 0.85, 0.72),  // linen
	vec3(0.62, 0.47, 0.32),  // hide
	vec3(0.70, 0.60, 0.40),  // rope
	vec3(0.30, 0.26, 0.08),  // oil
	vec3(0.30, 0.30, 0.34),  // iron
	vec3(0.50, 0.70, 0.30),  // grass
	vec3(0.90, 0.30, 0.34),  // flower_red
	vec3(0.98, 0.82, 0.30),  // flower_yellow
	vec3(0.97, 0.95, 0.90),  // flower_white
	vec3(0.56, 0.72, 0.88),  // sky_cloth
	vec3(0.40, 0.52, 0.68),  // roof_slate
	vec3(0.90, 0.70, 0.30),  // roof_gold
	vec3(0.58, 0.36, 0.22),  // roof_brown
	vec3(0.64, 0.70, 0.52),  // sage_cloth
	vec3(0.26, 0.50, 0.46),  // roof_patina
	vec3(0.68, 0.71, 0.80)  // stone_cool
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
	if (damage > 2.5 && !scaffold) {  // near collapse: the whole house settles askew, its loose parts lean far
		VERTEX.x += VERTEX.y * (COLOR.b > 0.5 ? 0.25 : 0.035);
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
	int part = clamp(int(UV.x * float(PALETTE_SIZE)), 0, 33);
	vec3 base = pow(PALETTE[part], vec3(2.2));  // the palette is in sRGB, ALBEDO linear
	base *= 0.84 + 0.32 * UV.y;  // no two bricks or tiles alike
	base *= mix(0.72, 1.0, clamp(v_local.y / 0.18, 0.0, 1.0));  // the shade where the walls meet the ground
	float n = noise(v_local * 9.0);
	float plaster = v_mask.g;
	// What the part is (the palette's order, tools/blender/bo_asset.py).
	bool tile = part == 2 || part == 28 || part == 29 || part == 30 || part == 32;  // roof tiles of any colour
	bool soft = part == 5 || part == 9 || part == 18 || part == 19 || part == 27 || part == 31;  // thatch, cloth, hides
	bool masonry = plaster > 0.5 || part == 1 || part == 4 || part == 7 || part == 12 || part == 33;
	float piece = fract(UV.y * 97.13);  // each brick, tile or board its own number (its tint)
	// Age: the colours dull, grime rises from the ground, rain streaks down the walls, moss on what faces the sky.
	base = mix(base, base * vec3(0.84, 0.81, 0.76), age * 0.7);
	base *= 1.0 - age * 0.3 * clamp(1.0 - v_local.y / 0.35, 0.0, 1.0);
	if (masonry) {
		base *= 1.0 - age * 0.22 * smoothstep(0.62, 0.9, noise(vec3(v_local.x * 11.0, v_local.y * 0.7, v_local.z * 11.0)));
	}
	float up = clamp(dot(NORMAL, (VIEW_MATRIX * vec4(0.0, 1.0, 0.0, 0.0)).xyz), 0.0, 1.0);
	base = mix(base, pow(vec3(0.42, 0.50, 0.28), vec3(2.2)), age * (tile ? 0.5 : 0.35) * up * step(0.5, n));
	// Damage (from the collapse counter): 1 cracks through the walls; 2 plaster falls off and shows the bricks,
	// roof tiles slip off, cloth tears; 3 more of it all, and the house settles askew (vertex()).
	if (damage >= 1.0 && masonry) {
		float c1 = abs(noise(v_local * vec3(3.0, 1.6, 3.0)) - 0.5);
		float c2 = abs(noise(v_local * vec3(7.0, 3.5, 7.0) + 3.1) - 0.5) * 1.4;
		float crack = 1.0 - smoothstep(0.0, 0.01 + 0.012 * damage, min(c1, c2));
		base *= 1.0 - 0.75 * crack;
	}
	if (plaster > 0.5 && damage >= 2.0 && noise(v_local * 5.0) > 0.64 - 0.08 * (damage - 2.0)) {
		discard;  // the plaster has fallen: the bricks under it show
	}
	if (tile && damage >= 2.0 && UV.y > 0.15 && piece < 0.1 + 0.15 * (damage - 2.0)) {
		discard;  // a tile slipped off: the bed shows
	}
	if (soft && damage >= 2.0 && noise(v_local * 6.0) > 0.72 - 0.06 * (damage - 2.0)) {
		discard;  // a tear in the cloth or the thatch
	}
	// Fire (32 days): 1 it smoulders: smoke stains, embers flickering low; 2 flames climb the walls and the roof;
	// 3 charred: black, the thatch and the cloth burned away, half the tiles fallen, embers in the ruin.
	vec3 glow = vec3(0.0);
	if (fire > 0.5) {
		float hgt = clamp(v_local.y / 1.6, 0.0, 1.0);
		base *= mix(1.0, 0.28, clamp(fire / 3.0 + 0.15, 0.0, 1.0) * (0.5 + 0.5 * n) * (0.6 + 0.4 * hgt));
		if (fire < 1.5) {
			float e = smoothstep(0.78, 0.92, noise(v_local * 8.0 + vec3(0.0, TIME * 0.4, 0.0)));
			glow = vec3(1.0, 0.32, 0.05) * e * 1.8 * (1.0 - hgt * 0.7);
		} else if (fire < 2.5) {
			float f = noise(vec3(v_local.x * 5.0, v_local.y * 3.0 - TIME * 2.2, v_local.z * 5.0));
			float tongue = smoothstep(0.42, 0.8, f) * (0.65 + 0.35 * sin(TIME * 9.0 + v_local.x * 13.0 + v_local.z * 7.0));
			glow = mix(vec3(0.9, 0.18, 0.02), vec3(1.0, 0.5, 0.1), f) * tongue * ((soft || tile) ? 1.7 : 1.1);
		} else {
			base *= 0.35;
			if (soft && noise(v_local * 4.0) > 0.22) {
				discard;
			}
			if (tile && UV.y > 0.15 && piece < 0.45) {
				discard;
			}
			glow = vec3(0.9, 0.2, 0.03) * smoothstep(0.86, 0.95, n) * 0.6 * (0.6 + 0.4 * sin(TIME * 2.0 + n * 20.0));
		}
	}
	// Water flows while the fountain works.
	if (part == WATER && working > 0.5) {
		base *= 0.9 + 0.2 * sin(TIME * 3.0 + v_local.x * 9.0 + v_local.z * 7.0);
	}
	// A painted look (owner, 2026-11-28, "a bit of Ghibli"): colour pools slightly in broad soft patches, as
	// brushwork would; light() then lays the light in soft bands with cool, bluish shade.
	base *= 0.94 + 0.12 * noise(v_local * 1.3);
	ALBEDO = base;
	ROUGHNESS = part == WATER ? 0.15 : 0.9;
	EMISSION = glow;
}

// Soft two-tone light: a wide smooth step between lit and shaded, the shade tinted to the sky (cool) and the lit
// side warmed, a thin warm rim where a face turns from the view. The shadows of the light still apply.
void light() {
	float ndl = dot(NORMAL, LIGHT);
	float lit = smoothstep(-0.05, 0.35, ndl) * ATTENUATION;
	vec3 shade = vec3(0.50, 0.56, 0.74);
	vec3 sun = vec3(1.04, 0.98, 0.88);
	DIFFUSE_LIGHT += LIGHT_COLOR / PI * mix(shade * 0.55, sun, lit);
	float rim = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 4.0) * lit;
	DIFFUSE_LIGHT += LIGHT_COLOR / PI * rim * vec3(0.30, 0.22, 0.12);
}
