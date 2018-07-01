// SL 2018-07-01

#pragma once

#include <LibSL.h>
#include <LibSL_gl.h>

void bed_init();
void bed_render(const m4x4f& proj,const m4x4f& view,float bed_w,float bed_h);

