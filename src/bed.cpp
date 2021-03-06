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

#include "bed.h"

GLShader    g_ShaderBed;
GLParameter g_bed_view;
GLParameter g_bed_mdl;
GLParameter g_bed_tex_scale;

typedef GPUMESH_MVF1(mvf_vertex_3f)      mvf_mesh;
typedef GPUMesh_VertexBuffer<mvf_mesh>   SimpleMesh;

AutoPtr<SimpleMesh>                      g_bed_quad;

void bed_init()
{
  g_ShaderBed.init(
    // VP
  #ifndef EMSCRIPTEN
    "#version 430 core\n"
  #else
    "#version 300 es\n"
    "precision mediump float;\n"
  #endif
    "in vec4 mvf_vertex;\n"
    "uniform mat4 u_view;\n"
    "uniform mat4 u_mdl;\n"
    "out vec3 v_vertex;\n"
    "void main()\n"
    "{\n"
    "	  v_vertex = mvf_vertex.xyz;\n"
    "	  gl_Position = u_view * u_mdl * mvf_vertex;\n"
    "}\n"
    ,
    //FP
  #ifndef EMSCRIPTEN
    "#version 430 core\n"
  #else
    "#version 300 es\n"
    "precision mediump float;\n"
  #endif
    "uniform vec2 u_tex_scale;\n"
    "in vec3 v_vertex;\n"
    "out vec4 fragColor;"
    "float grid(vec2 tc, float step, float ddu, float ddv)\n"
    "{\n"
    "vec2 fr = fract((tc + vec2(step,step) / 2.0) / vec2(step,step));\n"
    "float du = step * abs(fr.x - 0.5);\n"
    "float dv = step * abs(fr.y - 0.5);\n"
    "float lu = clamp(0.5 * du / ddu, 0.0, 1.0);\n"
    "float lv = clamp(0.5 * dv / ddv, 0.0, 1.0);\n"
    "return 1.0 - min(lu, lv);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "vec2 tc = v_vertex.xy * u_tex_scale;\n"
    "float ddu = max(abs(dFdx(tc).x), abs(dFdy(tc).x));\n"
    "float ddv = max(abs(dFdx(tc).y), abs(dFdy(tc).y));\n"
    /*
    "float ddu = 1.0;\n"
    "float ddv = 1.0;\n"
    */
    "float g = grid(tc, 10.0, ddu, ddv);\n"
    "float db = min(\n"
    "  (0.5 - abs(v_vertex.x - 0.5)) * u_tex_scale.x,\n"
    "  (0.5 - abs(v_vertex.y - 0.5)) * u_tex_scale.y\n"
    ");\n"
    "float b = (db > 2.0) ? 0.0 : 1.0;\n"
    "vec4  clr = g * (1.0 - b) * vec4(0.7, 0.7, 0.7, 0.9)\n"
    "          + b * vec4(0.0, 0.0, 0.0, 1.0);\n"
    "fragColor = clr;\n"
    "\n}"
  );
  g_bed_view.init(g_ShaderBed, "u_view");
  g_bed_mdl.init(g_ShaderBed, "u_mdl");
  g_bed_tex_scale.init(g_ShaderBed, "u_tex_scale");
  // quad
  g_bed_quad = AutoPtr<SimpleMesh>(new SimpleMesh());
  g_bed_quad->begin(GPUMESH_TRIANGLELIST);
  g_bed_quad->vertex_3(0, 0, 0);
  g_bed_quad->vertex_3(1, 0, 0);
  g_bed_quad->vertex_3(0, 1, 0);

  g_bed_quad->vertex_3(0, 1, 0);
  g_bed_quad->vertex_3(1, 0, 0);
  g_bed_quad->vertex_3(1, 1, 0);
  g_bed_quad->end();
}

void bed_render(const m4x4f& proj,const m4x4f& view,float bed_w,float bed_h)
{
  // position beneath part, if any
  m4x4f trl;
  trl.eqIdentity();
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  g_ShaderBed.begin();
  g_bed_view.set(proj*view);
  g_bed_mdl.set(trl*scaleMatrix(v3f(bed_w, bed_h, 1))*translationMatrix(v3f(0.0f, 0.0f, 0.0f))*translationMatrix(v3f(0.0039f, 0.0039f, 0)));
  g_bed_tex_scale.set(v2f(bed_w, bed_h));
  g_bed_quad->render();
  g_ShaderBed.end();
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}