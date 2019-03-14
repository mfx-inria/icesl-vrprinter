// SL 2018-07-03
#pragma once

#include <LibSL.h>

// start interpreting the gcode
void gcode_start(const char *gcode);

// restart from scratch
void gcode_reset();

// advances to the next position
// return false if none exists (end of gcode)
bool gcode_advance();

// returns the next position to reach (x,y,z,e)
v4f  gcode_next_pos();

// returns the speed in mm/sec
float gcode_speed();

// returns the current extruder
int gcode_extruder();

// return current line in gcode stream
int gcode_line();

// returns true in a reading error occured
bool gcode_error();
