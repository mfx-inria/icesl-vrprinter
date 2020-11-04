#pragma once

#include <LibSL.h>
#include <LibSL_gl.h>

LIBSL_WIN32_FIX;

#include <imgui.h>

#ifdef EMSCRIPTEN
  #include <emscripten.h>
  #include <emscripten/html5.h>
  #include <emscripten/bind.h>
#endif

#ifndef EMSCRIPTEN
  #include <tclap/CmdLine.h>
  #include "FileDialog.h"
  #ifdef _MSC_VER
    #define glActiveTexture glActiveTextureARB
  #endif
#endif

#include "sphere_squash.h"

// ----------------------------------------------------------------
using namespace std;
// ----------------------------------------------------------------

int           g_UIWidth = 350;

int           g_ScreenWidth = 700;
int           g_ScreenHeight = 700;
int           g_RenderWidth = 700;
int           g_RenderHeight = 700;

int           g_RTWidth = 1024;
int           g_RTHeight = 1024;

v2f           g_BedSize(200.0f, 200.0f);
float         g_FilamentDiameter = 1.75f;
float         g_NozzleDiameter = 0.4f;
bool          g_isCentered = false;
int           g_NumExtruders = 1;
vector<pair<float, float>> g_Extruders_offset;

const float   c_HeightFieldStep = 0.04f; // mm

const float   c_ThicknessEpsilon = 0.001f; // 1 um

float         g_StatsHeightThres = 1.2f; // mm, ignored everything below regarding overlaps and dangling

float         g_MmStep = g_NozzleDiameter * 0.5f;
float         g_UserMmStep = 1000.0f;
int           g_StartAtLine = 0;
int           g_LastLine = 0;

double        g_GlobalDepositionLength = 0.0;

bool          g_ShowTrajectory = false;
bool          g_ColorOverhangs = false;

bool          g_AutoPause = false;
float         g_AutoPauseDanglingLen = 5.0f;
float         g_AutoPauseOverlapLen = 5.0f;

bool          g_InDangling = false;
double        g_InDanglingStart = 0.0;
bool          g_InDanglingBridging = false;
std::map<int, float> g_DanglingHisto;

class TrajPoint {
public:
  v3d   pos;
  float th;
  float r;
  float ov;
  float dg;
  TrajPoint(v3d _pos, float _th, float _r, float _ov, float _dg) :
    pos(_pos), th(_th), r(_r), ov(_ov), dg(_dg) {}
};

std::vector<TrajPoint> g_DanglingTrajectory;


bool          g_InOverlap = false;
double        g_InOverlapStart = 0.0;
std::map<int, float> g_OverlapHisto;

bool          g_DumpHeightField = false;
double        g_DumpHeightFieldStartLen = 0.0;

bool          g_Paused = false;

std::string   g_GCode_path;
//std::string   g_GCode_fname;
std::string   g_GCode_string;

#include "simple.h"
AutoBindShader::simple     g_ShaderSimple;
#include "final.h"
AutoBindShader::final      g_ShaderFinal;
#include "deposition.h"
AutoBindShader::deposition g_ShaderDeposition;

AutoPtr<AutoBindShader::deposition> test;

RenderTarget2DRGBA_Ptr g_RT;

typedef GPUMESH_MVF1(mvf_vertex_3f)                mvf_mesh;
typedef GPUMesh_VertexBuffer<mvf_mesh>             SimpleMesh;

AutoPtr<SimpleMesh>                      g_GPUMesh_quad;
AutoPtr<SimpleMesh>                      g_GPUMesh_axis;

AutoPtr<MeshRenderer<mvf_mesh> >         g_GPUMesh_sphere;
AutoPtr<MeshRenderer<mvf_mesh> >         g_GPUMesh_cylinder;

m4x4f  g_LastView = m4x4f::identity();
bool   g_Rotating = false;
bool   g_Dragging = false;
float  g_Zoom = 1.0f;
float  g_ZoomTarget = 1.0f;

std::vector<v3d>                 g_Trajectory;

typedef struct
{
  double deplength;
  double radius;
  v3d   a;
  v3d   b;
} t_height_segment;

std::list<t_height_segment> g_HeightSegments;

AAB<3>                   g_HeightFieldBox;

Array2D<Tuple<float, 1> > g_HeightField;

v3d    g_PrevPos(0.0);
v3d    g_PrevPrevPos(0.0);
bool   g_PrevWasTravelOrDangling = false;


bool   g_ForceRedraw = true;
bool   g_ForceClear = false;
bool   g_FatalError = false;
bool   g_FatalErrorAllowRestart = false;
string g_FatalErrorMessage = "unkonwn error";

bool   g_Downloading = false;
float  g_DownloadProgress = 0.0f;

std::vector<float> g_Flows(64, 0.0f);
int                g_FlowsCount = 0;
float              g_FlowsSample = 0.0f;
std::vector<float> g_Speeds(64, 0.0f);
int                g_SpeedsCount = 0;
float              g_SpeedsSample = 0.0f;

const float ZNear = 0.1f;
const float ZFar = 500.0f;

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

// ----------------------------------------------------------------
// rendering

void mainRender();
void makeAxisMesh();
m4x4f alignAlongSegment(const v3f& p0, const v3f& p1);
void rasterizeDiskInHeightField(const v2i& p, float z, float r);
void rasterizeInHeightField(v3f a, const v3f& b, float r);

float heightAt(v3f a, float r);
float danglingAt(float max_th, const v3f& a, float r);
float overlapAt(float th, const v3f& a, float r);

// ----------------------------------------------------------------
// Simulation

bool step_simulation(bool gpu_draw);

// ----------------------------------------------------------------
// UI

void ImGuiPanel();
void mainKeyboard(unsigned char key);
void mainMouseButton(uint x, uint y, uint btn, uint flags);

// ----------------------------------------------------------------
// utilities

void session_start();
void printer_reset();
void load_gcode(std::string file = std::string()); // load a gcode file and return it as a string
void gen_histogram(std::map<int, float> &map, Histogram &histo, float filter = 1.0f);
void export_histogram(std::string fname, Histogram &h);
string getFileName(const string& s);

#ifdef EMSCRIPTEN
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

string jsEncodeString(const char* strin)
{
  string str;
  int i = 0;
  while (strin[i] != '\0') {
    if (strin[i] == '\\') {
      str = str + "\\\\";
    }
    else if (strin[i] == '\n') {
      str = str + "\\n";
    }
    else if (strin[i] == '\r') {

    }
    else if (strin[i] == '&') {
      str = str + "\\&";
    }
    else if (strin[i] == '\'') {
      str = str + "\\'";
    }
    else {
      str = str + strin[i];
    }
    i++;
  }
  return str;
}
#endif 

// ----------------------------------------------------------------
class GPUBead
{
private:

  bool m_Empty = true;

  // last drawn point
  float m_LastOverlap = 0.0f;
  float m_LastDangling = 0.0f;
  float m_LastTh = 0.0f;
  float m_LastRadius = 0.0f;
  v3f   m_LastPos;
  bool  m_IsBridge = false;
  int   m_Extruder = 0;

  void drawSegment(v3f a, v3f b, float th, float r, float dg, float ov, int e)
  {
    double squash_t = min(th / 2.0, r); // min dimension on the edge sphere
    double rs = disk_squashed_radius(r, squash_t); // squashed to preserve area or rectable (th * r)
    g_ShaderDeposition.u_height.set((float)b[2]);
    g_ShaderDeposition.u_thickness.set((float)th);
    g_ShaderDeposition.u_radius.set((float)r);
    g_ShaderDeposition.u_dangling.set(dg);
    g_ShaderDeposition.u_overlap.set(ov);
    g_ShaderDeposition.u_extruder.set(float(e + 1));
    g_ShaderDeposition.u_bridge.set(m_IsBridge ? 1.0f : 0.0f);
    // add cylinder from previous
    g_ShaderDeposition.u_model.set(
      translationMatrix(v3f(0, 0, -(float)squash_t))
      * alignAlongSegment(v3f(a), v3f(b))
      * scaleMatrix(v3f((float)rs, (float)rs, 1))
    );
    g_GPUMesh_cylinder->render();
    // add spheres
    g_ShaderDeposition.u_model.set(
      translationMatrix(v3f(a))
      * translationMatrix(v3f(0, 0, -(float)squash_t))
      * scaleMatrix(v3f((float)rs))
    );
    g_GPUMesh_sphere->render();
    g_ShaderDeposition.u_model.set(
      translationMatrix(v3f(b))
      * translationMatrix(v3f(0, 0, -(float)squash_t))
      * scaleMatrix(v3f((float)rs))
    );
    g_GPUMesh_sphere->render();
  }

public:

  GPUBead() {}

  void drawPoint(v3f p, float th, float r, float dg, float ov, int e)
  {
    // draw segment
    drawSegment(m_LastPos, p, th, r, dg, ov, e);
    // record as last drawn
    m_LastOverlap = ov;
    m_LastDangling = dg;
    m_LastTh = th;
    m_LastRadius = r;
    m_LastPos = p;
    m_Extruder = e;
  }

  void addPoint(v3f p, float th, float r, float dg, float ov, int e)
  {
    if (m_Empty) {
      m_Empty = false;
      // record as last drawn
      m_LastOverlap = ov;
      m_LastDangling = dg;
      m_LastTh = th;
      m_LastRadius = r;
      m_LastPos = p;
      m_Extruder = e;
    }
    else {
      drawPoint(p, th, r, dg, ov, e);
    }
  }

  void closeAny()
  {
    m_Empty = true;
  }

  void setIsBridge(bool b) {
    m_IsBridge = b;
  }

};

// ----------------------------------------------------------------