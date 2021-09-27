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
