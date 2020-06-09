
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


void draw_fullscreen_quad(const char *texture);
void draw_fullscreen_quad(GLuint texture_handle);

void draw_screen_quad(v2 position, v2 scale, GLuint texture_handle);

GLuint make_texture(const char *path);
GLuint get_or_make_texture(const char *path);

GLuint make_shader(const char *path);

RenderTexture make_render_texture(int width, int height);
void set_render_target(RenderTexture render_texture);
void reset_render_target();

void create_gl_context();

void init_common_graphics();

void check_gl_errors(const char *desc);

