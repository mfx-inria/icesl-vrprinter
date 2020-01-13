// SL 2018-07-03
#include "motion.h"
#include "gcode.h"

// --------------------------------------------------------------

v4f   g_CurrentPos  = v4f(0);
float g_CurrentFlow = 0.0f;
float g_ConsumedE = 0.0f;
float g_FilamentCrossArea = (float)M_PI * 1.75f * 1.75f;

// --------------------------------------------------------------

void  motion_start(float filament_diameter_mm)
{
  g_FilamentCrossArea = (float)M_PI * filament_diameter_mm * filament_diameter_mm / 4.0f;
  g_CurrentPos = v4f(0);
  g_CurrentFlow = 0.0f;
  g_ConsumedE = 0.0f;
  gcode_advance();
}

// --------------------------------------------------------------

void motion_reset(float filament_diameter_mm)
{
  g_FilamentCrossArea = (float)M_PI * filament_diameter_mm * filament_diameter_mm / 4.0f;
  g_CurrentPos = gcode_next_pos();
  g_CurrentFlow = 0.0f;
  g_ConsumedE = gcode_next_pos()[3];
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

float motion_step(float delta_ms,bool& _done)
{
  _done          = false;
  bool advance   = false;
  g_CurrentFlow  = 0.0f;
  v3f delta_pos  = v3f(gcode_next_pos() - g_CurrentPos);
  float len      = length(delta_pos);
  float delta_e  = gcode_next_pos()[3] - g_CurrentPos[3];
  float step_e   = 0.0f;
  v3f   step_pos = 0.0f;
  if (len < 1e-3f && fabs(delta_e) > 1e-3f) { 
    // E motion only
    // do not overshot!
    float len_step = delta_ms * gcode_speed() / 1000.0f;
    if (len_step > fabs(delta_e)) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = fabs(delta_e) * 1000.0f / gcode_speed();
      len_step = fabs(delta_e);
    }
    // update
    step_e = len_step * sign(delta_e);
  } else { 
    // all axis motion
    // do not overshot!
    float len_step = delta_ms * gcode_speed() / 1000.0f;
    if (len_step > len) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = len * 1000.0f / gcode_speed();
      len_step = len;
    }
    // update
    v3f dir  = normalize_safe(delta_pos);
    step_pos = dir * len_step;
    if (len > 1e-3f) {
      step_e = delta_e * len_step / len;
    }
  }
  if (g_CurrentPos[3] + step_e - g_ConsumedE > 0) {
    float e_remain = min(g_CurrentPos[3] + step_e - g_ConsumedE, step_e);
    g_CurrentFlow  = e_remain * g_FilamentCrossArea / delta_ms;
  }
  g_CurrentPos += v4f(step_pos, step_e);
  g_ConsumedE   = max(g_ConsumedE, g_CurrentPos[3]);
  // advance in gcode?
  if (advance) {
    // reached current gcode position, advance!
    _done = !gcode_advance();
  }
  return delta_ms;
}

// --------------------------------------------------------------
