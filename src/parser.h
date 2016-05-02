//
//  parser.h
//  pipeline
//
//  Created by ChuMingChing on 4/12/16.
//  Copyright © 2016 ChuMingChing. All rights reserved.
//

#ifndef parser_h
#define parser_h

#include <iostream>
#include <vector>
#include "bezier_surface.h"
using namespace std;

// Returns true if file is OBJ
bool checkIfOBJFileType (const char *file);

void read_wavefront_file (const char *file, vector<int> &tris, vector<float> &verts);
void read_bezier_file(const char* file, vector<bezier_surf> &s);

class bezier_surf;

#endif /* parser_h */
