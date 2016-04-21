attribute vec4 vPosition;
attribute vec4 vNorm;

uniform mat4 ctm;
uniform mat4 ptm;
// camera position
uniform vec4 view_pos;

uniform vec4 light_pos;
uniform vec4 light_ambi;
uniform vec4 light_diff;
uniform vec4 light_spec;

uniform vec4 material_ambi;
uniform vec4 material_diff;
uniform vec4 material_spec;
uniform float material_shin;

// color, sned to fshader
varying vec4 norm;

void main()
{
	// the direction from vertex to the light source and to the viewer
	vec4 v_light = normalize(light_pos - vPosition);
	vec4 v_viewer = normalize(view_pos - vPosition);
	
	// for ambient:
	norm = light_ambi * material_ambi;
	
	// for diffuse:
	float dd = max (0.0, dot(v_light, vNorm));
	norm += dd * (light_diff * material_diff);

	// for specular:
	float sd = 0.0;
	if ((dot(v_light, vNorm) > 0.0) && (dot(v_viewer, vNorm) > 0.0)) {
		sd = max(dot(normalize(v_light + v_viewer), vNorm), 0.0);
	}
	if (sd > 0.0) {
		sd = pow(sd, material_shin);
	}
	norm += sd * (light_spec * material_spec);
	
	norm.a = 1.0;
	
	gl_Position = ptm * ctm * vPosition;
}
