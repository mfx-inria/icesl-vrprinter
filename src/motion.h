// SL 2018-07-03
#pragma once

#include <LibSL.h>

// start motion, assumes gcode is ready (gcode_start has been called)
void  motion_start(double filament_diameter_mm, bool volumetric);

// returns the current pos
v4d   motion_get_current_pos();

// returns the current (instantaneaous) flow
// mm^3 per milliseconds
double motion_get_current_flow();

// returns the current (instantaneaous) ratio in E and XYZ axes
double motion_get_current_e_per_xyz();

// performs the next motion step, takes as input the step in milliseconds
// returns the consumed time (may be less than delta_ms)
double motion_step(double delta_ms,bool& _done);

// restarts from scratch
void motion_reset(double filament_diameter_mm, bool volumetric);

// returns true if motion is a travel
bool motion_is_travel();
