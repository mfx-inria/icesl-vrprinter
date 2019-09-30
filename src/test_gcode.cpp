#include "gcode.h"
#include "motion.h"

int main(int argc, const char **argv)
{

  std::string test = loadFileIntoString("E:\\SLEFEBVR\\PROJECTS\\IceSL_next\\icesl-next\\tests\\models\\pyramid.gcode");

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