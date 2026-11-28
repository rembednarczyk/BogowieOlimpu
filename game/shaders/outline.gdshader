// The ink line along the city's shapes (owner, 2026-11-28: a trial, R-018 "a clear outline"; off by default).
// A full-screen pass over the camera: where the depth jumps between neighbouring pixels by more than
// `threshold` (world units, i.e. tiles), a line in `ink` is drawn. Silhouettes, eaves over walls, roofs against
// the ground and walkers get it; the joints between bricks and tiles do not (their depth barely changes).
// Presentation only, read from the depth the frame already has.
shader_type spatial;
render_mode unshaded, depth_draw_never, depth_test_disabled, cull_disabled, blend_mix, fog_disabled, shadows_disabled;

uniform sampler2D depth_tex : hint_depth_texture, filter_nearest;
uniform float width = 0.0;            // 0 off, 1 thin, 2 strong (pixels to the neighbours)
uniform float threshold = 0.18;       // a jump in depth that makes an edge (tiles; 0.18 = 36 cm)
uniform vec3 ink : source_color = vec3(0.22, 0.14, 0.09);  // warm brown, not black: a painted line
uniform float opacity = 0.8;

void vertex() {
	POSITION = vec4(VERTEX.xy, 1.0, 1.0);  // the quad covers the screen
}

float view_depth(vec2 uv, mat4 inv_projection) {
	float d = texture(depth_tex, uv).r;
	vec4 v = inv_projection * vec4(uv * 2.0 - 1.0, d, 1.0);
	return -v.z / v.w;
}

void fragment() {
	if (width < 0.5) {
		discard;
	}
	vec2 px = width / VIEWPORT_SIZE;
	float c = view_depth(SCREEN_UV, INV_PROJECTION_MATRIX);
	float e = 0.0;
	e = max(e, abs(view_depth(SCREEN_UV + vec2(px.x, 0.0), INV_PROJECTION_MATRIX) - c));
	e = max(e, abs(view_depth(SCREEN_UV - vec2(px.x, 0.0), INV_PROJECTION_MATRIX) - c));
	e = max(e, abs(view_depth(SCREEN_UV + vec2(0.0, px.y), INV_PROJECTION_MATRIX) - c));
	e = max(e, abs(view_depth(SCREEN_UV - vec2(0.0, px.y), INV_PROJECTION_MATRIX) - c));
	float edge = smoothstep(threshold, threshold * 2.0, e);
	if (edge < 0.02) {
		discard;
	}
	ALBEDO = ink;
	ALPHA = edge * opacity;
}
