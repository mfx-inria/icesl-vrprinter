// SL 2018-07-03
#pragma once

#include <LibSL.h>

// start motion, assumes gcode is ready (gcode_start has been called)
void  motion_start(float filament_diameter_mm);

// returns the current pos
v4f   motion_get_current_pos();

// returns the current (instantaneaous) flow
// mm^3 per milliseconds
float motion_get_current_flow();

// performs the next motion step, takes as input the step in milliseconds
// returns the consumed time (may be less than delta_ms)
float motion_step(float delta_ms,bool& _done);

// restarts from scratch
void motion_reset(float filament_diameter_mm);