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

// --------------------------------------------------------------

typedef LibSL::BasicParser::BufferStream t_stream;
typedef LibSL::BasicParser::Parser<LibSL::BasicParser::BufferStream> t_parser;
typedef AutoPtr<t_stream> t_stream_ptr;
typedef AutoPtr<t_parser> t_parser_ptr;

// --------------------------------------------------------------

t_stream_ptr g_Stream;
t_parser_ptr g_Parser;

std::set<int> g_Extruders;
int           g_CurrentExtruder = 0;

bool         g_RelativeExMode = false; // false -> absolute extrusion | true - > relative extrusion 
bool         g_VolumetricMode = false;

double       g_FilDiameter = 1.75; // used when volumetric extrusion is detected

v4d          g_Pos(0.0);
v4d          g_Offset(0.0); // TODO : rework to have offset per extruder/tool
double       g_Speed = 20.0;
int          g_Line = 0;
const char  *g_GCode = NULL;
bool         g_GCodeError = false;

// --------------------------------------------------------------

static void set_extruder(int extruder) {
  g_Extruders.insert(extruder);
  g_CurrentExtruder = extruder;
}

// --------------------------------------------------------------

static double e_from_volumetric(double e_vol)
{
  return e_vol / (pow(g_FilDiameter / 2, 2) * M_PI);
}

// --------------------------------------------------------------

void gcode_start(const char *gcode)
{
  g_GCode  = gcode;
  gcode_reset();
}

// --------------------------------------------------------------

void gcode_reset()
{
  sl_assert(g_GCode != NULL);
  g_Stream = t_stream_ptr(new t_stream(g_GCode, (uint)strlen(g_GCode) + 1));
  g_Parser = t_parser_ptr(new t_parser(*g_Stream, false));

  //g_Extruders.clear();
  //set_extruder(0);
  g_CurrentExtruder = 0;
  g_FilDiameter = 1.75;
  g_RelativeExMode = false;
  g_VolumetricMode = false;

  g_Line = 0;
  g_GCodeError = false;

  g_Pos = 0.0f;
  g_Offset = 0.0f;
  g_Speed = 20.0f;  
}

// --------------------------------------------------------------

bool gcode_advance()
{
  if (g_GCodeError) return false;
  int c;
  while (!g_Parser->eof()) {
    g_Line ++;
    c = g_Parser->readChar();
    c = tolower(c);
    if (c == 'g') { // G gcode
      int n = g_Parser->readInt();
      if (n == 0 || n == 1) { // G0 G1
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          if (c == ';') {
            g_Parser->reachChar('\n');
            break;
          }
          c = tolower(c);
          double f = g_Parser->readDouble();
          if (c >= 'x' && c <= 'z') { // XYZ coordinates
            g_Pos[c - 'x'] = f + g_Offset[c - 'x'];
          } else if (c == 'e') { // E extrusion value
            double e = f;
            if (g_VolumetricMode) { // convert the e_value back to a length
              e = e_from_volumetric(f);
            }
            if (g_RelativeExMode) { // if relative extrusion is detected, individual extrusion steps are merged to behave like absolute extrusion
              e = g_Pos[3] + f;
            }
            g_Pos[3] = e + g_Offset[3];
          } else if (c == 'f') { // F feedrate
            g_Speed = f / 60.0f;
          } else if ((c >= 'a' && c <= 'd') || c == 'h') { // ABCDH mixing ratios
            // TODO mixing ratios
            /*
            if (c == 'a') {
              //TODO
            } else if (c == 'b') {
              //TODO
            } else if (c == 'c') {
              //TODO
            } else if (c == 'd') {
              //TODO
            } else if (c == 'h') {
              //TODO
            }
            */
            g_Parser->reachChar('\n');
          } else {
            g_GCodeError = true;
            return false;
          }
        }
#if 0
        // flow check (DEBUG)
        double ln = length(v3d(g_Pos) - v3d(pos_before));
        if (ln > 1e-6f) {
          double ex = g_Pos[3] - pos_before[3];
          std::cerr << ex / ln << ' ';
          if (ex / ln > 0.3f) {
            std::cerr << ex / ln << ' ';
          }
        }
#endif
        break; // done advancing
      } else if (n == 92) { // G92 reset axis values
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          c = tolower(c);
          double d = g_Parser->readDouble();
          if (c >= 'x' && c <= 'z') {
            g_Offset[c - 'x'] = g_Pos[c - 'x'] - d;
          } else if (c == 'e') { // G92 E0 extruder values reset
            g_Offset[3] = g_Pos[3] - d;
          }
        }        
        if (gcode_extruders() > 2) { // Dirty fix
          g_Parser->reachChar('\n'); // PB NOTE: fixes the latence when switching between multiple extruders (when more than 2 extruders are present) but breaks dual extrusion managment?
        }
      } else if (n == 10) { // G10
        g_Parser->reachChar('\n');
      } else if (n == 11) { // G11
        g_Parser->reachChar('\n');
      } else { // other => ignore
        g_Parser->reachChar('\n');
      }
    } else if (c == 'm') { // M gcode
      int n = g_Parser->readInt();
      if (n == 82) { // M82: absolute extrusion
        g_RelativeExMode = false;
        g_Parser->reachChar('\n');
      } else if (n == 83) { // M83 relative extrusion
        g_RelativeExMode = true;
        g_Parser->reachChar('\n');
      } else if (n == 200) { // M200 set filament diameter & enable volumetric extrusion
        g_VolumetricMode = true;
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          c = tolower(c);
          double d = g_Parser->readDouble();
          if (c == 'd') {
            g_FilDiameter = d; // update the filament diameter with the one provided by M200
          }
        }
        g_Parser->reachChar('\n');
      } else { // other => ignore
        g_Parser->reachChar('\n');
      }
    } else if (c == 't') { // T tool selection
      int e = g_Parser->readInt();
      set_extruder(e);
      g_Parser->reachChar('\n');
    } else if (c == ';') { // comments
      if (g_Line == 1) {
        std::string s = g_Parser->readString();
        if (s == "FLAVOR:UltiGCode") { // detecting UltiGcode to enable volumetric extrusion
          g_VolumetricMode = true;
          g_FilDiameter = 2.85;
          //std::cerr << Console::blue << "UM2 detected" << Console::gray << std::endl;
        }
      } else if (g_Line == 3) {
        std::string s = g_Parser->readString();
        if (s == "FLAVOR:Griffin") { // detecting UltiGcode (Ultimaker 3 or newer) to enable volumetric extrusion
          g_VolumetricMode = false;
          g_FilDiameter = 2.85;
          //std::cerr << Console::blue << "UM3 detected" << Console::gray << std::endl;
        }
      }
      g_Parser->reachChar('\n');
    } else if (c == '<') {
      g_Parser->reachChar('\n');
    } else if (c == '\r') {
      g_Parser->reachChar('\n');
    } else if (c == '\n') {
      // do nothing
    } else if (c == '\0' || c == -1) {
      return false;
    } else {
      std::cerr << Console::red <<  "Error parsing GCode line " << g_Line << Console::gray << std::endl;
      g_GCodeError = true;
      return false;
    }
  }
  return !g_Parser->eof();
}

// --------------------------------------------------------------

v4d gcode_next_pos()
{
  return g_Pos;
}

// --------------------------------------------------------------

double gcode_speed()
{
  return g_Speed;
}

// --------------------------------------------------------------

size_t gcode_extruders()
{
  return g_Extruders.size();
}

// --------------------------------------------------------------

int gcode_current_extruder()
{
  return g_CurrentExtruder;
}

// --------------------------------------------------------------

int gcode_line()
{
  return g_Line;
}

// --------------------------------------------------------------

bool gcode_error() 
{
  return g_GCodeError;
}

// --------------------------------------------------------------


double gcode_filament_dia()
{
  return g_FilDiameter;
}

// --------------------------------------------------------------