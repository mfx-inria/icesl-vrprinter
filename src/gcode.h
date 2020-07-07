// SL 2018-07-03
#pragma once

#include <LibSL.h>

// start interpreting the gcode
void gcode_start(const char *gcode);

// advances to the next position
// return false if none exists (end of gcode)
bool gcode_advance();

// returns the next position to reach (x,y,z,e)
v4d  gcode_next_pos();

// returns the speed in mm/sec
double gcode_speed();

// return the number of used extruders
size_t gcode_extruders();

// returns the current extruder
int gcode_current_extruder();

// return current line in gcode stream
int gcode_line();

// restart from scratch
void gcode_reset();

// returns true in a reading error occured
bool gcode_error();

// returns the extrusion type (true -> volumetric | false -> default)
bool gcode_volumetric_mode();

// returns the filament diameter provided by M200 (volumetric extrusion)
double gcode_filament_dia();
