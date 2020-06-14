// SL 2018-07-03
#include "motion.h"
#include "gcode.h"

// --------------------------------------------------------------

bool   g_IsTravel     = false;
bool   g_IsVolumetric = false;

v4d    g_PrevGcodePos = v4d(0);
v4d    g_CurrentPos   = v4d(0);

double g_Current_EperXYZ = 0.0;

double g_FilamentDiamenter = 1.75;
double g_FilamentCrossArea = M_PI * g_FilamentDiamenter * g_FilamentDiamenter;

// --------------------------------------------------------------

void motion_start(double filament_diameter_mm, bool volumetric)
{
  g_IsTravel = false;
  g_IsVolumetric = volumetric;
  g_PrevGcodePos = v4d(0);
  g_CurrentPos = v4d(0);
  g_Current_EperXYZ = 0.0;
  g_FilamentDiamenter = filament_diameter_mm;
  g_FilamentCrossArea = M_PI * g_FilamentDiamenter * g_FilamentDiamenter / 4.0;
  gcode_advance();
}

// --------------------------------------------------------------

void motion_reset(double filament_diameter_mm, bool volumetric)
{
  g_IsTravel = false;
  g_IsVolumetric = volumetric;
  g_PrevGcodePos = gcode_next_pos();
  g_CurrentPos = gcode_next_pos();
  g_Current_EperXYZ = 0.0;
  g_FilamentDiamenter = filament_diameter_mm;
  g_FilamentCrossArea = M_PI * g_FilamentDiamenter * g_FilamentDiamenter / 4.0;
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

static double e_from_volumetric(double e_vol) {
  return e_vol / (pow(g_FilamentDiamenter/2, 2) * M_PI);
}

// --------------------------------------------------------------

static double e_per_xyz() // (ratio) mm / mm
{
  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  if (ln < 1e-6) {
    return 0.0;
  }

  double e = (gcode_next_pos()[3] - g_PrevGcodePos[3]) / ln;
  if (g_IsVolumetric) {
    e = (e_from_volumetric(gcode_next_pos()[3]) - e_from_volumetric(g_PrevGcodePos[3])) / ln;
  } 
  return e;
}

// --------------------------------------------------------------

double motion_get_current_flow() // mm^3 / sec
{
  double ln = length(v3d(gcode_next_pos()) - v3d(g_PrevGcodePos));
  double vl = (gcode_next_pos()[3] - g_PrevGcodePos[3]) * g_FilamentCrossArea;
  if (g_IsVolumetric) {
    vl = (e_from_volumetric(gcode_next_pos()[3]) - e_from_volumetric(g_PrevGcodePos[3])) * g_FilamentCrossArea;
  }
  if (gcode_speed() < 1) {
    return 0.0f;
  }
  double tm = ln / gcode_speed();
  if (tm < 1e-6f) {
    return 0.0f;
  }
  return vl / tm;
}

// --------------------------------------------------------------

bool motion_is_travel()
{
  return g_IsTravel;
}

// --------------------------------------------------------------

double motion_step(double delta_ms,bool& _done)
{
  _done           = false;
  bool advance    = false;
  v3d delta_pos   = v3d(gcode_next_pos()) - v3d(g_CurrentPos);
  double len      = length(delta_pos);
  double delta_e  = gcode_next_pos()[3] - g_CurrentPos[3];
  double step_e   = 0.0;
  v3d    step_pos = 0.0;

  if (g_IsVolumetric) {
    delta_e = e_from_volumetric(gcode_next_pos()[3]) - e_from_volumetric(g_CurrentPos[3]);
  }

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
  if (g_IsVolumetric) {
    g_IsTravel = abs(e_from_volumetric(gcode_next_pos()[3]) - e_from_volumetric(g_PrevGcodePos[3])) < 1e-6;
  } else {
    g_IsTravel = abs(gcode_next_pos()[3] - g_PrevGcodePos[3]) < 1e-6;
  }

  if (advance) {
    // snap to exact pos
    g_CurrentPos = gcode_next_pos();
  } else {
    g_CurrentPos += v4d(step_pos, step_e);
  }

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
