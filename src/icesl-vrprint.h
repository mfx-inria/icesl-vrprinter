// PB 2019-03-11 from SL 2018-07-03

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
  #include "FileDialog.h"
  #ifdef _MSC_VER
    #define glActiveTexture glActiveTextureARB
  #endif
#endif

// ----------------------------------------------------------------

using namespace std;

// ----------------------------------------------------------------

int           g_UIWidth = 350; // width of the main ImGui window

int           g_ScreenWidth = 1024;  // dimensions of the main window
int           g_ScreenHeight = 1024;

int           g_RenderWidth = 1024;
int           g_RenderHeight = 1024;

int           g_RTWidth = 1024;
int           g_RTHeight = 1024;

v2f           g_BedSize(200.0f, 200.0f);

float         g_FilamentDiameter = 1.75f;
float         g_NozzleDiameter = 0.4f;

bool          g_EIsVolume = false;

float         g_TimeStep = 500.0f;
float         g_GlobalTime = 0.0f;

int           g_StartAtLine = 0;
bool          g_ColorOverhangs = false;

bool          g_Downloading = false;
float         g_DownloadProgress = 0.0f;

std::string   g_GCode_string;

std::vector<float> g_Flows(64, 0.0f);
int                g_FlowsCount = 0;
float              g_FlowsSample = 0.0f;
std::vector<float> g_Speeds(64, 0.0f);
int                g_SpeedsCount = 0;
float              g_SpeedsSample = 0.0f;

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
float heightAt(const v3f& a, float r);
void rasterizeDiskInHeightField(const v2i& p, float z, float r);
void rasterizeInHeightField(const v3f& a, const v3f&b, float r);
m4x4f alignAlongSegment(const v3f& p0, const v3f& p1);

// ----------------------------------------------------------------
// UI

void ImGuiPanel();
void mainKeyboard(unsigned char key);
void mainMouseButton(uint x, uint y, uint btn, uint flags);

// ----------------------------------------------------------------
// utilities

string load_gcode(); // load a gcode file and return it as a string
string jsEncodeString(const char *strin);
void printer_reset();

#ifdef EMSCRIPTEN
void beginDownload();

void setDownloadProgress(float progress);

void endDownload();

EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::function("beginDownload", &beginDownload);
  emscripten::function("setDownloadProgress", &setDownloadProgress);
  emscripten::function("endDownload", &endDownload);
}
#endif 

// ----------------------------------------------------------------
// main

int main(int argc, const char **argv);

// ----------------------------------------------------------------
