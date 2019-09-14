#pragma once

#include "my_math.h"

#include <windows.h>

#include <vector> // vector for meshes

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcommon.h>
#include "DX\d3dx10math.h"
#include "DX\d3dx11async.h"
#include "DX\d3dx11tex.h"

enum PrimitiveType
{
  PRIMITIVE_QUAD,
};

struct Color
{
  float r, g, b, a;

  Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
  Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
};

struct Mesh;
struct Shader;
struct Texture;
struct Model
{
  const char *debug_name = "";
  bool show = true;

  v3 position = v3();
  v2 scale = v2(1.0f, 1.0f);
  float rotation = 0.0f; // In degrees

  v4 blend_color = v4(1.0f, 1.0f, 1.0f, 1.0f);


  Mesh *mesh = 0;
  Shader *shader = 0;
  Texture *texture = 0;
};

typedef unsigned ModelHandle;




void init_renderer(HWND window, unsigned framebuffer_width, unsigned framebuffer_height, bool is_fullscreen, bool is_vsync);
void render();
void swap_frame();
void shutdown_renderer();


ModelHandle create_model(PrimitiveType primitive, const char *texture_path);
ModelHandle create_model(v3 *vertices, unsigned num_vertices, unsigned *indices, unsigned num_indices);

Model *get_temp_model_pointer(ModelHandle model);

void immediate_triangle(v3 p, v3 q, v3 r, Color color);
void immediate_line(v2 a, v2 b, float thickness, Color color);

v2 window_to_world_space(v2 window_position);

ID3D11Device *get_d3d_device();
ID3D11DeviceContext *get_d3d_device_context();

