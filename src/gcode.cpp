#include "gcode.h"

// --------------------------------------------------------------

typedef LibSL::BasicParser::BufferStream t_stream;
typedef LibSL::BasicParser::Parser<LibSL::BasicParser::BufferStream> t_parser;
typedef AutoPtr<t_stream> t_stream_ptr;
typedef AutoPtr<t_parser> t_parser_ptr;

// --------------------------------------------------------------

t_stream_ptr g_Stream;
t_parser_ptr g_Parser;

v4f         g_Pos(0.0f);
v4f         g_Offset(0.0f);
float       g_Speed = 0.0f;
int         g_Line = 0;
const char *g_GCode = NULL;

// --------------------------------------------------------------

void gcode_start(const char *gcode)
{
  g_GCode  = gcode;
  g_Stream = t_stream_ptr(new t_stream(g_GCode,(uint)strlen(g_GCode)+1));
  g_Parser = t_parser_ptr(new t_parser(*g_Stream,false));
  g_Pos = 0.0f;
  g_Offset = 0.0f;
  g_Speed = 0.0f;
  g_Line = 0;
}

// --------------------------------------------------------------

void gcode_reset()
{
  sl_assert(g_GCode != NULL);
  g_Stream = t_stream_ptr(new t_stream(g_GCode, (uint)strlen(g_GCode) + 1));
  g_Parser = t_parser_ptr(new t_parser(*g_Stream, false));
  g_Pos = 0.0f;
  g_Offset = 0.0f;
  g_Speed = 0.0f;
  g_Line = 0;
}

// --------------------------------------------------------------

bool gcode_advance()
{
  int c;
  while (!g_Parser->eof()) {
    g_Line ++;
    c = g_Parser->readChar();
    if (c == 'G') {
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
          float f = g_Parser->readFloat();
          if (c >= 'x' && c <= 'z') {
            g_Pos[c - 'x'] = f + g_Offset[c - 'x'];
          } else if (c == 'e') {
            g_Pos[3] = f + g_Offset[3];
          } else if (c == 'f') {
            g_Speed = f / 60.0f;
          }
        }
        break; // done advancing
      } else if (n == 92) { // G92
        while (!g_Parser->eof()) {
          c = g_Parser->readChar();
          if (c == '\n') break;
          c = tolower(c);
          float f = g_Parser->readFloat();
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
      } else { // other
        g_Parser->reachChar('\n');
      }
    } else if (c == 'M') {
      int n = g_Parser->readInt();
      g_Parser->reachChar('\n');
    } else if (c == '\n') {
      // do nothing
    } else {
      g_Parser->reachChar('\n');
    }
  }
  return !g_Parser->eof();
}

// --------------------------------------------------------------

v4f gcode_next_pos()
{
  return g_Pos;
}

// --------------------------------------------------------------

float gcode_speed()
{
  return g_Speed;
}

// --------------------------------------------------------------

int gcode_line()
{
  return g_Line;
}

// --------------------------------------------------------------
