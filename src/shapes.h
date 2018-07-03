// ------------------------------------------------------
// Sylvain Lefebvre - 2013-01-29
//-------------------------------------------------------

#pragma once

#include <LibSL/LibSL.h>

typedef struct
{
  LibSL::Math::v3f pos;
} t_VertexData;

typedef MVF1(mvf_position_3f) t_VertexFormat;

TriangleMesh *shape_box(float sz);
TriangleMesh *shape_cube_grid(int Nx,int Ny);
TriangleMesh *shape_cylinder(float rb, float rt, float h, int N_radius = 128);
TriangleMesh *shape_sphere(float sz, int N = 128);
