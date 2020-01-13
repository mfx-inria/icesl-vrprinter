// SL 2018-07-03
#include "motion.h"
#include "gcode.h"

// --------------------------------------------------------------

v4d    g_PrevGcodePos = v4d(0);
v4d    g_CurrentPos   = v4d(0);
double g_Current_EperXYZ = 0.0;
double g_ConsumedE    = 0.0;
double g_FilamentCrossArea = M_PI * 1.75 * 1.75;

// --------------------------------------------------------------

void  motion_start(double filament_diameter_mm)
{
  g_FilamentCrossArea = (double)M_PI * filament_diameter_mm * filament_diameter_mm / 4.0f;
  g_PrevGcodePos = v4d(0);
  g_CurrentPos = v4d(0);
  g_Current_EperXYZ = 0.0;
  g_ConsumedE  = 0.0;
  gcode_advance();
}

// --------------------------------------------------------------

void motion_reset(double filament_diameter_mm)
{
  g_FilamentCrossArea = (double)M_PI * filament_diameter_mm * filament_diameter_mm / 4.0f;
  g_PrevGcodePos = gcode_next_pos();
  g_CurrentPos = v4d(gcode_next_pos());
  g_Current_EperXYZ = 0.0;
  g_ConsumedE  = (double)gcode_next_pos()[3];
}

// --------------------------------------------------------------

v4d   motion_get_current_pos()
{
  return v4d(g_CurrentPos);
}

// --------------------------------------------------------------

double motion_get_current_e_per_xyz() // (ratio) mm / mm
{
  return g_Current_EperXYZ;
}

// --------------------------------------------------------------

static double e_per_xyz() // (ratio) mm / mm
{
  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  if (ln < 1e-6) {
    return 0.0;
  }
  return (gcode_next_pos()[3] - g_PrevGcodePos[3]) / ln;
}

// --------------------------------------------------------------

double motion_get_current_flow() // mm^3 / sec
{
  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  double vl = (gcode_next_pos()[3] - g_PrevGcodePos[3]) * g_FilamentCrossArea;
  if (gcode_speed() < 1) {
    return 0.0f;
  }
  double tm = ln / gcode_speed();
  if (tm < 1e-6f) {
    return 0.0f;
  }
  return (double)(vl / tm);
}

// --------------------------------------------------------------

double motion_step(double delta_ms,bool& _done)
{
  if (gcode_line() == 2378) {
    std::cerr << "x";
  }

  _done           = false;
  bool advance    = false;
  v3d delta_pos   = v3d(gcode_next_pos()) - v3d(g_CurrentPos);
  double len      = (double)length(delta_pos);
  double delta_e  = (double)gcode_next_pos()[3] - g_CurrentPos[3];
  double step_e   = 0.0;
  v3d    step_pos = 0.0;

  if (abs(len) < 1e-6 && abs(delta_e) > 1e-6) {

    // E motion only
    // do not overshot!
    double len_step = (double)delta_ms * (double)gcode_speed() / 1000.0;
    if (len_step > abs(delta_e)) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = abs(delta_e) * 1000.0 / (double)gcode_speed();
      len_step = abs(delta_e);
    }
    // update
    step_e = len_step * sign(delta_e);

  } else if (abs(len) > 1e-6) {

    // all axis motion
    // do not overshot!
    double len_step = (double)delta_ms * (double)gcode_speed() / 1000.0;
    if (len_step > len) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = len * 1000.0 / (double)gcode_speed();
      len_step = len;
    }
    // update
    v3d dir          = normalize_safe(delta_pos);
    step_pos         = dir * len_step;
    step_e           = len_step * e_per_xyz();

  } else {
    // only advance in gcode
    advance  = true;
    delta_ms = 0.0;
    step_pos = 0.0;
    step_e   = 0.0;
  }
  g_Current_EperXYZ = e_per_xyz();
  if (advance) {
    // snap to exact pos
    g_CurrentPos = gcode_next_pos();
  } else {
    g_CurrentPos += v4d(step_pos, step_e);
  }
  g_ConsumedE = max(g_ConsumedE, g_CurrentPos[3]);
  // advance in gcode?
  if (advance) {
    //std::cerr << 'a';
    // reached current gcode position, advance!
    g_PrevGcodePos = gcode_next_pos();
    _done          = !gcode_advance();
  } else {
    //std::cerr << '_';
  }
  return delta_ms;
}

// --------------------------------------------------------------
