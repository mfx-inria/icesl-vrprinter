#include <imgui.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifndef EMSCRIPTEN
#ifdef _MSC_VER
#define glActiveTexture glActiveTextureARB
#endif
#endif

#include <LibSL.h>
#include <LibSL_gl.h>

LIBSL_WIN32_FIX;

// ----------------------------------------------------------------

#include "bed.h"
#include "shapes.h"
#include "gcode.h"
#include "motion.h"
#include "sphere_squash.h"

// ----------------------------------------------------------------

using namespace std;

// ----------------------------------------------------------------

int           g_UIWidth = 350;

int           g_ScreenWidth = 700;
int           g_ScreenHeight = 700;
int           g_RenderWidth = 700;
int           g_RenderHeight = 700;

int           g_RTWidth  = 1024;
int           g_RTHeight = 1024;

v2f           g_BedSize(200.0f, 150.0f);
float         g_FilamentDiameter = 1.75f;
float         g_NozzleDiameter = 0.4f;

float         g_TimeStep = 500.0f;
float         g_GlobalTime = 0.0f;

int           g_StartAtLine = 0;
bool          g_ColorOverhangs = false;

std::string   g_GCode_string;

typedef GPUMESH_MVF1(mvf_vertex_3f)                mvf_mesh;
typedef GPUMesh_VertexBuffer<mvf_mesh>             SimpleMesh;

AutoPtr<SimpleMesh>                      g_GPUMesh_quad;
AutoPtr<SimpleMesh>                      g_GPUMesh_axis;

AutoPtr<MeshRenderer<mvf_mesh> >         g_GPUMesh_sphere;
AutoPtr<MeshRenderer<mvf_mesh> >         g_GPUMesh_cylinder;

#include "simple.h"
AutoBindShader::simple     g_ShaderSimple;
#include "final.h"
AutoBindShader::final      g_ShaderFinal;
#include "deposition.h"
AutoBindShader::deposition g_ShaderDeposition;

RenderTarget2DRGBA_Ptr g_RT;

m4x4f  g_LastView = m4x4f::identity();
bool   g_Rotating = false;
bool   g_Dragging = false;
float  g_Zoom = 1.0f;
float  g_ZoomTarget = 1.0f;

std::vector<v3f>                 g_Trajectory;

typedef struct 
{
  float time;
  float radius;
  v3f   a;
  v3f   b;
} t_height_segment;

std::list<t_height_segment> g_HeightSegments;

const float c_HeightFieldStep = 0.1f; // mm
Array2D<float>   g_HeightField;

v3f    g_PrevPos(0.0f);

bool   g_ForceRedraw = true;
bool   g_FatalError = false;
bool   g_FatalErrorAllowRestart = false;
string g_FatalErrorMessage = "unkonwn error";

// ----------------------------------------------------------------

template <class T_Mesh>
void addBar(AutoPtr<T_Mesh> gpumesh, v3f a, v3f b, pair<v3f, v3f> uv, float sz = 0.1f)
{
  v3f a00 = (a - sz * uv.first - sz * uv.second);
  v3f a01 = (a - sz * uv.first + sz * uv.second);
  v3f a11 = (a + sz * uv.first + sz * uv.second);
  v3f a10 = (a + sz * uv.first - sz * uv.second);
  v3f b00 = (b - sz * uv.first - sz * uv.second);
  v3f b01 = (b - sz * uv.first + sz * uv.second);
  v3f b11 = (b + sz * uv.first + sz * uv.second);
  v3f b10 = (b + sz * uv.first - sz * uv.second);

  gpumesh->vertex_3(a00[0], a00[1], a00[2]);
  gpumesh->vertex_3(b00[0], b00[1], b00[2]);
  gpumesh->vertex_3(b10[0], b10[1], b10[2]);
  //
  gpumesh->vertex_3(a00[0], a00[1], a00[2]);
  gpumesh->vertex_3(b10[0], b10[1], b10[2]);
  gpumesh->vertex_3(a10[0], a10[1], a10[2]);

  gpumesh->vertex_3(a10[0], a10[1], a10[2]);
  gpumesh->vertex_3(b11[0], b11[1], b11[2]);
  gpumesh->vertex_3(b10[0], b10[1], b10[2]);
  //
  gpumesh->vertex_3(a10[0], a10[1], a10[2]);
  gpumesh->vertex_3(a11[0], a11[1], a11[2]);
  gpumesh->vertex_3(b11[0], b11[1], b11[2]);

  gpumesh->vertex_3(a11[0], a11[1], a11[2]);
  gpumesh->vertex_3(b01[0], b01[1], b01[2]);
  gpumesh->vertex_3(b11[0], b11[1], b11[2]);
  //
  gpumesh->vertex_3(a11[0], a11[1], a11[2]);
  gpumesh->vertex_3(a01[0], a01[1], a01[2]);
  gpumesh->vertex_3(b01[0], b01[1], b01[2]);

  gpumesh->vertex_3(a01[0], a01[1], a01[2]);
  gpumesh->vertex_3(b00[0], b00[1], b00[2]);
  gpumesh->vertex_3(b01[0], b01[1], b01[2]);
  //
  gpumesh->vertex_3(a00[0], a00[1], a00[2]);
  gpumesh->vertex_3(b00[0], b00[1], b00[2]);
  gpumesh->vertex_3(a01[0], a01[1], a01[2]);
}

// ----------------------------------------------------------------

void printer_reset()
{
  gcode_reset();
  g_PrevPos = v3f(0.0f);
  g_Trajectory.clear();
  g_HeightSegments.clear();
  g_GlobalTime = 0.0f;
  float prev_z = 0.0f;
  float curr_z = 0.0f;
  while (gcode_line() < g_StartAtLine) {
    gcode_advance();
    if (gcode_next_pos()[2] > curr_z) {
      prev_z = curr_z;
      curr_z = gcode_next_pos()[2];
    }
  }
  g_HeightField.fill(prev_z);
  motion_reset(g_FilamentDiameter);
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

bool                                     g_Downloading = false;
float                                    g_DownloadProgress = 0.0f;

// ----------------------------------------------------------------

std::vector<float> g_Flows(64,0.0f);
int                g_FlowsCount = 0;
float              g_FlowsSample = 0.0f;
std::vector<float> g_Speeds(64,0.0f);
int                g_SpeedsCount = 0;
float              g_SpeedsSample = 0.0f;

// ----------------------------------------------------------------

void ImGuiPanel()
{

  if (g_Downloading) {
    ImGui::SetNextWindowSize(ImVec2(300, 50));
    ImGui::SetNextWindowPosCenter();
    ImGui::Begin("Downloading ...");
    ImGui::ProgressBar(g_DownloadProgress);
    ImGui::End();
  }

  if (!g_FatalError) {

    // imgui
    if (g_Downloading) {
      ImGui::SetNextWindowSize(ImVec2(300, 50));
      ImGui::SetNextWindowPosCenter();
      ImGui::Begin("Downloading ...");
      ImGui::ProgressBar(g_DownloadProgress);
      ImGui::End();
    }
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(g_UIWidth, 750), ImGuiSetCond_Once);
    ImGui::Begin("Virtual 3D printer");
    ImGui::PushItemWidth(200);

    // printer setup
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Printer")) {      
      ImGui::InputFloat("Filament diameter", &g_FilamentDiameter, 0.0f, 0.0f, 3);
      g_FilamentDiameter = clamp(g_FilamentDiameter, 0.1f, 10.0f);
      static float nozzleDiameter = g_NozzleDiameter;
      ImGui::InputFloat("Nozzle diameter", &nozzleDiameter, 0.0f, 0.0f, 3);
      g_NozzleDiameter = clamp(nozzleDiameter, c_HeightFieldStep*2.0f, 10.0f);      
      ImGui::InputInt("Start at GCode line", &g_StartAtLine);
      if (ImGui::Button("Reset")) {
        g_NozzleDiameter = nozzleDiameter;
        printer_reset();
        g_ForceRedraw = true;
      }
    }
    // control
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Control")) {
      ImGui::SliderFloat("Time step (msec)", &g_TimeStep, 0.1f, 3000.0f,"%.1f",3.0f);
      ImGui::Checkbox("Color overhangs", &g_ColorOverhangs);
    }
    // status
    ImGui::SetNextTreeNodeOpen(true);
    if (ImGui::CollapsingHeader("Status")) {
      int line = gcode_line();
      ImGui::InputInt("GCode line", &line);
      ImGui::InputFloat3("XYZ (mm)", &motion_get_current_pos()[0]);
      // flow graph
      float flow = motion_get_current_flow() * 1000.0f;
      g_FlowsSample += flow;
      g_FlowsCount++;
      if (g_FlowsCount == 32) {
        g_FlowsSample /= (float)g_FlowsCount;
        g_Flows.erase(g_Flows.begin());
        g_Flows.push_back(g_FlowsSample);
        g_FlowsCount = 0;
        g_FlowsSample = 0.0f;
      }      
      ImGui::PlotLines("Flow (mm^3/sec)", &g_Flows[0], g_Flows.size());
      // speed graph
      float speed = gcode_speed();
      g_SpeedsSample += speed;
      g_SpeedsCount++;
      if (g_SpeedsCount == 32) {
        g_SpeedsSample /= (float)g_SpeedsCount;
        g_Speeds.erase(g_Speeds.begin());
        g_Speeds.push_back(g_SpeedsSample);
        g_SpeedsCount = 0;
        g_SpeedsSample = 0.0f;
      }
      ImGui::PlotLines("Speed (mm/sec)", &g_Speeds[0], g_Speeds.size());
    }
    ImGui::End();

  } else { // fatal error

    ImGui::SetNextWindowSize(ImVec2(600, 50));
    ImGui::SetNextWindowPosCenter();
    ImGui::Begin("Unsupported");
    ImGui::Text("%s", g_FatalErrorMessage.c_str());
    ImGui::End();

  }

  ImGui::Render();

}

// ----------------------------------------------------------------

string jsEncodeString(const char *strin)
{
  string str;
  int i = 0;
  while (strin[i] != '\0') {
    if (strin[i] == '\\') {
      str = str + "\\\\";
    } else if (strin[i] == '\n') {
      str = str + "\\n";
    } else if (strin[i] == '\r') {
      
    } else if (strin[i] == '&') {
      str = str + "\\&";
    } else if (strin[i] == '\'') {
      str = str + "\\'";
    } else {
      str = str + strin[i];
    }
    i++;
  }
  return str;
}

// ----------------------------------------------------------------

void rasterizeDiskInHeightField(const v2i& p,float z,float r)
{
  int N = ceil(r / c_HeightFieldStep);
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
      if (sqrt((float)(ni * ni + nj * nj)) < N) {
        float& h = g_HeightField.at<Clamp>(p[0] + ni, p[1] + nj);
        h = max(h, z);
      }
    }
  }
}

void rasterizeInHeightField(const v3f& a, const v3f&b, float r)
{
  v3f cur(a);
  v3f step = v3f(b - a);
  float len = length(v2f(step));
  if (len < 1e-6f) {
    v2i p = v2i(round((a[0]) / c_HeightFieldStep), round((a[1]) / c_HeightFieldStep));
    rasterizeDiskInHeightField(p, max(a[2],b[2]), r);
    return;
  }
  step = step / len;
  float l = 0.0f;
  while (l < len) {
    v2i p = v2i(round((cur[0]) / c_HeightFieldStep), round((cur[1]) / c_HeightFieldStep));
    rasterizeDiskInHeightField(p, cur[2], r);
    cur += step * c_HeightFieldStep;
    l += c_HeightFieldStep;
  }
}

float heightAt(const v3f& a, float r)
{
  float h = 0.0f;
  int N = max(1,ceil(r / c_HeightFieldStep));
  ForRange(nj, -N, N) {
    ForRange(ni, -N, N) {
      v2i p = v2i(round(a[0] / c_HeightFieldStep), round(a[1] / c_HeightFieldStep));
      float v = g_HeightField.at<Clamp>(p[0], p[1]);
      h = max(h, v);
    }
  }
  return h;
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
  return translationMatrix((p0 + p1)*0.5f)
    * quatf(v3f(0, 0, 1), -aglZ).toMatrix()
    * quatf(v3f(1, 0, 0), (float)M_PI / 2.0f).toMatrix()
    * quatf(v3f(1, 0, 0), aglX).toMatrix()
    * translationMatrix(v3f(0, 0, -l/2.0f))
    * scaleMatrix(v3f(1, 1, l));
}

// ----------------------------------------------------------------

const float ZNear = 0.1f;
const float ZFar  = 500.0f;

void mainRender()
{
  static t_time tm_lastChange = milliseconds();
  static t_time tm_lastFrame = milliseconds();
  t_time tm_now  = milliseconds();
  t_time elapsed = tm_now - tm_lastFrame;
  tm_lastFrame   = tm_now;

#ifdef EMSCRIPTEN
  if (LibSL::System::File::exists("/print.gcode")) {
    g_GCode_string = loadFileIntoString("/print.gcode");
    gcode_start(g_GCode_string.c_str());
    motion_start(g_FilamentDiameter);
    std::remove("/print.gcode");
    g_ForceRedraw = true;
  }
#endif

  if (!g_FatalError) {

    AAB<3> bx;
    bx.addPoint(-v3f(100.0));
    bx.addPoint( v3f(100.0));
    float ex = tupleMax(bx.extent());
    m4x4f proj = perspectiveMatrixGL<float>((float)M_PI / 6.0f, 1.0f, ZNear, ZFar);
      //bx.center()[0] - ex * 0.8f, bx.center()[0] + ex * 0.8f,
      //bx.center()[1] - ex * 0.8f, bx.center()[1] + ex * 0.8f,
      //bx.center()[2] - ex * 4.0f, bx.maxCorner()[2] + ex * 4.0f);

    g_Zoom = g_Zoom + 0.1f * (g_ZoomTarget - g_Zoom);
    TrackballUI::trackball().setRadius(ex / (g_Zoom*g_Zoom));

    // view matrix
    m4x4f view = scaleMatrix(v3f(g_Zoom)) * TrackballUI::matrix() * translationMatrix(-bx.center());

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
    }

    glEnable(GL_CULL_FACE);

    g_ShaderDeposition.begin();
    g_ShaderDeposition.u_projection.set(proj);
    g_ShaderDeposition.u_view.set(view);
    g_ShaderDeposition.u_ZNear.set(ZNear);
    g_ShaderDeposition.u_ZFar.set(ZFar);
    float step_ms = (float)g_TimeStep;
    while (step_ms > 0.0f) {
      // step motion
      bool done;
      float delta_ms = motion_step(step_ms, done);
      if (gcode_error()) {
#ifdef EMSCRIPTEN
        std::string command = "errorLine(" + to_string(gcode_line()) + ");";
        emscripten_run_script(command.c_str());
#endif
      }
      g_GlobalTime += delta_ms;
      step_ms -= delta_ms;
      // line highlighting
#ifdef EMSCRIPTEN
      static int last_line = -1;
      if (gcode_line() != last_line) {
        std::string command = "highlightLine(" + to_string(gcode_line()) + ");";
        emscripten_run_script(command.c_str());
        last_line = gcode_line();
      }
#endif
      // pushed material volume during time interval
      float vf = motion_get_current_flow() * delta_ms; // recover pushed mm^3
      v3f pos  = v3f(motion_get_current_pos());
      float h  = heightAt(pos, g_NozzleDiameter / 2.0f);
      float t  = pos[2] - h;
      // t = max(t, 0.01f);
      g_Trajectory.push_back(pos);
      float len = length(pos - g_PrevPos);
      if (vf > 0.0f) {
        // print move
        float sa  = vf / len;
        float r   = sqrt(sa / (float)M_PI); // sa = pi*r^2
        float squash_t = min(t / 2.0f, r);
        float rs  = disk_squashed_radius(r, squash_t);

        g_ShaderDeposition.u_height.set(pos[2]);
        g_ShaderDeposition.u_thickness.set(t);
        g_ShaderDeposition.u_radius.set(r);
        g_ShaderDeposition.u_extruder.set( 0.25f + 0.75f * gcode_extruder());
        // add cylinder from previous
        g_ShaderDeposition.u_model.set(
          translationMatrix(v3f(0, 0, -squash_t))
         *alignAlongSegment(v3f(g_PrevPos), v3f(pos))
         *scaleMatrix(v3f(rs, rs, 1))
        );
        g_GPUMesh_cylinder->render();
        // add spheres
        g_ShaderDeposition.u_model.set(
           translationMatrix(g_PrevPos)
          *translationMatrix(v3f(0,0,-squash_t))
          *scaleMatrix(v3f(rs))
        );
        g_GPUMesh_sphere->render();
        g_ShaderDeposition.u_model.set(
          translationMatrix(pos)
          *translationMatrix(v3f(0, 0, -squash_t))
          *scaleMatrix(v3f(rs))
        );
        g_GPUMesh_sphere->render();
        // update height field
        t_height_segment seg;
        seg.a = pos;
        seg.b = g_PrevPos;
        seg.time = max(pos[2], g_PrevPos[2]); // g_GlobalTime;
        seg.radius = rs;
        g_HeightSegments.push_back(seg);
      }
      g_PrevPos = pos;
#if 1
      // height segments (with delay)
      auto S = g_HeightSegments.begin();
      while (S != g_HeightSegments.end()) {
        if (S->time < pos[2]) {
          rasterizeInHeightField(S->a, S->b, S->radius);
          S = g_HeightSegments.erase(S);
        } else {
          S++;
        }
      }
#endif
      if (done) break;
    } // iter
    g_ShaderDeposition.end();

    g_RT->unbind();

    // trajectory
    while (g_Trajectory.size() > 256) {
      g_Trajectory.erase(g_Trajectory.begin());
    }
    // erase one each time
    if (!g_Trajectory.empty()) {
      g_Trajectory.erase(g_Trajectory.begin());
    }

    if (!redraw) {
      tm_lastChange = milliseconds();
    }

    // clear screen
    glViewport(g_UIWidth, 0, g_ScreenWidth, g_ScreenHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.1f, 0.1f, 0.1f);

    // final pass
    g_ShaderFinal.begin();
    g_ShaderFinal.u_projview.set(orthoMatrixGL(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
    g_ShaderFinal.u_texpts.set(0);
    g_ShaderFinal.u_pixsz.set(v3f(1.0f / g_RTWidth, 1.0f / g_RTWidth, 0.0f));
    g_ShaderFinal.u_texscl.set(v2f(g_RenderWidth / (float)g_RTWidth, g_RenderHeight / (float)g_RTHeight));
    g_ShaderFinal.u_ZNear.set(0.01f);
    g_ShaderFinal.u_ZFar.set(1000.0f);
    g_ShaderFinal.u_color_overhangs.set(g_ColorOverhangs ? 1 : 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_RT->texture());
    g_GPUMesh_quad->render();
    g_ShaderFinal.end();

    // render trajectory
    if (!g_Trajectory.empty()) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_DEPTH_TEST);
      g_ShaderSimple.begin();
      g_ShaderSimple.u_projection.set(proj);
      g_ShaderSimple.u_view.set(view);
      g_ShaderSimple.u_color.set(v4f(0, 1, 0, 1));
      v3f prev = g_Trajectory.front();
      ForRange(t, 1, (int)g_Trajectory.size() - 1) {
        g_ShaderSimple.u_alpha.set(t / (float)((int)g_Trajectory.size() - 1));
        v3f pos = g_Trajectory[t];
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

#if 0
    g_ShaderSimple.begin();
    g_ShaderSimple.u_projection.set(proj);
    g_ShaderSimple.u_view.set(view);
    g_ShaderSimple.u_color.set(v4f(0, 0, 1, 1));
    g_ShaderSimple.u_alpha.set(1);
    ForRange(j, 550, 750) {
      ForRange(i, 900, 1100) {
        if (g_HeightField.at(i, j) > 22.8f) {
          g_ShaderSimple.u_view.set(
            view
            *translationMatrix(v3f((i + 0.5f)*c_HeightFieldStep, (j + 0.5f)*c_HeightFieldStep, g_HeightField.at(i, j)))
            *scaleMatrix(v3f(0.2f))
          );
          g_GPUMesh_sphere->render();
        }
      }
    }
    g_ShaderSimple.end();
#endif

    // render bed
    bed_render(proj,view,g_BedSize[0], g_BedSize[1]);

    // render axis
    {
      m4x4f proj = orthoMatrixGL<float>(
        bx.center()[0] - ex * 0.8f, bx.center()[0] + ex * 0.8f,
        bx.center()[1] - ex * 0.8f, bx.center()[1] + ex * 0.8f,
        bx.center()[2] - ex * 4.0f, bx.maxCorner()[2] + ex * 4.0f);
      glViewport(g_UIWidth, 0, g_ScreenWidth / 4, g_ScreenHeight / 4);
      g_ShaderSimple.begin();
      g_ShaderSimple.u_projection.set(proj);
      g_ShaderSimple.u_view.set(translationMatrix(-view.mulPoint(v3f(0))) * view * scaleMatrix(v3f(4.0f)));
      g_ShaderSimple.u_color.set(v4f(0));
      g_ShaderSimple.u_alpha.set(1.0f);
      glDisable(GL_CULL_FACE);
      g_GPUMesh_axis->render();
      g_ShaderSimple.end();
      glViewport(g_UIWidth, 0, g_ScreenWidth, g_ScreenHeight);
    }

  } else if (g_FatalError) { // fatal error

    glViewport(g_UIWidth, 0, g_ScreenWidth, g_ScreenHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.7f, 0.5f, 0.5f);

  } else {

    glViewport(g_UIWidth, 0, g_ScreenWidth, g_ScreenHeight);
    LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.1f, 0.1f, 0.1f);

  }

  if (!g_FatalError || g_FatalErrorAllowRestart) {
#ifdef EMSCRIPTEN
/*
          std::string command = "reportError(" + to_string(g_Script->lastErrorLine()) + ",'" + jsEncodeString(g_Script->lastErrorMessage().c_str()) + "');";
          emscripten_run_script(command.c_str());
*/
#endif
  }

  // interface
  glViewport(0, 0, g_UIWidth + g_ScreenWidth, g_ScreenHeight);
  ImGuiPanel();
}

// ----------------------------------------------------------------

void mainKeyboard(unsigned char key)
{
  if (key == 'e')      g_ZoomTarget = g_ZoomTarget*1.01f;
  else if (key == 'r') g_ZoomTarget = g_ZoomTarget / 1.01f;
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

  {
    if (btn == LIBSL_WHEEL_UP) {
      g_ZoomTarget = g_ZoomTarget*1.05f;
    } else if (btn == LIBSL_WHEEL_DOWN) {
      g_ZoomTarget = g_ZoomTarget / 1.05f;
    }
  }

}

// ----------------------------------------------------------------

#ifdef EMSCRIPTEN

#include <emscripten/bind.h>

void beginDownload()
{
  g_Downloading = true;
  std::remove("/print.gcode");
}

void setDownloadProgress(float progress)
{
  g_DownloadProgress = progress;
  cout << "progress = " << progress << endl;
}

void endDownload()
{
  g_Downloading = false;
}

EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::function("beginDownload", &beginDownload);
  emscripten::function("setDownloadProgress", &setDownloadProgress);
  emscripten::function("endDownload", &endDownload);
}

#endif 

// ----------------------------------------------------------------

int main(int argc, const char **argv)
{
  /// init simple UI
  TrackballUI::onRender = mainRender;
  TrackballUI::onKeyPressed = mainKeyboard;
  TrackballUI::onMouseButtonPressed = mainMouseButton;

  TrackballUI::init(g_UIWidth+g_ScreenWidth, g_ScreenHeight);

  // GL init
  glEnable(GL_DEPTH_TEST);

  // imgui
  SimpleUI::bindImGui();
  SimpleUI::onReshape(g_ScreenWidth + g_UIWidth, g_ScreenHeight);

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

  // shader for simple drawing
#ifdef EMSCRIPTEN
  g_ShaderSimple.emscripten = "precision mediump float;\n";
#endif
  g_ShaderSimple.init();

  // shader for drawing deposited material
#ifdef EMSCRIPTEN
  g_ShaderDeposition.emscripten = "precision mediump float;\n";
#endif
  g_ShaderDeposition.init();

#ifdef EMSCRIPTEN
  g_ShaderFinal.emscripten = "precision mediump float;\n";
#endif
  g_ShaderFinal.init();
  g_RT = RenderTarget2DRGBA_Ptr(new RenderTarget2DRGBA(g_RTWidth, g_RTHeight, GPUTEX_AUTOGEN_MIPMAP));
  g_RT->bind();
  glViewport(0, 0, g_RenderWidth, g_RenderHeight);
  LibSL::GPUHelpers::clearScreen(LIBSL_COLOR_BUFFER | LIBSL_DEPTH_BUFFER, 0.0f, 0.0f, 0.0f);
  g_RT->unbind();

  g_GPUMesh_sphere = AutoPtr<MeshRenderer<mvf_mesh> >( new MeshRenderer<mvf_mesh>(shape_sphere( 1.0f,32 )) );
  g_GPUMesh_cylinder = AutoPtr<MeshRenderer<mvf_mesh> >(new MeshRenderer<mvf_mesh>(shape_cylinder(1.0f, 1.0f, 1.0f, 32)));

  /// default view
  TrackballUI::trackball().set(v3f(-g_BedSize[0]/2.0, -g_BedSize[1] / 2.0,-300.0f), v3f(0), quatf(v3f(1, 0, 0), -1.0f)*quatf(v3f(0, 0, 1), 0.0f));
  TrackballUI::trackball().setCenter(v3f(g_BedSize[0] / 2.0, g_BedSize[1] / 2.0, 10.0f));
  TrackballUI::trackball().setBallSpeed(0.0f);
  TrackballUI::trackball().setAllowRoll(false);
  TrackballUI::trackball().setUp(Trackball::Z_neg);

  // height field
  int hszx = ceil(g_BedSize[0] / c_HeightFieldStep);
  int hszy = ceil(g_BedSize[1] / c_HeightFieldStep);
  cerr << "Allocated height field " << printByteSize(hszx*hszy*sizeof(float)) << endl;
  g_HeightField.allocate(hszx,hszy);
  g_HeightField.fill(0.0f);

  SimpleUI::initImGui();

  /// load gcode
#ifdef EMSCRIPTEN
  emscripten_run_script("parseCommandLine();\n");
  if (!g_Downloading) {
    g_GCode_string = loadFileIntoString("./icesl.gcode");
    gcode_start(g_GCode_string.c_str());
    std::string command = "setupEditor();";
    emscripten_run_script(command.c_str());
  }
#else
  std::string g_GCode_string = loadFileIntoString("E:\\SLEFEBVR\\PROJECTS\\ALL\\curvislice\\models\\kitten__mid2.000000kitten__mid2.000000.gcode");
  gcode_start(g_GCode_string.c_str());
#endif
  motion_start( g_FilamentDiameter );

  printer_reset();

  /// main loop
  TrackballUI::loop();

  SimpleUI::terminateImGui();
  TrackballUI::shutdown();

  return 0;
}

// ----------------------------------------------------------------
