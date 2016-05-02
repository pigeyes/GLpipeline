attribute vec4 vPosition;
attribute vec4 vNorm;
// in vec4 vPosition;
// in vec4 vNorm;

uniform mat4 ctm;
uniform mat4 ptm;

// color, sned to fshader
varying vec4 norm;
varying vec4 v_light;
varying vec4 v_viewer;
varying vec4 vnorm;

void main()
{
	vnorm = vNorm;
	
	gl_Position = ptm * ctm * vPosition;
}
