// SL 2018-07-03
#include "motion.h"
#include "gcode.h"

// --------------------------------------------------------------

bool   g_IsTravel     = false;

v4d    g_PrevGcodePos = v4d(0);
v4d    g_CurrentPos   = v4d(0);

double g_Current_EperXYZ = 0.0;

double g_FilamentDiamenter = 1.75;

// --------------------------------------------------------------

void motion_start(double filament_diameter_mm)
{
  motion_reset(filament_diameter_mm);
  gcode_advance();
}

// --------------------------------------------------------------

void motion_reset(double filament_diameter_mm)
{
  g_FilamentDiamenter = filament_diameter_mm;

  g_IsTravel = false;
  
  g_PrevGcodePos = gcode_next_pos();
  g_CurrentPos = gcode_next_pos();
  g_Current_EperXYZ = 0.0;
}

// --------------------------------------------------------------

v4d motion_get_current_pos()
{
  return v4d(g_CurrentPos);
}

// --------------------------------------------------------------

double motion_get_current_e_per_xyz() // (ratio) mm / mm
{
  return g_Current_EperXYZ;
}

// --------------------------------------------------------------

static double e_from_volumetric(double e_vol)
{
  return e_vol / (pow(g_FilamentDiamenter / 2, 2) * M_PI);
}

// --------------------------------------------------------------

static double filament_cross_section(double filament_diameter) {
  return M_PI * filament_diameter * filament_diameter / 4.0;
}

// --------------------------------------------------------------

static double e_per_xyz() // (ratio) mm / mm
{
  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  if (ln < 1e-6) {
    return 0.0;
  }

  double delta_e = gcode_next_pos()[3] - g_PrevGcodePos[3];

  double e = delta_e / ln;

  return e;
}

// --------------------------------------------------------------

double motion_get_current_flow() // mm^3 / sec
{
  double delta_e = gcode_next_pos()[3] - g_PrevGcodePos[3];

  double vl = delta_e * filament_cross_section(g_FilamentDiamenter);

  if (gcode_speed() < 1) {
    return 0.0;
  }

  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  double tm = ln / gcode_speed();
  if (tm < 1e-6f) {
    return 0.0;
  }

  return vl / tm;
}

// --------------------------------------------------------------

bool motion_is_travel()
{
  return g_IsTravel;
}

// --------------------------------------------------------------

double motion_step(double delta_ms, bool& _done)
{
  _done           = false;
  bool advance    = false;
  v3d delta_pos   = v3d(gcode_next_pos()) - v3d(g_CurrentPos);
  double len      = length(delta_pos);
  double delta_e  = gcode_next_pos()[3] - g_CurrentPos[3];
  double step_e   = 0.0;
  v3d    step_pos = 0.0;

  g_Current_EperXYZ = e_per_xyz();

  g_IsTravel = abs(delta_e) < 1e-6;

  double len_step = delta_ms * gcode_speed() / 1000.0;
  if (abs(len) < 1e-6 && abs(delta_e) > 1e-6) { // E motion only - do not overshot!
    if (len_step > abs(delta_e)) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = abs(delta_e) * 1000.0 / gcode_speed();
      len_step = abs(delta_e);
    }
    // update
    step_e = len_step * sign(delta_e);
  } else if (abs(len) > 1e-6) { // all axis motion - do not overshot!
    if (len_step > len) {
      advance = true;
      // adjust time step to reach exactly the target
      delta_ms = len * 1000.0 / gcode_speed();
      len_step = len;
    }
    // update
    step_pos = normalize_safe(delta_pos) * len_step; // dir * step
    step_e   = len_step * e_per_xyz();
  } else { // only advance in gcode
    advance  = true;
    delta_ms = 0.0;
    step_pos = 0.0;
    step_e   = 0.0;
  }

#if 0
  std::cerr << Console::green << "step_pos: " << step_pos
            << Console::magenta << " step_e: " << step_e
            << Console::yellow << " delta_e: " << delta_e
            << Console::cyan << " next_e: " << gcode_next_pos()[3] << " current_e: " << g_CurrentPos[3]
            << Console::gray << std::endl;
#endif

  // advance in gcode?
  if (advance) { // reached current gcode position, advance!
    //std::cerr << 'a';
    // snap to exact pos
    g_CurrentPos   = gcode_next_pos();
    g_PrevGcodePos = gcode_next_pos();
    _done          = !gcode_advance();
  } else {
    //std::cerr << '_';
    g_CurrentPos += v4d(step_pos, step_e);
  }

  return delta_ms;
}

// --------------------------------------------------------------
