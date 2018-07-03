// SL 2018-07-03
#include "motion.h"
#include "gcode.h"

// --------------------------------------------------------------

v4f   g_CurrentPos = v4f(0);
float g_CurrentFlow = 0.0f;
float g_FilamentArea = (float)M_PI * 1.75f*1.75f;

// --------------------------------------------------------------

void  motion_start(float filament_diameter_mm)
{
  g_FilamentArea = (float)M_PI * filament_diameter_mm * filament_diameter_mm;
  g_CurrentPos = v4f(0);
  g_CurrentFlow = 0.0f;
  gcode_advance();
}

// --------------------------------------------------------------

void motion_reset(float filament_diameter_mm)
{
  gcode_reset();
  motion_start(filament_diameter_mm);
}

// --------------------------------------------------------------

v4f   motion_get_current_pos()
{
  return g_CurrentPos;
}

// --------------------------------------------------------------

float motion_get_current_flow()
{
  return g_CurrentFlow;
}

// --------------------------------------------------------------

bool  motion_step(float delta_ms)
{
  v3f delta_pos  = v3f(gcode_next_pos() - g_CurrentPos);
  float len      = length(delta_pos);
  float delta_e  = gcode_next_pos()[3] - g_CurrentPos[3];
  float len_step = delta_ms * gcode_speed() / 1000.0f;
  float step_e   = 0.0f;
  v3f   step_pos = 0.0f;
  if (len < 1e-6f && fabs(delta_e) > 1e-6f) {
    // E motion only
    step_e = min(fabs(delta_e),len_step) * sign(delta_e);
  } else {
    // all axis motion
    v3f dir = normalize_safe(delta_pos);
    step_pos = dir * min(len, len_step);
    if (len > 1e-10f) {
      step_e = delta_e * length(step_pos) / len;
    }
  }
  if (g_CurrentPos[3] > -step_e) {
    g_CurrentFlow = min(g_CurrentPos[3] + step_e, step_e) * g_FilamentArea / delta_ms;
  }
  g_CurrentPos += v4f(step_pos, step_e);

  // NOTE: this ignores the fact that motion continues on the next
  //       segment during the same time step. TODO (recurse?)

  // advance in gcode?
  if (sqLength(gcode_next_pos() - g_CurrentPos) < 1e-6f) {
    // reached current gcode position, advance!
    bool done = !gcode_advance();
    if (done) return false;
  }
  return true;
}

// --------------------------------------------------------------
