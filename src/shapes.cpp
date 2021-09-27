/**
  * IceSL-vrprinter, a tool to help simulate and visualize Gcode for 3D printers
  * Copyright (C) 2021  Sylvain Lefebvre    sylvain.lefebvre@inria.fr
  *                     Pierre Bedell       pierre.bedell@gmail.com
  *                     Salim Perchy        salim.perchy@gmail.com
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU Affero General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU Affero General Public License for more details.
  *
  * You should have received a copy of the GNU Affero General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

// ------------------------------------------------------
// Sylvain Lefebvre - 2013-01-29
//-------------------------------------------------------
#include <LibSL/LibSL.h>
#include "shapes.h"

// ------------------------------------------------------

TriangleMesh *shape_box(float sz)
{
  TriangleMesh_generic<t_VertexData> *mesh = new TriangleMesh_generic<t_VertexData>(8,12);

  int t = 0;

  v3f p[8];
  p[0]  = V3F(0,0,0) * sz;
  p[1]  = V3F(1,0,0) * sz;
  p[2]  = V3F(0,1,0) * sz;
  p[3]  = V3F(1,1,0) * sz;
  p[4]  = V3F(0,0,1) * sz;
  p[5]  = V3F(1,0,1) * sz;
  p[6]  = V3F(0,1,1) * sz;
  p[7]  = V3F(1,1,1) * sz;
  ForIndex(i,8) {
    p[i] = p[i] - v3f(sz)/2.0f;
  }

  mesh->vertexAt(0).pos = p[0];
  mesh->vertexAt(1).pos = p[1];
  mesh->vertexAt(2).pos = p[2];
  mesh->vertexAt(3).pos = p[3];
  mesh->vertexAt(4).pos = p[4];
  mesh->vertexAt(5).pos = p[5];
  mesh->vertexAt(6).pos = p[6];
  mesh->vertexAt(7).pos = p[7];

  mesh->triangleAt(t++) = V3U(0, 2, 3);
  mesh->triangleAt(t++) = V3U(0, 3, 1);

  mesh->triangleAt(t++) = V3U(2, 6, 7);
  mesh->triangleAt(t++) = V3U(2, 7, 3);

  mesh->triangleAt(t++) = V3U(0, 4, 6);
  mesh->triangleAt(t++) = V3U(0, 6, 2);

  mesh->triangleAt(t++) = V3U(4, 7, 6);
  mesh->triangleAt(t++) = V3U(4, 5, 7);

  mesh->triangleAt(t++) = V3U(1, 3, 7);
  mesh->triangleAt(t++) = V3U(1, 7, 5);

  mesh->triangleAt(t++) = V3U(0, 1, 5);
  mesh->triangleAt(t++) = V3U(0, 5, 4);

  return mesh;
}

// ------------------------------------------------------

/*
Face layout
     4----7
     | 4) |
4----5----6----7----4
| 0) | 1) | 2) | 3) |
0----1----2----3----0
     | 5) |
     0----3
^ y
|
--> x
*/

TriangleMesh *shape_cube_grid(int Nx,int Ny)
{
  sl_assert(Nx >= 3);
  sl_assert(Ny >= 3);
  std::vector<v3f>     pts;
  Array<Array2D<int> > grid_idx(6);
  ForIndex(f, 4) {
    grid_idx[f].allocate(Nx,Ny);
  }
  grid_idx[4].allocate(Nx, Nx);
  grid_idx[5].allocate(Nx, Nx);
  // corners
  pts.push_back(v3f(0, 0, 0)); // 0
  pts.push_back(v3f(1, 0, 0)); // 1
  pts.push_back(v3f(1, 0, 1)); // 2
  pts.push_back(v3f(0, 0, 1)); // 3
  pts.push_back(v3f(0, 1, 0)); // 4
  pts.push_back(v3f(1, 1, 0)); // 5
  pts.push_back(v3f(1, 1, 1)); // 6
  pts.push_back(v3f(0, 1, 1)); // 7
  grid_idx[0].at(   0,   0) = 0;
  grid_idx[0].at(Nx-1,   0) = 1;
  grid_idx[0].at(Nx-1,Ny-1) = 5;
  grid_idx[0].at(   0,Ny-1) = 4;
  grid_idx[1].at(   0,   0) = 1;
  grid_idx[1].at(Nx-1,   0) = 2;
  grid_idx[1].at(Nx-1,Ny-1) = 6;
  grid_idx[1].at(   0,Ny-1) = 5;
  grid_idx[2].at(   0,   0) = 2;
  grid_idx[2].at(Nx-1,   0) = 3;
  grid_idx[2].at(Nx-1,Ny-1) = 7;
  grid_idx[2].at(   0,Ny-1) = 6;
  grid_idx[3].at(   0,   0) = 3;
  grid_idx[3].at(Nx-1,   0) = 0;
  grid_idx[3].at(Nx-1,Ny-1) = 4;
  grid_idx[3].at(   0,Ny-1) = 7;
  grid_idx[4].at(   0,   0) = 5;
  grid_idx[4].at(Nx-1,   0) = 6;
  grid_idx[4].at(Nx-1,Nx-1) = 7;
  grid_idx[4].at(   0,Nx-1) = 4;
  grid_idx[5].at(   0,   0) = 0;
  grid_idx[5].at(Nx-1,   0) = 3;
  grid_idx[5].at(Nx-1,Nx-1) = 2;
  grid_idx[5].at(   0,Nx-1) = 1;
  // edges
  // 0--1
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[0] + (pts[1] - pts[0]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[0].at(e, 0) = id; // 0 bottom
    grid_idx[5].at(0, e) = id; // 5 left
  }
  // 1--2
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[1] + (pts[2] - pts[1]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[1].at(e, 0) = id; // 1 bottom
    grid_idx[5].at(e, Nx-1) = id; // 5 top
  }
  // 2--3
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[2] + (pts[3] - pts[2]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[2].at(e, 0) = id; // 2 bottom
    grid_idx[5].at(Nx-1, Nx-1-e) = id; // 5 right
  }
  // 3--0
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[3] + (pts[0] - pts[3]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[3].at(e, 0) = id; // 3 bottom
    grid_idx[5].at(Nx-1-e,0) = id; // 5 bottom
  }
  // 4--5
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[4] + (pts[5] - pts[4]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[0].at(e, Ny - 1) = id; // 0 top
    grid_idx[4].at(0, Nx-1-e) = id; // 4 left
  }
  // 5--6
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[5] + (pts[6] - pts[5]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[1].at(e, Ny - 1) = id; // 1 top
    grid_idx[4].at(e, 0) = id; // 4 bottom
  }
  // 6--7
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[6] + (pts[7] - pts[6]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[2].at(e, Ny - 1) = id; // 2 top
    grid_idx[4].at(Nx - 1,e) = id; // 4 right
  }
  // 7--4
  ForRange(e, 1, Nx - 2) {
    v3f pt = pts[7] + (pts[4] - pts[7]) * (float)e / (float)(Nx-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[3].at(e, Ny - 1) = id; // 3 top
    grid_idx[4].at(Nx-1-e,Nx-1) = id; // 4 top
  }
  // 0--4
  ForRange(e, 1, Ny - 2) {
    v3f pt = pts[0] + (pts[4] - pts[0]) * (float)e / (float)(Ny-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[3].at(Nx - 1, e) = id; // 3 right
    grid_idx[0].at(0, e) = id; // 0 left
  }
  // 1--5
  ForRange(e, 1, Ny - 2) {
    v3f pt = pts[1] + (pts[5] - pts[1]) * (float)e / (float)(Ny-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[0].at(Nx-1, e) = id; // 0 right
    grid_idx[1].at(0, e) = id; // 1 left
  }
  // 2--6
  ForRange(e, 1, Ny - 2) {
    v3f pt = pts[2] + (pts[6] - pts[2]) * (float)e / (float)(Ny-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[1].at(Nx - 1, e) = id; // 1 right
    grid_idx[2].at(0, e) = id; // 2 left
  }
  // 3--7
  ForRange(e, 1, Ny - 2) {
    v3f pt = pts[3] + (pts[7] - pts[3]) * (float)e / (float)(Ny-1);
		int id = (int)pts.size(); pts.push_back(pt);
    grid_idx[2].at(Nx - 1, e) = id; // 2 right
    grid_idx[3].at(0, e) = id; // 3 left
  }
  // grid centers
  ForIndex(f, 6) {
    int Ni = Nx, Nj = Ny;
    if (f >= 4) {
      Nj = Nx;
    }
    ForRange(j, 1, Nj - 2) { ForRange(i, 1, Ni - 2) {
      float  s = (float)i / (float)(Ni-1);
      float  t = (float)j / (float)(Nj-1);
      v3f   pt =
          (1 - s)*(1 - t)*pts[grid_idx[f].at(0,0)]
        + (    s)*(1 - t)*pts[grid_idx[f].at(Ni-1,0)]
        + (1 - s)*(    t)*pts[grid_idx[f].at(0,Nj-1)]
        + (    s)*(    t)*pts[grid_idx[f].at(Ni-1,Nj-1)];
			int id = (int)pts.size(); pts.push_back(pt);
      grid_idx[f].at(i, j) = id;
    } }
  }
  // create mesh  
  TriangleMesh_generic<t_VertexData> *mesh = new TriangleMesh_generic<t_VertexData>(
    8 /*corners*/ + 8 * (Nx - 2) + 4 * (Ny - 2) /*edges*/ + 4 * (Nx - 2)*(Ny - 2) + 2 * (Nx - 2) * (Nx - 2) /*cores*/,
    4 * 2 * (Nx - 1)*(Ny - 1) + 2 * 2 * (Nx - 1)*(Nx - 1));
  sl_assert(pts.size() == mesh->numVertices());
  ForIndex(v, pts.size()) {
    mesh->vertexAt(v).pos = pts[v];
  }
  // triangles
  uint t = 0;
  ForIndex(f, 6) {
    int Ni = Nx, Nj = Ny;
    if (f >= 4) {
      Nj = Nx;
    }
    ForIndex(tj, Nj-1) { ForIndex(ti, Ni-1) {
      v3u ta(grid_idx[f].at(ti, tj), grid_idx[f].at(ti + 1, tj + 1), grid_idx[f].at(ti + 1, tj));
      v3u tb(grid_idx[f].at(ti, tj), grid_idx[f].at(ti, tj + 1), grid_idx[f].at(ti + 1, tj + 1));
      mesh->triangleAt(t++) = ta;
      mesh->triangleAt(t++) = tb;
    } }
  }
  sl_assert(t == mesh->numTriangles());
  return mesh;
}

// ------------------------------------------------------

TriangleMesh *shape_cylinder(float rb, float rt, float h, int N)
{
  N = N + ((N & 1) ? 0 : 1); // make odd
  TriangleMesh *cube = shape_cube_grid(N,3);
  v3f ctr(0.5f);
  ForIndex(v, cube->numVertices()) {
    t_VertexData *vtx = static_cast<t_VertexData*>(cube->vertexDataAt(v));
    // swap y-z
    std::swap(vtx->pos[1], vtx->pos[2]);
    vtx->pos[0] = 1.0f-vtx->pos[0];
    // map to cylinder
    float rh = rb + (rt - rb) * vtx->pos[2];
    float ring = 1.0f;
    v2f pt = v2f(vtx->pos) - v2f(0.5f);
    ring  = 2.0f * tupleMax(tupleAbs(pt));
    pt    = rh * ring * normalize_safe(pt);
    vtx->pos[0] = pt[0];
    vtx->pos[1] = pt[1];
    vtx->pos[2] = vtx->pos[2] * h;
  }
  return cube;
}

// ------------------------------------------------------

TriangleMesh *shape_sphere(float rd, int N)
{
  N = N + ((N & 1)?0:1); // make odd
  TriangleMesh *cube = shape_cube_grid(N, N);
  v3f ctr(0.5f);
  ForIndex(v, cube->numVertices()) {
    t_VertexData *vtx = static_cast<t_VertexData*>(cube->vertexDataAt(v));
    vtx->pos = rd * normalize(vtx->pos - ctr);
  }
  return cube;
}

// ------------------------------------------------------
