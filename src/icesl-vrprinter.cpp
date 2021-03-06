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

// NOTE: we assume there is no curved printing, eg z grows monotically

#include <iostream>
#include <fstream>

#include <LibSL/UIHelpers/StyleManager.h>

#include "icesl-vrprinter.h"

#include "bed.h"
#include "shapes.h"
#include "gcode.h"
#include "motion.h"

#ifndef WIN32
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/stat.h>
#else
  #include <algorithm>
#endif

// ----------------------------------------------------------------

GPUBead g_Bead;

// ----------------------------------------------------------------

int main(int argc, const char* argv[])
{
#ifndef EMSCRIPTEN
  /// prepare cmd line arguments
  TCLAP::CmdLine   cmd(" Analyse Gcode and produce statistics", ' ', "1.0");

  TCLAP::UnlabeledValueArg<std::string> gcArg("gcode", "gcode to load", false, "", "filename");
  TCLAP::SwitchArg statsArg("s", "stats", "compute stats and return", false);
  TCLAP::ValueArg<float> export_statsArg("e", "export", "export and filter (percent to keep: 0.0 to 1.0) computed stats to a latex file", false, -1.0f, "float");
  TCLAP::ValueArg<int> viewArg("v", "view", "use a predefined view for trackballUI", false, -1, "int");

  std::string cmd_gcode = "";
  bool cmd_stats = false;
  float cmd_export_stats = 1.0f;
  int cmd_view = -1;

  try
  {
    cmd.add(gcArg);
    cmd.add(statsArg);
    cmd.add(export_statsArg);
    cmd.add(viewArg);
    cmd.parse(argc, argv);

    cmd_gcode = gcArg.getValue();
    cmd_stats = statsArg.getValue();
    cmd_export_stats = export_statsArg.getValue();
    cmd_view = viewArg.getValue();
  }
  catch (const TCLAP::ArgException & e)
  {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Error: unknown exception caught" << std::endl;
  }

  if (!cmd_gcode.empty()) {
    g_GCode_path = cmd_gcode.c_str();
  }
#endif

  /// load gcode
  load_gcode(g_GCode_path);
  g_GCode_string = loadFileIntoString(g_GCode_path.c_str());
  session_start();

#ifndef EMSCRIPTEN
  /// stats mode (generate stats without opening GUI)
  if (cmd_stats) {
    Console::progressTextInit(g_LastLine);
    while (!step_simulation(false)) {
      Console::progressTextUpdate(gcode_line());
    }
    Console::progressTextEnd();

    Histogram hd, ho;
    gen_histogram(g_DanglingHisto, hd);
    gen_histogram(g_OverlapHisto, ho);
    std::cout << Console::green << "\n== unsupported ==" << Console::gray << std::endl;
    hd.print();
    std::cout << Console::green << "==  overlaps   ==" << Console::gray << std::endl;
    ho.print();

    // export as a .tex histogram
    if (cmd_export_stats != -1.0f) {
      gen_histogram(g_DanglingHisto, hd, cmd_export_stats);
      gen_histogram(g_OverlapHisto, ho, cmd_export_stats);
      export_histogram("dangling", hd);
      export_histogram("overlap", ho);
    }

    exit(0);
  }
#endif

  /// init TrackballUI UI
  TrackballUI::onRender = mainRender;
  TrackballUI::onKeyPressed = mainKeyboard;
  TrackballUI::onMouseButtonPressed = mainMouseButton;
  TrackballUI::onMouseWheel = mainMouseWheel;
  TrackballUI::onReshape = mainWindowReshape;

  TrackballUI::init(g_ScreenWidth, g_ScreenHeight);

  // GL init
  glEnable(GL_DEPTH_TEST);

  // imgui binding with SimpleUI
  SimpleUI::bindImGui();

  // quad for shader invocation
  g_GPUMesh_quad = AutoPtr<SimpleMesh>(new SimpleMesh());
  g_GPUMesh_quad->begin(GPUMESH_TRIANGLELIST);
  g_GPUMesh_quad->vertex_3(0, 0, 0);
  g_GPUMesh_quad->vertex_3(1, 0, 0);
  g_GPUMesh_quad->vertex_3(0, 1, 0);

  g_GPUMesh_quad->vertex_3(0, 1, 0);
  g_GPUMesh_quad->vertex_3(1, 0, 0);
  g_GPUMesh_quad->vertex_3(1, 1, 0);
  g_GPUMesh_quad->end();

  // mesh for axes
  makeAxisMesh();

  // bed rendering
  bed_init();

  /// shaders init
#ifdef EMSCRIPTEN
  g_ShaderSimple.settings = "#version 300 es\nprecision mediump float;\nprecision mediump int;\n";
  g_ShaderDeposition.settings = "#version 300 es\nprecision mediump float;\n";
  g_ShaderFinal.settings = "#version 300 es\nprecision mediump float;\nprecision mediump int;\n";
#else
  g_ShaderSimple.settings = "#version 430 core\n";
  g_ShaderDeposition.settings = "#version 430 core\n";
  g_ShaderFinal.settings = "#version 430 core\n";
#endif
  g_ShaderSimple.init(); // shader for simple drawing
  g_ShaderDeposition.init(); // shader for drawing deposited material
  g_ShaderFinal.init(); // final shader

  g_RT = RenderTarget2DRGBA_Ptr(new RenderTarget2DRGBA(g_RTWidth, g_RTHeight, GPUTEX_AUTOGEN_MIPMAP));
  g_RT->bind();
  glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
  LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.0f, 0.0f, 0.0f);
  g_RT->unbind();

  g_GPUMesh_sphere   = AutoPtr<MeshRenderer<mvf_mesh> >(new MeshRenderer<mvf_mesh>(shape_sphere(1.0f, 12)));
  g_GPUMesh_cylinder = AutoPtr<MeshRenderer<mvf_mesh> >(new MeshRenderer<mvf_mesh>(shape_cylinder(1.0f, 1.0f, 1.0f, 12)));

  /// default view init
  TrackballUI::trackball().set(v3f(-g_BedSize[0] / 2.0f, -g_BedSize[1] / 2.0f, -300.0f), v3f(0), quatf(v3f(1, 0, 0), -1.0f) * quatf(v3f(0, 0, 1), 0.0f));
  TrackballUI::trackball().setCenter(v3f(g_BedSize[0] / 2.0f, g_BedSize[1] / 2.0f, 10.0f));
  TrackballUI::trackball().setBallSpeed(0.0f);
  TrackballUI::trackball().setAllowRoll(false);
  TrackballUI::trackball().setUp(Trackball::Z_neg);

  /// view selection
#ifndef EMSCRIPTEN
  if (cmd_view > 0 && cmd_view <= 7) {
    std::string view_file = "trackball.F0";
    view_file += to_string(cmd_view);
    TrackballUI::trackballLoad(view_file.c_str());
  }
#endif

  SimpleUI::initImGui();
  SimpleUI::onReshape(g_ScreenWidth, g_ScreenHeight);

  // theme
#ifdef EMSCRIPTEN
  std::string theme_file = "./default.icss";
#else
#if WIN32
  TCHAR buffer[MAX_PATH] = { 0 };
  GetModuleFileName(NULL, buffer, MAX_PATH);
#else
  char buffer[256];
  size_t len = sizeof(buffer);
  int bytes = std::min(static_cast<size_t>(readlink("/proc/self/exe", buffer, len)), len - 1);
  if(bytes >= 0) { buffer[bytes] = '\0'; }
#endif
  std::string theme_file = LibSL::StlHelpers::extractPath(std::string(buffer)) + "/default.icss";
#endif

  StyleManager* style = StyleManager::get();
  if (LibSL::System::File::exists(theme_file.c_str())) {
    style->load(theme_file.c_str());
    style->push("main");
  } else {
    std::cerr << Console::red << "theme not found" << Console::gray << std::endl;
  }

#ifdef EMSCRIPTEN
  /// load gcode in editor window
  std::string command = "setupEditor();";
  emscripten_run_script(command.c_str());

  // report
  emscripten_async_wget("https://icesl.loria.fr/webprinter/report.php", "/reportresult", onLoadedData, onErrorData);
#endif

  motion_start(g_FilamentDiameter);

  printer_reset();

  glDepthFunc(GL_LEQUAL);

  /// main loop
  TrackballUI::loop();

  style->pop();

  SimpleUI::terminateImGui();
  TrackballUI::shutdown();

  return 0;
}

// ----------------------------------------------------------------

void load_gcode(std::string file) {
  if (file.empty()) {
#ifdef EMSCRIPTEN
    emscripten_run_script("parseCommandLine();\n");
    if (!g_Downloading) {
      g_GCode_path = "./icesl.gcode";
    }
#else
    g_GCode_path = openFileDialog(OFD_FILTER_GCODE);
    if (g_GCode_path.empty()) {
      std::cerr << Console::red << "No file provided - Abording!" << Console::gray << std::endl;
      exit(0);
    }
#endif
  }
  else {
    g_GCode_path = file;
  }
}

// ----------------------------------------------------------------

void gen_histogram(std::map<int, float> &map, Histogram &histo, float filter) {
  std::map<int, float> t_map = map;
  std::map<int, float> data_percent;
  std::map<int, float>::iterator it;
  // computing summ
  int summ = 0;
  for (auto _m : t_map) {
    summ += _m.second;
  }
  // computing percentages
  for (auto _m : t_map) {
    float p = (_m.second / summ);
    data_percent.emplace(std::pair<int, float>(_m.first, p));
  }
  // filter out too low values based on the filter
  for (auto _m : data_percent) {
    if (_m.second <= 1.0f - filter) {
      it = t_map.find(_m.first);
      t_map.erase(it);
    }
  }

  histo;
  int n = 0; // re-do the indexing, since we delete some entries when filtering
  for (auto _m : t_map) {
    ForIndex(i, _m.second) { // totally stupid loop, but hey, no consequence and we reuse what we have
      histo << n;
    }
    n++;
  }
}

// ----------------------------------------------------------------

void export_histogram(std::string fname, Histogram &h) {
  // prepare additionnal informations for the histogram
  std::string y_label = fname;
  // prepare full file name
  fname += ".tex";
  fname = "_" + fname;
  fname = g_GCode_path.c_str() + fname;

  // generate latex file
  ofstream file (fname);
  if (file.is_open()) {
    std::cout << Console::blue << "Generate statistics file : " << fname << Console::gray << std::endl;
    h.printAsTex(file, "", "", y_label.c_str());
    file.close();
  }
  else {
    std::cerr << Console::red << "Unable to produce "<< fname << " statistics file" << Console::gray << std::endl;
  }
}

// ----------------------------------------------------------------

string getFileName(const string& s) {
  char sep = '/';
#ifdef _WIN32
  sep = '\\';
#endif
  size_t i = s.rfind(sep, s.length());
  if (i != string::npos) {
    return(s.substr(i + 1, s.length() - i));
  }
  return("");
}

// ----------------------------------------------------------------

void printer_reset()
{
  // TODO: track state of gcode (if different, make a 1st pass link in session_start() to correctly fetch extruder number)
  gcode_reset();
  g_PrevPos      = v3d(0.0);
  g_PrevPrevPos  = v3d(0.0);

  g_NumExtruders = gcode_extruders() > 0 ? gcode_extruders() : 1;
  g_Extruders_offset.clear();

  g_Trajectory.clear();
  g_HeightSegments.clear();
  g_GlobalDepositionLength = 0.0f;

  double prev_z   = 0.0;
  double curr_z   = 0.0;
  while (gcode_line() < g_StartAtLine) {
    gcode_advance();
    if (gcode_next_pos()[2] > curr_z) {
      prev_z = curr_z;
      curr_z = gcode_next_pos()[2];
    }
  }
  g_HeightField.fill((float)prev_z);

  motion_reset(g_FilamentDiameter);

  // stats
  g_DanglingTrajectory.clear();
  g_InDangling = false;
  g_InDanglingBridging = false;
  g_DanglingHisto.clear();
  g_InOverlap = false;
  g_OverlapHisto.clear();
}

// ----------------------------------------------------------------

void session_start()
{
  gcode_start(g_GCode_string.c_str());

  // build path box (traverses the entire gcode ... a bit sad, but ...)
  g_HeightFieldBox = AAB<3>();
  while (gcode_advance()) {
#if 0
    // we ignore first layers due to purge. should be fine? hmmm
    if (gcode_next_pos()[3] > 0.0f && gcode_next_pos()[2] > g_StatsHeightThres) { 
      g_HeightFieldBox.addPoint(v3f(gcode_next_pos()));
    }
#else
    g_HeightFieldBox.addPoint(v3f(gcode_next_pos()));
#endif
  }
  g_LastLine = gcode_line();
  std::cout << "gcode has " << g_LastLine << " line(s)" << std::endl;

  // get the number of extruders used
  g_NumExtruders = gcode_extruders() > 0 ? gcode_extruders() : 1;
  // prepare the extruders offsets
  g_Extruders_offset.resize(g_NumExtruders);
  for (auto i = 0; i != g_Extruders_offset.size(); i++) {
    g_Extruders_offset[i].first = 0.0f;
    g_Extruders_offset[i].second = 0.0f;
  }

  g_FilamentDiameter = (float)gcode_filament_dia();

  // height field
  int hszx = (int)ceil(g_HeightFieldBox.extent()[0] / c_HeightFieldStep);
  int hszy = (int)ceil(g_HeightFieldBox.extent()[1] / c_HeightFieldStep);
  std::cout << "Allocated height field " << printByteSize(hszx * hszy * sizeof(float)) << std::endl;
  g_HeightField.allocate(hszx, hszy);

  // reset printer
  //printer_reset();
}

// ----------------------------------------------------------------

m4x4f alignAlongSegment(const v3f& p0, const v3f& p1)
{
  v3f d = p1 - p0;
  float l = sqrt(dot(d, d));
  v3f   u = normalize_safe(d);
  v3f   u_xy = normalize_safe(v3f(d[0], d[1], 0));
  float aglZ = atan2(u_xy[0], u_xy[1]);
  float aglX = atan2(u[2], length(v3f(u[0], u[1], 0)));
  return translationMatrix((p0 + p1) * 0.5f)
    * quatf(v3f(0, 0, 1), -aglZ).toMatrix()
    * quatf(v3f(1, 0, 0), (float)M_PI / 2.0f).toMatrix()
    * quatf(v3f(1, 0, 0), aglX).toMatrix()
    * translationMatrix(v3f(0, 0, -l / 2.0f))
    * scaleMatrix(v3f(1, 1, l));
}

// ----------------------------------------------------------------

void rasterizeDiskInHeightField(const v2i& p,float z,float r)
{
  int N = (int)round(r / c_HeightFieldStep);
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
      if (ni * ni + nj * nj < N*N) {
        float& h = g_HeightField.at<Clamp>(p[0] + ni, p[1] + nj)[0];
        h = max(h, z);
      }
    }
  }
}

void rasterizeInHeightField(v3f a, const v3f&b, float r)
{
  v3f cur(a);
  v3f step = v3f(b - a);
  float len = length(v2f(step));
  if (len < 1e-6f) {
    v2i p = v2i(
      (int)round((a[0] - g_HeightFieldBox.minCorner()[0]) / c_HeightFieldStep), 
      (int)round((a[1] - g_HeightFieldBox.minCorner()[1]) / c_HeightFieldStep));
    rasterizeDiskInHeightField(p, max(a[2],b[2]), r);
    return;
  }
  step = step / len;
  float l = 0.0f;
  while (l < len) {
    v2i p = v2i(
      (int)round((cur[0] - g_HeightFieldBox.minCorner()[0]) / c_HeightFieldStep),
      (int)round((cur[1] - g_HeightFieldBox.minCorner()[1]) / c_HeightFieldStep));
    rasterizeDiskInHeightField(p, cur[2], r);
    cur += step * c_HeightFieldStep;
    l   += c_HeightFieldStep;
  }
}

float heightAt(v3f a, float r)
{
  float h = 0.0f;
  int   N = max(1,round(r / c_HeightFieldStep));
  v2i   p = v2i(
    (int)round((a[0] - g_HeightFieldBox.minCorner()[0]) / c_HeightFieldStep),
    (int)round((a[1] - g_HeightFieldBox.minCorner()[1]) / c_HeightFieldStep));
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
        float v = g_HeightField.at<Clamp>(p[0] + ni, p[1] + nj)[0];
        if (v < a[2] - c_ThicknessEpsilon) { // ignore values at same height, these are due to aliasing
          h = max(h, v);
        }
    }
  }
  return h;
}

float danglingAt(float max_th,const v3f &a, float r)
{
  float d = 0.0f;
  int   N = max(1, round(r / c_HeightFieldStep));
  int num = 0;
  v2i p = v2i(
    (int)round((a[0] - g_HeightFieldBox.minCorner()[0]) / c_HeightFieldStep),
    (int)round((a[1] - g_HeightFieldBox.minCorner()[1]) / c_HeightFieldStep));
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
      if (ni * ni + nj * nj < N * N) {
        float v = g_HeightField.at<Clamp>(p[0] + ni, p[1] + nj)[0];
        if (v + max_th + 0.05f < a[2]) {
          d += 1.0f;
        }
        num++;
      }
    }
  }
  return d / (float)(num);
}

float overlapAt(float th, const v3f &a, float r)
{
  float o = 0.0f;
  int   N = max(1, round(r / c_HeightFieldStep));
  int num = 0;
  v2i   p = v2i(
    (int)round((a[0] - g_HeightFieldBox.minCorner()[0]) / c_HeightFieldStep),
    (int)round((a[1] - g_HeightFieldBox.minCorner()[1]) / c_HeightFieldStep));
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
      if (ni * ni + nj * nj < N * N) {
        float v = g_HeightField.at<Clamp>(p[0] + ni, p[1] + nj)[0];
        if (v + 0.01f > a[2]) {
          o += 1.0f;
        }
        num++;
      }
    }
  }
  return o / (float)(num);
}

// ----------------------------------------------------------------

bool step_simulation(bool gpu_draw)
{
  float raster_erode = c_HeightFieldStep * sqrt(2.0f);

  double step_ms = g_MmStep / (gcode_speed() / 1000.0);
  while (step_ms > 0.0f) {
    // step motion
    bool  done;
    double delta_ms = motion_step(step_ms, done);
    // accumulate step time
    step_ms -= delta_ms;

    if (done) {
      return true;
    }

    if (gcode_error()) {
#ifdef EMSCRIPTEN
      std::string command = "errorLine(" + to_string(gcode_line()) + ");";
      emscripten_run_script(command.c_str());
#endif
      return true;
    }

    // line highlighting
#ifdef EMSCRIPTEN
    static int last_line = -1;
    if (gcode_line() != last_line) {
      std::string command = "highlightLine(" + to_string(gcode_line()) + ");";
      emscripten_run_script(command.c_str());
      last_line = gcode_line();
    }
#endif
    // current position
    v3d pos   = v3d(motion_get_current_pos());
    // applying extruders offsets to pos
    if (g_NumExtruders > 1) {
      pos[0] = pos[0] + g_Extruders_offset[gcode_current_extruder()].first;
      pos[1] = pos[1] + g_Extruders_offset[gcode_current_extruder()].second;
    }
    // appliying offsets for centered bed (center of the bed is (0,0) )
    if (g_isCentered) {
      pos[0] = pos[0] + g_BedSize[0]/2;
      pos[1] = pos[1] + g_BedSize[1]/2;
    }

    // pushed material volume during time interval
    float h   = heightAt(v3f(pos), g_NozzleDiameter / 2.0f);

    static double th_prev = 0.0;
    double th = pos[2] - h;
    if (th < c_ThicknessEpsilon) {
      // cerr << 'e';
      th = th_prev;
    } else {
      th_prev = th;
    }

    if (g_ShowTrajectory) {
      g_Trajectory.push_back(pos);
    }

    double len     = length(pos - g_PrevPos);
    float dangling = 0.0f;
    float overlap  = 0.0f;

    TrajPoint tj   = TrajPoint(pos, (float)th, (float)0.0f, dangling, overlap);

    if (len > 1e-6 && !motion_is_travel()) {
      // print move
      double cs = M_PI * g_FilamentDiameter * g_FilamentDiameter / 4.0f; // mm^2
      double sa = motion_get_current_e_per_xyz() * cs;   // vf / len;
      double r  = sqrt(sa / M_PI); // sa = pi*r^2
      double squash_t = min(th / 2.0, r);
      double rs = disk_squashed_radius(r, squash_t);
      double max_th = sa / g_NozzleDiameter;

      if (!g_AutoDepositionHW) { // fixed th and radius
        th       = g_DepositionHeight;
        squash_t = th;
        rs       = g_DepositionWidth;
      }

#if 0
      if (rs > 0.3f) {
        std::cerr << sprint("z %.6f th %.6f th_prev %.6f rs %.6f \n", (float)pos[2], (float)th, (float)th_prev, (float)rs);
        sl_assert(false);
      }
#endif
      tj = TrajPoint(pos, (float)th, (float)r, dangling, overlap);

      // stats
      if (pos[2] > g_StatsHeightThres) {
        dangling = danglingAt((float)max_th, v3f(pos), (float)rs);
        overlap  = overlapAt((float)th, v3f(pos), (float)rs - raster_erode);

        // dangling only if > 60%
        dangling = max(dangling - 0.6f, 0.0f) / 0.4f;
        // overlap only if > 40%
        overlap = max(overlap - 0.4f, 0.0f) / 0.6f;
      }

      // add segment to global length
      g_GlobalDepositionLength += len;

      // add pos to dangling section if potentially bridging
      if (g_InDangling) {
        g_DanglingTrajectory.push_back(tj);
      }

      if (gpu_draw) {
        g_Bead.addPoint(v3f(pos), (float)th, (float)r, dangling, overlap, gcode_current_extruder());
      }

      // update height field
      t_height_segment seg;
      seg.a = pos;
      seg.b = g_PrevPos;
      seg.deplength = g_GlobalDepositionLength;
      seg.radius = rs;
      g_HeightSegments.push_back(seg);
    
    } else {
      if (gpu_draw) {
        g_Bead.closeAny();
      }
    }

    bool is_travel_or_dangling = motion_is_travel() || (dangling > 0.0);

    // stats
    if (g_InDangling && dangling == 0.0f) {
      // exit dangling
      g_InDangling = false;
      double dangling_len = (g_GlobalDepositionLength - g_InDanglingStart);
      if (g_AutoPause && dangling_len >= g_AutoPauseDanglingLen) {
        g_Paused = true;
      }
      dangling_len = round(dangling_len / 0.1); // quantize
      g_DanglingHisto[(int)dangling_len] += 1.0f;
      // bridge?
      bool is_bridge = false;
      if (!motion_is_travel() && g_InDanglingBridging  // attached on both ends
        && g_DanglingTrajectory.size() >= 2) {
        is_bridge = true;
        // verify deviation
        v2d delta = normalize_safe(v2d(g_DanglingTrajectory.back().pos - g_DanglingTrajectory.front().pos));
        v2d nrm   = v2d(-delta[1], delta[0]);
        for (auto p : g_DanglingTrajectory) {
          float dev = dot(normalize_safe(v2d(p.pos - g_DanglingTrajectory.front().pos)), nrm);
          if (abs(dev) > g_NozzleDiameter / 10.0f) {
            is_bridge = false; break;
          }
        }
        if (is_bridge) {
          //g_Paused = true;
          //g_Trajectory.clear();
          //for (auto p : g_DanglingTrajectory) {
          //  g_Trajectory.push_back(p.pos);
          //}
          if (gpu_draw) {
            // redraw orange
            g_Bead.closeAny();
            g_Bead.setIsBridge(true);
            for (auto p : g_DanglingTrajectory) {
              g_Bead.addPoint(v3f(p.pos), (float)p.th, (float)p.r, 0.0f, 0.0f, gcode_current_extruder());
            }
            g_Bead.closeAny();
            g_Bead.setIsBridge(false);
          }
        }
      }
      g_DanglingTrajectory.clear();
    }
    if (g_InOverlap && overlap == 0.0f) {
      // exit overlap
      g_InOverlap = false;
      double overlap_len = (g_GlobalDepositionLength - g_InOverlapStart);
      if (g_AutoPause && overlap_len >= g_AutoPauseOverlapLen) {
        g_Paused = true;
      }
      overlap_len = round(overlap_len / 0.1); // quantize
      g_OverlapHisto[(int)overlap_len] += 1.0f;
    }
    if (!g_InDangling && dangling > 0.0) {
      // enter dangling
      g_DanglingTrajectory.clear();
      // PB FIXME: this assert should be needed , but is disabled to comply with etruders offsets
      //sl_assert(tj.r > 0.0f);
      g_DanglingTrajectory.push_back(tj);
      g_InDangling          = true;
      g_InDanglingStart     = g_GlobalDepositionLength;
      g_InDanglingBridging  = !g_PrevWasTravelOrDangling;
    }
    if (!g_InOverlap && overlap > 0.0) {
      // enter overlap
      g_InOverlap      = true;
      g_InOverlapStart = g_GlobalDepositionLength;
    }

#if 1
    // height segments (with delay)
    auto S = g_HeightSegments.begin();
    while (S != g_HeightSegments.end()) {
      if ( S->deplength + max((double)g_NozzleDiameter,g_MmStep) * 4.0 < g_GlobalDepositionLength
        || max(S->a[2],S->b[2]) < pos[2]
        ) {
        rasterizeInHeightField(v3f(S->a), v3f(S->b), (float)S->radius /*- raster_erode*/); // uncomment to visualize raster erode
        S = g_HeightSegments.erase(S);
      } else {
        // S++;
        break; // monotonous so no need to continue
      }
    }
#endif

    // prepare next
    g_PrevWasTravelOrDangling = is_travel_or_dangling;
    g_PrevPrevPos = g_PrevPos;
    g_PrevPos = pos;

  } // iter
  return false;
}

// ----------------------------------------------------------------

#ifdef EMSCRIPTEN

bool fileChanged(std::string file, time_t& _last)
{
  struct stat st;
  int fh = open(file.c_str(), O_RDONLY);
  if (fh < 0) return false;
  fstat(fh, &st);
  close(fh);
  if (st.st_mtime != _last) {
    _last = st.st_mtime;
    cerr << "[vrprinter] detected change in " << file << endl;
    return true;
  }
  return false;
}

#endif

void mainWindowReshape(uint w, uint h) {
  g_ScreenWidth = w;
  g_ScreenHeight = h;
  g_RenderWidth = w - g_UIWidth;
  g_RenderHeight = h;

  printer_reset();
  g_ForceRedraw = true;
  
  /*
  std::cerr << Console::green << w << " x " << h << " size on listener" << Console::gray << std::endl;
  std::cerr << Console::blue << g_ScreenWidth << " x " << g_ScreenHeight << " screen size" << Console::gray << std::endl;
  std::cerr << Console::red << g_RenderWidth << " x " << g_RenderHeight << " render size" << Console::gray << std::endl;
  std::cerr << Console::magenta << g_UIWidth << " ui width" << Console::gray << std::endl;
  */
}

void mainRender()
{
  static t_time tm_lastChange = milliseconds();
  static t_time tm_lastFrame = milliseconds();
  t_time tm_now  = milliseconds();
  t_time elapsed = tm_now - tm_lastFrame;
  tm_lastFrame   = tm_now;

#ifdef EMSCRIPTEN
  if (fileChanged("/icesl.gcode", g_FileStamp)) {
    g_GCode_string = loadFileIntoString("/icesl.gcode");
    session_start();
    motion_start(g_FilamentDiameter);
    g_ForceRedraw = true;
  }
#endif

  if (!g_FatalError) {

    AAB<3> bx;
    bx.addPoint(-v3f(100.0));
    bx.addPoint( v3f(100.0));
    float ex = tupleMax(bx.extent());
    m4x4f proj = perspectiveMatrixGL<float>((float)M_PI / 6.0f, 1.0f, ZNear, ZFar);

    g_Zoom = g_Zoom + 0.1f * (g_ZoomTarget - g_Zoom);
    TrackballUI::trackball().setRadius(ex / (g_Zoom*g_Zoom));

    // view matrix
    m4x4f view =
        translationMatrix(v3f(0, 0, g_Zoom))
      * TrackballUI::matrix()
      * translationMatrix(-bx.center());

    // should we redraw?
    bool redraw = true;
    if (length(g_LastView - view) < 1e-6f) {
      redraw = false;
    }
    g_LastView = view;

    // offscreen rendering
    g_RT->bind();
    glViewport(0, 0, g_RenderWidth, g_RenderHeight);

    if (redraw || g_ForceRedraw) {
      // clear on redraw
      g_ForceRedraw = false;
      LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.0f, 0.0f, 0.0f);
      // reset motion
      printer_reset();
      // unpause
      g_Paused = false;
    }
    if (g_ForceClear) {
      // clear
      g_ForceClear = false;
      LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.0f, 0.0f, 0.0f);
    }

    glEnable(GL_CULL_FACE);

    g_ShaderDeposition.begin();
    g_ShaderDeposition.u_projection.set(proj);
    g_ShaderDeposition.u_view.set(view);
    g_ShaderDeposition.u_ZNear.set(ZNear);
    g_ShaderDeposition.u_ZFar.set(ZFar);

    if (!g_Paused) {
      int n = max(1,(int)round(g_UserMmStep / g_MmStep));
      ForIndex(sub, n) {
        step_simulation(true);
        if (g_Paused) break;
      }
    }

    g_ShaderDeposition.end();

    g_RT->unbind();

    if (g_ShowTrajectory) {
      // trajectory
      while (g_Trajectory.size() > 256) {
        g_Trajectory.erase(g_Trajectory.begin());
      }
      // erase one each time
      if (!g_Trajectory.empty()) {
        g_Trajectory.erase(g_Trajectory.begin());
      }
    }

    if (!redraw) {
      tm_lastChange = milliseconds();
    }

    // clear screen
    glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.1f, 0.1f, 0.1f);

    // final pass
    g_ShaderFinal.begin();
    g_ShaderFinal.u_projview.set(orthoMatrixGL(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
    g_ShaderFinal.u_texpts.set(0);
    g_ShaderFinal.u_pixsz.set(v3f(1.0f / (float)g_RTWidth, 1.0f / (float)g_RTWidth, 0.0f)); // squared and power of 2 tex to increase compatibility
    g_ShaderFinal.u_texscl.set(v2f(g_RenderWidth / (float)g_RTWidth, g_RenderHeight / (float)g_RTHeight)); // to adapt screen texture to the tex coord. one above
    g_ShaderFinal.u_ZNear.set(0.01f);
    g_ShaderFinal.u_ZFar.set(1000.0f);
    g_ShaderFinal.u_color_overhangs.set(g_ColorOverhangs ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_RT->texture());
    g_GPUMesh_quad->render();
    g_ShaderFinal.end();

    // render trajectory
    if (g_ShowTrajectory && !g_Trajectory.empty()) {
    // if (!g_Trajectory.empty()) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_DEPTH_TEST);
      g_ShaderSimple.begin();
      g_ShaderSimple.u_projection.set(proj);
      g_ShaderSimple.u_view.set(view);
      g_ShaderSimple.u_color.set(v4f(0, 1, 0, 1));
      v3d prev = g_Trajectory.front();
      ForRange(t, 1, (int)g_Trajectory.size() - 1) {
        g_ShaderSimple.u_alpha.set(t / (float)((int)g_Trajectory.size() - 1));
        v3d pos = g_Trajectory[t];
        // draw cylinder
        g_ShaderSimple.u_view.set(
          view
          *alignAlongSegment(v3f(prev), v3f(pos))
          *scaleMatrix(v3f(0.1f, 0.1f, 1))
        );
        g_GPUMesh_cylinder->render();
        prev = pos;
      }
      g_ShaderSimple.end();
      glEnable(GL_DEPTH_TEST);
    }

    if (g_DumpHeightField) {
      ImageRGB img(g_HeightField.xsize(), g_HeightField.ysize());
      ForImage((&img), i, j) {
        img.pixel(i, j) = uchar(frac(g_HeightField.at(i, j)[0]) * 255.0f);
      }
      static int cnt = 0;
      // Warning! the folder must be prepared !
      // Note: shouldn't it be tied to line number / layer height?
      saveImage(sprint("%s_dump\\hfield_%04d.png", g_GCode_path.c_str(), cnt++), &img);
      std::cerr << "Height Fields dumped at : " << g_GCode_path << "_dump\\" << std::endl;
      if (g_GlobalDepositionLength - g_DumpHeightFieldStartLen > 10.0f) {
        g_DumpHeightField = false;
      }
    }

#if 0
    //////////////////////////////////////////////////////////////
    glViewport(g_UIWidth, 0, g_RenderWidth /4, g_RenderHeight /4);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_RT->texture());
    AutoPtr<Tex2DLum32F> texh(new Tex2DLum32F(g_HeightField));
    LIBSL_GL_CHECK_ERROR;
    g_ShaderSimple.begin();
    g_ShaderSimple.u_projection.set(proj);
    g_ShaderSimple.u_view.set(view);
    g_ShaderSimple.u_color.set(v4f(0, 0, 1, 1));
    g_ShaderSimple.u_alpha.set(1.0f);
    LIBSL_GL_CHECK_ERROR;
    g_ShaderSimple.u_use_tex.set(1);
    g_ShaderSimple.u_tex.set(1);
    LIBSL_GL_CHECK_ERROR;
    g_ShaderSimple.u_projection.set(orthoMatrixGL(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
    g_ShaderSimple.u_view.set(m4x4f::identity());
    g_GPUMesh_quad->render();
    g_ShaderSimple.u_use_tex.set(0);
    LIBSL_GL_CHECK_ERROR;
    g_ShaderSimple.end();
    LIBSL_GL_CHECK_ERROR;
    glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
    //////////////////////////////////////////////////////////////
#endif

    // render bed
    bed_render(proj,view,g_BedSize[0], g_BedSize[1]);

    // render axis
    {
      m4x4f proj = orthoMatrixGL<float>(
        bx.center()[0] - ex * 0.8f, bx.center()[0] + ex * 0.8f,
        bx.center()[1] - ex * 0.8f, bx.center()[1] + ex * 0.8f,
        bx.center()[2] - ex * 4.0f, bx.maxCorner()[2] + ex * 4.0f);
      int axis_size = g_ScreenWidth / 8;
      glViewport(g_UIWidth, 0, axis_size, axis_size);
      g_ShaderSimple.begin();
      g_ShaderSimple.u_projection.set(proj);
      g_ShaderSimple.u_view.set(translationMatrix(-view.mulPoint(v3f(0))) * view * scaleMatrix(v3f(4.0f)));
      g_ShaderSimple.u_color.set(v4f(0));
      g_ShaderSimple.u_alpha.set(1.0f);
      glDisable(GL_CULL_FACE);
      g_GPUMesh_axis->render();
      g_ShaderSimple.end();
      glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
    }

  } else if (g_FatalError) { // fatal error

    glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.7f, 0.5f, 0.5f);

  } else {

    glViewport(g_UIWidth, 0, g_RenderWidth, g_RenderHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.1f, 0.1f, 0.1f);

  }

  if (!g_FatalError || g_FatalErrorAllowRestart) {
#ifdef EMSCRIPTEN
//        std::string command = "reportError(" + to_string(g_Script->lastErrorLine()) + ",'" + jsEncodeString(g_Script->lastErrorMessage().c_str()) + "');";
//        emscripten_run_script(command.c_str());
#endif
  }

  // interface
  glViewport(0, 0, g_UIWidth, g_ScreenHeight);
  ImGuiPanel();
}

// ----------------------------------------------------------------

void makeAxisMesh()
{
  g_GPUMesh_axis = AutoPtr<SimpleMesh>(new SimpleMesh());
  g_GPUMesh_axis->begin(GPUMESH_TRIANGLELIST);
  addBar(g_GPUMesh_axis, v3f(0, 0, 0), v3f(10, 0, 0), make_pair(v3f(0, 1, 0), v3f(0, 0, 1)), 0.5f);
  addBar(g_GPUMesh_axis, v3f(0, 0, 0), v3f(0, 10, 0), make_pair(v3f(1, 0, 0), v3f(0, 0, 1)), 0.5f);
  addBar(g_GPUMesh_axis, v3f(0, 0, 0), v3f(0, 0, 10), make_pair(v3f(1, 0, 0), v3f(0, 1, 0)), 0.5f);
  g_GPUMesh_axis->end();
}

// ----------------------------------------------------------------

static void HelpMarker(const char* desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

// ----------------------------------------------------------------

void ImGuiPanel()
{
  if (!g_FatalError) {
    // imgui panel flags
    ImGuiWindowFlags panel_flags = 0;
    panel_flags |= ImGuiWindowFlags_NoTitleBar;
    panel_flags |= ImGuiWindowFlags_NoMove;
    panel_flags |= ImGuiWindowFlags_NoResize;
    panel_flags |= ImGuiWindowFlags_NoCollapse;

    // imgui panel size & pos
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2((float)g_UIWidth, g_ScreenHeight), ImGuiSetCond_Once);

    // creating imgui panel
    ImGui::Begin("Virtual 3D printer", NULL, panel_flags);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

    // file management section
#ifndef EMSCRIPTEN
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("File")) {
      if (ImGui::Button("Load a new Gcode")) {
        load_gcode();
        g_GCode_string = loadFileIntoString(g_GCode_path.c_str());
        g_FilamentDiameter = 1.75f;
        g_NozzleDiameter = 0.4f;
        session_start();
        motion_start(g_FilamentDiameter);
        printer_reset();
        g_ForceRedraw = true;
      }
    }
#else
    /*
    // TODO move to a download_file() function
    if (g_Downloading) { // g_Downloading never used ?
      ImGui::SetNextWindowSize(ImVec2(300, 50));
      ImGui::SetNextWindowPosCenter();
      ImGui::Begin("Downloading ...");
      ImGui::ProgressBar(g_DownloadProgress);
      ImGui::End();
    }
    */
#endif

    // printer setup section
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Printer")) {
      // filament diameter
      ImGui::InputFloat("Filament diameter", &g_FilamentDiameter, 0.0f, 0.0f, 3);
      g_FilamentDiameter = std::clamp(g_FilamentDiameter, 0.1f, 10.0f);
      // nozzle diameter
      ImGui::InputFloat("Nozzle diameter", &g_NozzleDiameter, 0.0f, 0.0f, 3);
      g_NozzleDiameter = std::clamp(g_NozzleDiameter, c_HeightFieldStep * 2.0f, 10.0f);
      // extruders
      ImGui::InputInt("Number of extruders", &g_NumExtruders, 1, 1);
      if (g_NumExtruders > 1) {
        ImGui::Text("Extruders detected: %d", g_NumExtruders);
        // extruders offsets (extruder 0 is ommited, as it should be the reference)
        for (int i = 1; i != g_NumExtruders; i++) {
          if (ImGui::TreeNode(("Offsets for Extruder " + std::to_string(i)).c_str())) {
            ImGui::InputFloat("X Offset", &g_Extruders_offset[i].first, 0.1f, 0.5f, "%.3f");
            ImGui::InputFloat("Y Offset", &g_Extruders_offset[i].second, 0.1f, 0.5f, "%.3f");
            ImGui::TreePop();
          }
        }
      }
      // bed dimmensions
      ImGui::InputFloat("Bed X size", &g_BedSize[0], 0.5f, 1.0f, "%.3f");
      ImGui::InputFloat("Bed Y size", &g_BedSize[1], 0.5f, 1.0f, "%.3f");
      // volumetric extrusion
      ImGui::Checkbox("Bed center is (0,0)", &g_isCentered);
    }

    // control
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Control")) {
      // start line
      ImGui::InputInt("Start at GCode line", &g_StartAtLine);
      g_StartAtLine = max(0, min(g_StartAtLine, g_LastLine - 1));
      // animation step (mm/step)
      ImGui::SliderFloat("Step (mm)", &g_UserMmStep, 0.001f, 1000.0f, "%.3f", 3.0f);
      // control buttons
      if (ImGui::Button("Reset")) {
        printer_reset();
        g_ForceRedraw = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear below")) {
        g_ForceClear = true;
      }
#ifndef EMSCRIPTEN
      ImGui::SameLine();
      if (ImGui::Button("Dump")) {
        g_DumpHeightField = true;
        g_DumpHeightFieldStartLen = g_GlobalDepositionLength;
      }
#endif
      // pause button
      if (g_Paused) {
        if (ImGui::Button("Resume")) {
          g_Paused = false;
        }
      } else {
        if (ImGui::Button("Pause")) {
          g_Paused = true;
        }
      }
      // auto pause
      ImGui::SameLine();
      ImGui::Checkbox("Auto pause", &g_AutoPause);
      if (g_AutoPause) {
        ImGui::InputFloat("Overhang >", &g_AutoPauseDanglingLen, 1.0f, 1000.0f);
        ImGui::SameLine(); HelpMarker("Will auto-pause when the detected overhang value is higher than the threshold");
        ImGui::InputFloat("Overlap >", &g_AutoPauseOverlapLen, 1.0f, 1000.0f);
        ImGui::SameLine(); HelpMarker("Will auto-pause when the detected overlap value is higher than the threshold");
      }
      // show trajectory (virtual nozzle)
      ImGui::Checkbox("Show trajectory", &g_ShowTrajectory);
      // show overlaps
      ImGui::Checkbox("Show overlaps and overhangs", &g_ColorOverhangs);
      ImGui::SameLine(); HelpMarker("Overlaps -> blue \nOverhangs -> red");

      ImGui::Checkbox("Automatic deposition height & width", &g_AutoDepositionHW);
      ImGui::SameLine(); HelpMarker("The deposition height & width will be automatically processed depending on coordinates, flow, filament diameter and nozzle diameter");
      if (!g_AutoDepositionHW) {
        ImGui::InputFloat("Deposition Height", &g_DepositionHeight, 0.0f, 0.0f, 3);
        ImGui::InputFloat("Deposition Width", &g_DepositionWidth, 0.0f, 0.0f, 3);
      }
    }

    // status
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Status")) {
      // current gcode line
      int line = gcode_line();
      ImGui::InputInt("GCode line", &line, 1, 100, ImGuiInputTextFlags_ReadOnly);
      // current gcode pos
      static v3f pos;
      pos = v3f(motion_get_current_pos());
      ImGui::InputFloat3("XYZ (mm)", &pos[0]);
      // flow graph
      double flow = motion_get_current_flow() * 1000.0;
      g_FlowsSample += (float)flow;
      g_FlowsCount++;
      if (g_FlowsCount == 32) {
        g_FlowsSample /= (float)g_FlowsCount;
        g_Flows.erase(g_Flows.begin());
        g_Flows.push_back(g_FlowsSample);
        g_FlowsCount = 0;
        g_FlowsSample = 0.0f;
      }
      ImGui::PlotLines("Flow (mm^3/sec)", &g_Flows[0], (int)g_Flows.size());
      // speed graph
      float speed = (float)gcode_speed();
      g_SpeedsSample += speed;
      g_SpeedsCount++;
      if (g_SpeedsCount == 32) {
        g_SpeedsSample /= (float)g_SpeedsCount;
        g_Speeds.erase(g_Speeds.begin());
        g_Speeds.push_back(g_SpeedsSample);
        g_SpeedsCount = 0;
        g_SpeedsSample = 0.0f;
      }
      ImGui::PlotLines("Speed (mm/sec)", &g_Speeds[0], (int)g_Speeds.size());
      // dangling histogram
      {
        static std::vector<float> histo; /// oh this is ugly, very ugly
        int maxv = 0;
        for (auto h : g_DanglingHisto) {
          maxv = max(maxv, h.first);
        }
        histo.resize(maxv + 1);
        for (auto h : g_DanglingHisto) {
          histo[h.first] = h.second;
        }
        ImGui::PlotHistogram("dangling (red) ", &histo[0], (int)histo.size());
      }
      // overlap histogram
      {
        static std::vector<float> histo; /// oh this is ugly, very ugly
        int maxv = 0;
        for (auto h : g_OverlapHisto) {
          maxv = max(maxv, h.first);
        }
        histo.resize(maxv + 1);
        for (auto h : g_OverlapHisto) {
          histo[h.first] = h.second;
        }
        ImGui::PlotHistogram("overlaps (blue)", &histo[0], (int)histo.size());
      }
    }
    ImGui::End();
  }
  else { // fatal error
    ImGui::SetNextWindowSize(ImVec2(600, 50));
    ImGui::SetNextWindowPosCenter();
    ImGui::Begin("Unsupported");
    ImGui::Text("%s", g_FatalErrorMessage.c_str());
    ImGui::End();
  }

  ImGui::Render();
}

// ----------------------------------------------------------------

void mainKeyboard(unsigned char key)
{
  if (key == 'e')      g_ZoomTarget = g_ZoomTarget*1.1f;
  else if (key == 'r') g_ZoomTarget = g_ZoomTarget / 1.1f;
}

// ----------------------------------------------------------------

void mainMouseButton(uint x, uint y, uint btn, uint flags)
{
  g_Rotating = false;

  if (btn == LIBSL_LEFT_BUTTON || btn == LIBSL_RIGHT_BUTTON) {
    if (flags == LIBSL_BUTTON_DOWN) {
      g_Dragging = true;
    } else if (flags == LIBSL_BUTTON_UP) {
      g_Dragging = false;
    }
  }

  if (btn == LIBSL_WHEEL_UP) {
    g_ZoomTarget = g_ZoomTarget*1.05f;
    std::cout << Console::red << "zoom in" << Console::gray << std::endl;
  } else if (btn == LIBSL_WHEEL_DOWN) {
    g_ZoomTarget = g_ZoomTarget / 1.05f;
    std::cout << Console::green << "zoom in" << Console::gray << std::endl;
  }
}

// ----------------------------------------------------------------

void mainMouseWheel(int incr)
{
  incr = -25 * sign(incr);
  v3f up = TrackballUI::trackball().up();
  v3f tz = v3f(up[0] * incr, up[1] * incr, up[2] * incr);
  TrackballUI::trackball().translation() = TrackballUI::trackball().translation() + tz ;
}

// ----------------------------------------------------------------
