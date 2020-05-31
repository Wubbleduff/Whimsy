
#pragma once

//#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/wglew.h"

GLuint make_texture(const char *path);

GLuint make_shader(const char *vert_source, const char *frag_source, const char *geom_source = nullptr);

void check_gl_errors(const char *desc);

void create_gl_context();

