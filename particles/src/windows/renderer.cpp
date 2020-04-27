
#include "renderer.h"

#include "my_math.h"

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/wglew.h"

#include <assert.h>
#include <stdio.h>




struct RendererData
{
    HWND window_handle;
    HGLRC gl_context;
    HDC device_context;
    int window_width;
    int window_height;
    float aspect_ratio;

    GLuint quad_vao;
    GLuint quad_shader_program;
};
static RendererData *renderer_data;

// {major, minor}
static const int TARGET_GL_VERSION[2] = {3, 3};











static void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        printf("Error: %s\n", desc);
        assert(false);
    }
}

static void create_gl_context()
{
    renderer_data->device_context = GetDC(renderer_data->window_handle);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(renderer_data->device_context, &pfd);
    if(pixel_format == 0) return;

    BOOL result = SetPixelFormat(renderer_data->device_context, pixel_format, &pfd);
    if(!result) return;

    HGLRC temp_context = wglCreateContext(renderer_data->device_context);
    wglMakeCurrent(renderer_data->device_context, temp_context);

    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        OutputDebugString("GLEW is not initialized!");
    }

    int attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, TARGET_GL_VERSION[0],
        WGL_CONTEXT_MINOR_VERSION_ARB, TARGET_GL_VERSION[1],
        WGL_CONTEXT_FLAGS_ARB, 0,
        0
    };

    if(wglewIsSupported("WGL_ARB_create_context") == 1)
    {
        renderer_data->gl_context = wglCreateContextAttribsARB(renderer_data->device_context, 0, attribs);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temp_context);
        wglMakeCurrent(renderer_data->device_context, renderer_data->gl_context);
    }
    else
    {    //It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
        renderer_data->gl_context = temp_context;
    }

    //Checking GL version
    const GLubyte *GLVersionString = glGetString(GL_VERSION);

    //Or better yet, use the GL3 way to get the version number
    int OpenGLVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

    if(!renderer_data->gl_context) return; // Bad
}



void init_renderer(HWND window_handle, int width, int height)
{
    renderer_data = (RendererData *)calloc(1, sizeof(RendererData));

    renderer_data->window_handle = window_handle;
    renderer_data->window_width = width;
    renderer_data->window_height = height;
    renderer_data->aspect_ratio = (float)width / (float)height;

    create_gl_context();

    glViewport(0, 0, width, height);


    v2 vertices[] =
    {
        {-0.5f, -0.5f},
        { 0.5f, -0.5f},
        { 0.5f,  0.5f},

        { 0.5f,  0.5f},
        {-0.5f,  0.5f},
        {-0.5f, -0.5f}
    };


    glGenVertexArrays(1, &renderer_data->quad_vao);
    glBindVertexArray(renderer_data->quad_vao);
    check_gl_errors("making vao");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    check_gl_errors("making vbo");

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    check_gl_errors("vertex attrib pointer");




    const char *vert_shader_source =
        "#version 330 core\n"
        "layout (location = 0) in vec2 a_pos;\n"
        "uniform mat4 mvp;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = mvp * vec4(a_pos.x, a_pos.y, 0.0f, 1.0);\n"
        "}\0";

    const char *frag_shader_source =
        "#version 330 core\n"
        "out vec4 frag_color;\n"
        "void main()\n"
        "{\n"
        "   frag_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\0";

    int  success;
    char info_log[512];

    unsigned int vert_shader;
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
    glCompileShader(vert_shader);
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
        printf("SHADER ERROR: %s\n", info_log);
    }

    unsigned int frag_shader;
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        printf("SHADER ERROR: %s\n", info_log);
    }
    check_gl_errors("compiling shaders");

    renderer_data->quad_shader_program = glCreateProgram();
    check_gl_errors("making program");

    glAttachShader(renderer_data->quad_shader_program, vert_shader);
    glAttachShader(renderer_data->quad_shader_program, frag_shader);
    glLinkProgram(renderer_data->quad_shader_program);
    check_gl_errors("linking program");

    glGetProgramiv(renderer_data->quad_shader_program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(renderer_data->quad_shader_program, 512, NULL, info_log);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader); 
    check_gl_errors("deleting shaders");
}


void render_quad(v2 pos, v2 scale)
{
    glUseProgram(renderer_data->quad_shader_program);
    check_gl_errors("use program");

    mat4 world_m_model =
    {
        scale.x, 0.0f, 0.0f, pos.x,
        0.0f, scale.y, 0.0f, pos.y,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float camera_width = 10.0f;
    mat4 clip_m_world =
    {
        2.0f / camera_width, 0.0f, 0.0f, 0.0f,
        0.0f, (2.0f / camera_width) * renderer_data->aspect_ratio, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    mat4 mvp = clip_m_world * world_m_model;

    GLint loc = glGetUniformLocation(renderer_data->quad_shader_program, "mvp");
    glUniformMatrix4fv(loc, 1, true, &(mvp[0][0]));
    check_gl_errors("set uniform");

    glBindVertexArray(renderer_data->quad_vao);
    check_gl_errors("use vao");

    glDrawArrays(GL_TRIANGLES, 0, 6);
    check_gl_errors("draw");

}

void render()
{
    glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static float offset = 0.0f;
    offset += 0.05f;

    for(int i = 0; i < 20; i++)
    {
        float percent = i / 20.0f;
        float theta = 2.0f * PI * percent + offset;

        v2 pos;
        pos.x = cos(theta);
        pos.y = sin(theta);

        render_quad(pos, v2(0.2f, 0.2f));
    }



    SwapBuffers(renderer_data->device_context);
}


