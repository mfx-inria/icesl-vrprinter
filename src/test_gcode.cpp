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

#include "gcode.h"
#include "motion.h"

#include "FileDialog.h"

int main(int argc, const char **argv)
{
  std::string gcode_path = openFileDialog(OFD_FILTER_GCODE);
  std::string test = loadFileIntoString(gcode_path.c_str());

  gcode_start(test.c_str());

#if 1  
  motion_start();
  while (motion_step(100)) {
    std::cerr << gcode_line() << "] " << motion_get_current_pos() << " " << motion_get_current_flow() << std::endl;
    Sleep(1000);
  }
#else
  while (gcode_advance()) {
    std::cerr << gcode_line() << "] " << gcode_next_pos() << " " << gcode_speed() << std::endl;
    Sleep(100);
  }
#endif
  return 0;
}
