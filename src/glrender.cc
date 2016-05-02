// Very simple display triangle program, that allows you to rotate the
// triangle around the Y axis.
//
// This program does NOT use a vertex shader to define the vertex colors.
// Instead, it computes the colors in the display callback (using Blinn/Phong)
// and passes those color values, one per vertex, to the vertex shader, which
// passes them directly to the fragment shader. This achieves what is called
// "gouraud shading".

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#endif

#include <vector>
#include "amath.h"
#include "parser.h"

using namespace std;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

GLuint buffers[2];

// camera position
point4 eye;
const point4 viewer = vec4(0.0, 0.0, 0.0, 0.0);
vec4 up;

float theta = 0.0;  // rotation around the Y (up) axis
float phi = 90.0; // rotation around the X axis
float r = 5.0; // position on Z axis
float posx = 0.0;   // translation along X
float posy = 0.0;   // translation along Y
// X axis constraint
const float RMAX = 50.0;
const float RMIN = 2.0;
// Y axis constraint
const float ZENITH = 175.0;
const float NADIR = 5.0;

// Bezier limits
bool bezier_changed = false; // If vertices changed
bool bezier_mode = false;
const int MIN_DETAIL = 2;
const int MAX_DETAIL = 20;
unsigned int bezier_coarseness = 2; // Number of samples per degree

vector<bezier_surf> surfaces;

int NumVertices;
point4 *vertices = NULL;
vec4 *norms = NULL;

GLint view_pos, ctm, ptm;

vec4 light_position = vec4(100., 100., 100., 1.0);
vec4 light_ambient  = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

vec4 material_ambient  = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse  = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 0.8, 0.0, 1.0);
// material_shin = 100.0
GLint light_pos, light_spec, light_ambi, light_diff, material_ambi, material_diff, material_spec, material_shin;

GLuint program;

void loadOBJ(const char *file_name)
{
	vector <int> tris;
	vector <float> verts;
	read_wavefront_file(file_name, tris, verts);
	
	NumVertices = (int) tris.size();
	vertices = new point4[NumVertices];
	norms = new vec4[NumVertices];
	
	vec4 *vert_norms = new vec4[verts.size()/3];
	
	// initialize to zeros
	for (unsigned int i = 0; i < verts.size()/3; i++)
		vert_norms[i] = vec4(0.0);
	
	/* Add all normals for all vertices. Shared */
	for (unsigned int i = 0; i < NumVertices; i+=3) {
		int p1_i = tris[i];
		int p2_i = tris[i+1];
		int p3_i = tris[i+2];
		vertices[i] = point4(verts[3*p1_i], verts[3*p1_i+1], verts[3*p1_i+2], 1.0);
		vertices[i+1] = point4(verts[3*p2_i], verts[3*p2_i+1], verts[3*p2_i+2], 1.0);
		vertices[i+2] = point4(verts[3*p3_i], verts[3*p3_i+1], verts[3*p3_i+2], 1.0);
		
		vec4 norm = normalize(vec4(cross(vertices[i+1]-vertices[i], vertices[i+2]-vertices[i]), 0.0));
		vert_norms[p1_i] += norm;
		vert_norms[p2_i] += norm;
		vert_norms[p3_i] += norm;
	}
	
	for (unsigned int i = 0; i < verts.size()/3; i++)
		vert_norms[i] = normalize(vert_norms[i]);
	
	for (unsigned int i = 0; i < NumVertices; i+=3) {
		int p1_i = tris[i];
		int p2_i = tris[i+1];
		int p3_i = tris[i+2];
		norms[i] = vert_norms[p1_i];
		norms[i+1]= vert_norms[p2_i];
		norms[i+2] = vert_norms[p3_i];
	}
	delete[] vert_norms;
}

void loadBezierVertsAndNorms() {
	int n_verts = 0;
	for (int i = 0; i < surfaces.size(); ++i) {
		n_verts += 3 * ((2*surfaces[i].getUSamples(bezier_coarseness) - 2) *
						(surfaces[i].getVSamples(bezier_coarseness) - 1)  );
	}
	NumVertices = n_verts;
	
	delete vertices;
	delete norms;
	vertices = new point4[NumVertices];
	norms = new vec4[NumVertices];
	
	vector<point4> v_verts;
	vector<vec4> v_norms;
	for (int i = 0; i < surfaces.size(); ++i)
		surfaces[i].sample(bezier_coarseness, v_verts, v_norms);
	
	// Triangulate
	int to_skip = 0;
	int vPos = 0;
	for (int i = 0; i < surfaces.size(); ++i) {
		int u_sam = surfaces[i].getUSamples(bezier_coarseness);
		int v_sam = surfaces[i].getVSamples(bezier_coarseness);
		
		int verts_surf = u_sam*v_sam;
		for (int v = 0; v < v_sam-1; v++)
			for (int u = 0; u < u_sam-1; u++) {
				vec4 tri1_1 = v_verts[to_skip + v*u_sam + u];
				vec4 norm1_1 = v_norms[to_skip + v*u_sam + u];
				vec4 tri1_2 = v_verts[to_skip + (v+1)*u_sam + u+1];
				vec4 norm1_2 = v_norms[to_skip + (v+1)*u_sam + u+1];
				vec4 tri1_3 = v_verts[to_skip + (v+1)*u_sam + u];
				vec4 norm1_3 = v_norms[to_skip + (v+1)*u_sam + u];
				
				vec4 tri2_1 = tri1_2;
				vec4 norm2_1 = norm1_2;
				vec4 tri2_2 = tri1_1;
				vec4 norm2_2 = norm1_1;
				vec4 tri2_3 = v_verts[to_skip + v*u_sam + u+1];
				vec4 norm2_3 = v_norms[to_skip + v*u_sam + u+1];
				
				vertices[vPos] = tri1_1;
				vertices[vPos+1] = tri1_2;
				vertices[vPos+2] = tri1_3;
				vertices[vPos+3] = tri2_1;
				vertices[vPos+4] = tri2_2;
				vertices[vPos+5] = tri2_3;
				
				norms[vPos] = norm1_1;
				norms[vPos+1] = norm1_2;
				norms[vPos+2] = norm1_3;
				norms[vPos+3] = norm2_1;
				norms[vPos+4] = norm2_2;
				norms[vPos+5] = norm2_3;
				
				vPos += 6;
			}
		to_skip += verts_surf;
	}
	
}

void loadBezier(const char *file_name) {
	read_bezier_file(file_name, surfaces);
	bezier_mode = true;
	loadBezierVertsAndNorms();
}

// initialization: set up a Vertex Array Object (VAO) and then
void init()
{
    
    // create a vertex array object - this defines mameory that is stored
    // directly on the GPU
    GLuint vao;
    
    // deending on which version of the mac OS you have, you may have to do this:
#ifdef __APPLE__
    glGenVertexArraysAPPLE( 1, &vao );  // give us 1 VAO:
    glBindVertexArrayAPPLE( vao );      // make it active
#else
    glGenVertexArrays( 1, &vao );   // give us 1 VAO:
    glBindVertexArray( vao );       // make it active
#endif
    
    // set up vertex buffer object - this will be memory on the GPU where
    // we are going to store our vertex data (that is currently in the "points"
    // array)
    glGenBuffers(1, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  // make it active
    
    // specify that its part of a VAO, what its size is, and where the
    // data is located, and finally a "hint" about how we are going to use
    // the data (the driver will put it in a good memory location, hopefully)
    glBufferData(GL_ARRAY_BUFFER, 2*sizeof(vec4)*NumVertices, NULL, GL_STATIC_DRAW);
    
    // load in these two shaders...  (note: InitShader is defined in the
    // accompanying initshader.c code).
    // the shaders themselves must be text glsl files in the same directory
    // as we are running this program:
    program = InitShader("vshader_blinnphong.glsl", "fshader_passthrough.glsl");
 
    // ...and set them to be active
    glUseProgram(program);
    
    
    // this time, we are sending TWO attributes through: the position of each
    // transformed vertex, and the color we have calculated in tri().
    GLuint loc, loc2;
    
    loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    
    // the vPosition attribute is a series of 4-vecs of floats, starting at the
    // beginning of the buffer
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    loc2 = glGetAttribLocation(program, "vNorm");
    glEnableVertexAttribArray(loc2);

    // the vColor attribute is a series of 4-vecs of floats, starting just after
    // the points in the buffer
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vec4)*NumVertices));
	
	// set uniform values
	view_pos = glGetUniformLocation(program, "view_pos");
	ctm = glGetUniformLocation(program, "ctm");
	ptm = glGetUniformLocation(program, "ptm");
	
	// pass light/material parameters
	light_spec = glGetUniformLocation(program, "light_spec");
	light_pos = glGetUniformLocation(program, "light_pos");
	light_ambi = glGetUniformLocation(program, "light_ambi");
	light_diff = glGetUniformLocation(program, "light_diff");
	material_ambi = glGetUniformLocation(program, "material_ambi");
	material_diff = glGetUniformLocation(program, "material_diff");
	material_spec = glGetUniformLocation(program, "material_spec");
	material_shin = glGetUniformLocation(program, "material_shin");
    
    // set the background color (white)
    glClearColor(1.0, 1.0, 1.0, 1.0); 
}



void display( void )
{
 
    // clear the window (with white) and clear the z-buffer (which isn't used
    // for this example).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	
	GLfloat p = DegreesToRadians*phi;
	GLfloat t = DegreesToRadians*theta;
	eye = point4(r*sin(p)*sin(t), r*cos(p), r*sin(p)*cos(t), 1.0);
	up = normalize(vec4(cross(normalize(cross(normalize(viewer - eye), vec4(0, 1, 0, 0))), normalize(viewer - eye)), 0.0));
	
	glUniform4fv(view_pos, 1, eye);
	
	glUniform4fv(light_pos, 1, light_position);
	glUniform4fv(light_spec, 1, light_specular);
	glUniform4fv(light_ambi, 1, light_ambient);
	glUniform4fv(light_diff, 1, light_diffuse);
	glUniform4fv(material_ambi, 1, material_ambient);
	glUniform4fv(material_diff, 1, material_diffuse);
	glUniform4fv(material_spec, 1, material_specular);
	glUniform1f(material_shin, 100.0);
	
	glUniformMatrix4fv(ctm, 1, GL_TRUE, LookAt(eye, viewer, up));
	glUniformMatrix4fv(ptm, 1, GL_TRUE, Perspective(40, 1.0, 1, 50));
	
	glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(vec4)*NumVertices, vertices );
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(vec4)*NumVertices, sizeof(vec4)*NumVertices, norms );
	
	if (bezier_mode && bezier_changed) {
		loadBezierVertsAndNorms();
		glBufferData(GL_ARRAY_BUFFER, 2*sizeof(vec4)*NumVertices, NULL, GL_STATIC_DRAW);
		
		GLuint loc, loc2;
		loc = glGetAttribLocation(program, "vPosition");
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
		
		loc2 = glGetAttribLocation(program, "vNorm");
		glEnableVertexAttribArray(loc2);
		glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vec4)*NumVertices));
		
		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(vec4)*NumVertices, vertices );
		glBufferSubData( GL_ARRAY_BUFFER, sizeof(vec4)*NumVertices, sizeof(vec4)*NumVertices, norms );
	}
	bezier_changed = false;
	
    // draw the VAO:
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	
    // move the buffer we drew into to the screen, and give us access to the one
    // that was there before:
    glutSwapBuffers();
}


// use this motionfunc to demonstrate rotation - it adjusts "theta" based
// on how the mouse has moved. Theta is then used the the display callback
// to generate the transformation, ctm, that is applied
// to all the vertices before they are displayed:
void mouse_move_rotate (int x, int y)
{
    
    static int lastx = 0;// keep track of where the mouse was last:
	static int lasty = 0;
    
    int amntX = x - lastx; 
    if (amntX != 0) {
        theta +=  amntX;
        if (theta > 360.0 ) theta -= 360.0;
        if (theta < 0.0 ) theta += 360.0;
        
        lastx = x; //update lastx
    }
	
	int amntY = y - lasty;
	if (amntY != 0) {
		phi += amntY;
		if (phi > ZENITH) phi = ZENITH;
		if (phi < NADIR) phi = NADIR;
		
		lasty = y;
	}

    // force the display routine to be called as soon as possible:
    glutPostRedisplay();
    
}


// use this motionfunc to demonstrate translation - it adjusts posx and
// posy based on the mouse movement. posx and posy are then used in the
// display callback to generate the transformation, ctm, that is applied
// to all the vertices before they are displayed:
void mouse_move_translate (int x, int y)
{
    
    static int lastx = 0;
    static int lasty = 0;  // keep track of where the mouse was last:

    if (x - lastx < 0) --posx;
    else if (x - lastx > 0) ++posx;
    lastx = x;

    if (y - lasty > 0) --posy;
    else if (y - lasty < 0) ++posy;
    lasty = y;
    
    // force the display routine to be called as soon as possible:
    glutPostRedisplay();
    
}


// the keyboard callback, called whenever the user types something with the
// regular keys.
void mykey(unsigned char key, int mousex, int mousey)
{
	if(key=='q'|| key=='Q') {
		delete vertices;
		delete norms;
		exit(0);
	}
	
	// r resets the view:
	if (key =='r') {
		theta = 0;
		phi = 90.0;
		r = 5.0;
		glutPostRedisplay();
	}
	
	// z moves the view closer
	if (key == 'z' && r > RMIN) {
		r *= 0.9;
		glutPostRedisplay();
	}
	
	// x moves the view farther
	if (key == 'x' && r < RMAX) {
		r *= 1.1;
		glutPostRedisplay();
	}
	
	// < decreases detail
	if (key == '<' && bezier_coarseness > MIN_DETAIL) {
		bezier_coarseness--;
		bezier_changed = true;
		glutPostRedisplay();
	}
	
	// > increases detail
	if (key == '>' && bezier_coarseness < MAX_DETAIL) {
		bezier_coarseness++;
		bezier_changed = true;
		glutPostRedisplay();
	}
}


int main(int argc, char** argv)
{
	if (checkIfOBJFileType(argv[1]))
		loadOBJ(argv[1]);
	else
		loadBezier(argv[1]);
	
	// std::cout << sizeof(points[0]) << ", " << sizeof(points) << endl;

    // initialize glut, and set the display modes
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    
    // give us a window in which to display, and set its title:
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rotate OBJ File");
    
    // for displaying things, here is the callback specification:
    glutDisplayFunc(display);
    
    // when the mouse is moved, call this function!
    // you can change this to mouse_move_translate to see how it works
    glutMotionFunc(mouse_move_rotate);
 
    // for any keyboard activity, here is the callback:
    glutKeyboardFunc(mykey);
    
#ifndef __APPLE__
    // initialize the extension manager: sometimes needed, sometimes not!
    glewInit();
#endif

    // call the init() function, defined above:
    init();
    
    // enable the z-buffer for hidden surface removel:
    glEnable(GL_DEPTH_TEST);

    // once we call this, we no longer have control except through the callbacks:
    glutMainLoop();
    return 0;
}
