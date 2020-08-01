
#pragma once

//#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/wglew.h"

#include "my_math.h"



struct RenderTexture
{
    GLuint frame_buffer;
    GLuint texture;
    int width;
    int height;
};

void draw_quad(const char *texture, v2 position = v2(), v2 scale = v2(1.0f, 1.0f));
void draw_quad(GLuint texture_handle, v2 position = v2(), v2 scale = v2(1.0f, 1.0f));
void draw_quad(v4 color, v2 position = v2(), v2 scale = v2(1.0f, 1.0f));

void draw_bottom_left_quad(GLuint texture_handle, v2 position, v2 scale);
void draw_bottom_left_quad(v4 color, v2 position, v2 scale);

void draw_arrow(v2 start, v2 end, v4 color);

void draw_ndc_quad(v4 color, v2 position, v2 scale);

void set_camera_position(v2 position);
void set_camera_width(float width);

mat4 get_ndc_m_world();
v2 ndc_point_to_world(v2 ndc);

GLuint make_texture(const char *path);
GLuint get_or_make_texture(const char *path);

RenderTexture make_render_texture(int width, int height);
void set_render_target(RenderTexture render_texture);
void reset_render_target();

void create_gl_context();

void init_common_graphics();

void check_gl_errors(const char *desc);

