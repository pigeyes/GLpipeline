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
using namespace std;

void read_wavefront_file (const char *file, vector<int> &tris, vector<float> &verts);

#endif /* parser_h */
