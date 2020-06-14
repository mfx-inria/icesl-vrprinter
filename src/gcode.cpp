#include "gcode.h"

// --------------------------------------------------------------

typedef LibSL::BasicParser::BufferStream t_stream;
typedef LibSL::BasicParser::Parser<LibSL::BasicParser::BufferStream> t_parser;
typedef AutoPtr<t_stream> t_stream_ptr;
typedef AutoPtr<t_parser> t_parser_ptr;

// --------------------------------------------------------------

t_stream_ptr g_Stream;
t_parser_ptr g_Parser;

std::vector<int> g_Extruders;
int              g_CurrentExtruder = 0;

v4d          g_Pos(0.0);
v4d          g_Offset(0.0);
double       g_Speed = 20.0;
int          g_Line = 0;
const char  *g_GCode = NULL;
bool         g_GCodeError = false;

// --------------------------------------------------------------

static void set_extruder(int extruder) {
  //check if we already registered the extruder
  if (!(std::find(g_Extruders.begin(), g_Extruders.end(), extruder) != g_Extruders.end())) {
    g_Extruders.push_back(extruder); // register new extruder
  }
  g_CurrentExtruder = extruder;
}

// --------------------------------------------------------------

void gcode_start(const char *gcode)
{
  g_GCode  = gcode;
  g_Stream = t_stream_ptr(new t_stream(g_GCode,(uint)strlen(g_GCode)+1));
  g_Parser = t_parser_ptr(new t_parser(*g_Stream,false));
  g_Pos = 0.0f;
  g_Offset = 0.0f;
  g_Speed = 20.0f;
  g_CurrentExtruder = 0;
  g_Line = 0;
  g_GCodeError = false;
}

// --------------------------------------------------------------

void gcode_reset()
{
  sl_assert(g_GCode != NULL);
  g_Stream = t_stream_ptr(new t_stream(g_GCode, (uint)strlen(g_GCode) + 1));
  g_Parser = t_parser_ptr(new t_parser(*g_Stream, false));
  g_Pos = 0.0f;
  g_Offset = 0.0f;
  g_Speed = 20.0f;
  g_CurrentExtruder = 0;
  g_Line = 0;
  g_GCodeError = false;
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
    if (c == 'g') {
      int n = g_Parser->readInt();
      if (n == 0 || n == 1) { // G0 G1
        //v4d pos_before = g_Pos;
        //double e_raw   = 0.0f;
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          if (c == ';') {
            g_Parser->reachChar('\n');
            break;
          }
          c = tolower(c);
          double f = g_Parser->readDouble();
          if (c >= 'x' && c <= 'z') {
            g_Pos[c - 'x'] = f + g_Offset[c - 'x'];
          } else if (c == 'e') {
            g_Pos[3] = f + g_Offset[3];
            //e_raw    = f;
          } else if (c == 'f') {
            g_Speed = f / 60.0f;
          } else if (c >= 'a' && c <= 'f') {
            // TODO mixing ratios
          } else {
            g_GCodeError = true;
            return false;
          }
        }
        // flow check (DEBUG)
        /*
        double ln = length(v3d(g_Pos) - v3d(pos_before));
        if (ln > 1e-6f) {
          double ex = g_Pos[3] - pos_before[3];
          std::cerr << ex / ln << ' ';
          if (ex / ln > 0.3f) {
            std::cerr << ex / ln << ' ';
          }
        }
        */
        break; // done advancing
      } else if (n == 92) { // G92
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          c = tolower(c);
          double f = g_Parser->readDouble();
          if (c >= 'x' && c <= 'z') {
            g_Offset[c - 'x'] = g_Pos[c - 'x'] - f;
          } else if (c == 'e') {
            g_Offset[3] = g_Pos[3] - f;
          }
        }
      } else if (n == 10) { // G10
        g_Parser->reachChar('\n');
      } else if (n == 11) { // G11
        g_Parser->reachChar('\n');
      } else { // other => ignore
        g_Parser->reachChar('\n');
      }
    } else if (c == 'm') {
      int n = g_Parser->readInt();
      g_Parser->reachChar('\n');
    } else if (c == 't') {
      int e = g_Parser->readInt();
      set_extruder(e);
      g_Parser->reachChar('\n');
    } else if (c == '\n') {
      // do nothing
    } else if (c == '<') {
      g_Parser->reachChar('\n');
    } else if (c == ';') {
      g_Parser->reachChar('\n');
    } else if (c == '\r') {
      g_Parser->reachChar('\n');
    } else if (c == '\0' || c == -1) {
      return false;
    } else {
      std::cerr << "Error parsing GCode line " << g_Line << std::endl;
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

std::vector<int> gcode_extruders_list()
{
  std::sort(g_Extruders.begin(), g_Extruders.end());
  return g_Extruders;
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
