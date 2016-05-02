// the simplest fragment shader: get the color (from the vertex shader - 
// actually interpolated from values specified in the vertex shader - and
// just pass them through to the render:
// on mac: 
//varying vec4 norm;
varying vec4 norm;
varying vec4 v_light;
varying vec4 v_viewer;
varying vec4 vnorm;

uniform vec4 light_pos;
uniform vec4 light_ambi;
uniform vec4 light_diff;
uniform vec4 light_spec;

uniform vec4 material_ambi;
uniform vec4 material_diff;
uniform vec4 material_spec;
uniform float material_shin;

void main() 
{
	// for ambient:
	norm = light_ambi * material_ambi;
	
	// for diffuse:
	float dd = max (0.0, dot(v_light, vnorm));
	norm += dd * (light_diff * material_diff);
	
	// for specular:
	float sd = 0.0;
	if ((dot(v_light, vnorm) > 0.0) && (dot(v_viewer, vnorm) > 0.0)) {
		sd = max(dot(normalize(v_light + v_viewer), vnorm), 0.0);
	}
	if (sd > 0.0) {
		sd = pow(sd, material_shin);
	}
	norm += sd * (light_spec * material_spec);
	
	norm.a = 1.0;
	
	gl_FragColor = norm;
} 

