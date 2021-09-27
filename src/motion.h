/**
  * IceSL-vrprinter, a tool to help simulate and visualize Gcode for 3D printers
  * Copyright (C) 2021  Sylvain Lefebvre    sylvain.lefebvre@inria.fr
  *                     Pierre Bedell       pierre.bedell@gmail.com
  *                     Salim Perchy        salim.perchy@gmail.com
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU Affero General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU Affero General Public License for more details.
  *
  * You should have received a copy of the GNU Affero General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

// SL 2018-07-03
#pragma once

#include <LibSL.h>

// start motion, assumes gcode is ready (gcode_start has been called)
void motion_start(double filament_diameter_mm);

// restarts from scratch
void motion_reset(double filament_diameter_mm);

// performs the next motion step, takes as input the step in milliseconds
// returns the consumed time (may be less than delta_ms)
double motion_step(double delta_ms, bool& _done);

// returns the current pos
v4d motion_get_current_pos();

// returns the current (instantaneaous) flow (mm^3 per milliseconds)
double motion_get_current_flow();

// returns the current (instantaneaous) ratio in E and XYZ axes
double motion_get_current_e_per_xyz();

// returns true if motion is a travel
bool motion_is_travel();
