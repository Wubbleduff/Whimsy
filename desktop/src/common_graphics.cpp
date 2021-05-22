
#include "common_graphics.h"
#include "shader.h"
#include "platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <map>

#include "imgui.h"


struct Vertex
{
    v2 pos;
    v2 uv;

    Vertex() {}
    Vertex(v2 p, v2 u) : pos(p), uv(u) { }
};

struct Mesh
{
    GLuint vao;
    GLuint vbo;
};


struct Camera
{
    v2 position;
    float width;
};

struct GraphicsData
{
    std::map<std::string, GLuint> *textures_map;

    HGLRC gl_context;

    Camera camera;

    Mesh quad_mesh;
    Mesh bottom_left_quad_mesh;
    Mesh triangle_mesh;

    Shader quad_shader;
    Shader flat_color_shader;
};
static GraphicsData *graphics_data;
// {major, minor}
static const int TARGET_GL_VERSION[2] = { 4, 4 };



static void draw_quad_with_mesh(GLuint texture_handle, v2 position, v2 scale, Mesh *mesh)
{
    // use quad mesh
    glBindVertexArray(mesh->vao);
    check_gl_errors("use vao");

    // use shader
    use_shader(graphics_data->quad_shader);

    mat4 world_m_model =
    {
        scale.x, 0.0f, 0.0f, position.x,
        0.0f, scale.y, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    mat4 ndc_m_world = get_ndc_m_world();
    mat4 mvp = ndc_m_world * world_m_model;

    set_uniform(graphics_data->quad_shader, "mvp", mvp);
    set_uniform(graphics_data->quad_shader, "blend_color", v4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_quad_with_mesh(v4 color, v2 position, v2 scale, Mesh *mesh)
{
    // use quad mesh
    glBindVertexArray(mesh->vao);
    check_gl_errors("use vao");

    // use shader
    use_shader(graphics_data->flat_color_shader);

    mat4 world_m_model =
    {
        scale.x, 0.0f, 0.0f, position.x,
        0.0f, scale.y, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    mat4 ndc_m_world = get_ndc_m_world();
    mat4 mvp = ndc_m_world * world_m_model;

    set_uniform(graphics_data->flat_color_shader, "mvp", mvp);

    set_uniform(graphics_data->flat_color_shader, "color", color);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_quad(const char *texture, v2 position, v2 scale)
{
    GLuint texture_handle = get_or_make_texture(texture);
    draw_quad(texture_handle, position, scale);
}

void draw_quad(GLuint texture_handle, v2 position, v2 scale)
{
    draw_quad_with_mesh(texture_handle, position, scale, &graphics_data->quad_mesh);
}

void draw_quad(v4 color, v2 position, v2 scale)
{
    draw_quad_with_mesh(color, position, scale, &graphics_data->quad_mesh);
}

void draw_bottom_left_quad(GLuint texture_handle, v2 position, v2 scale)
{
    draw_quad_with_mesh(texture_handle, position, scale, &graphics_data->bottom_left_quad_mesh);
}

void draw_bottom_left_quad(v4 color, v2 position, v2 scale)
{
    draw_quad_with_mesh(color, position, scale, &graphics_data->bottom_left_quad_mesh);
}

void draw_arrow(v2 start, v2 end, v4 color)
{
    // use quad mesh
    glBindVertexArray(graphics_data->triangle_mesh.vao);
    check_gl_errors("use vao");

    // use shader
    use_shader(graphics_data->flat_color_shader);

    float angle = angle_between(v2(0.0f, 1.0f), end - start);
    if(dot(v2(1.0f, 0.0f), end - start) > 0.0f)
    {
        angle = 2.0f*PI - angle;
    }
    float l = length(end - start);
    v2 scale = v2(l * 0.1f, l);
    mat4 world_m_model = make_translation_matrix(v3(start, 0.0f)) * make_z_axis_rotation_matrix(angle) * make_scale_matrix(v3(scale, 1.0f));
    mat4 ndc_m_world = get_ndc_m_world();
    mat4 mvp = ndc_m_world * world_m_model;

    set_uniform(graphics_data->flat_color_shader, "mvp", mvp);
    set_uniform(graphics_data->flat_color_shader, "color", color);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void draw_ndc_quad(v4 color, v2 position, v2 scale)
{
    // use quad mesh
    glBindVertexArray(graphics_data->quad_mesh.vao);
    check_gl_errors("use vao");

    use_shader(graphics_data->flat_color_shader);

    mat4 mvp = 
    {
        scale.x, 0.0f, 0.0f, position.x,
        0.0f, scale.y, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f
    };
    set_uniform(graphics_data->flat_color_shader, "mvp", mvp);
    set_uniform(graphics_data->flat_color_shader, "color", color);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void set_camera_position(v2 position)
{
    graphics_data->camera.position = position;
}

void set_camera_width(float width)
{
    graphics_data->camera.width = width;
}

mat4 get_ndc_m_world()
{
    float screen_aspect_ratio = get_aspect_ratio(); // width / height
    v2 camera_position = graphics_data->camera.position;
    float camera_width = graphics_data->camera.width;
    
    mat4 view_m_world =
    {
        1.0f, 0.0f, 0.0f, -camera_position.x,
        0.0f, 1.0f, 0.0f, -camera_position.y,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float single_monitor_scale = (2560.0f / 5680.0f);
    mat4 ndc_m_view =
    {
        (2.0f * single_monitor_scale) / camera_width, 0.0f, 0.0f, 0.12f,
        0.0f, ((2.0f * single_monitor_scale) / camera_width) * screen_aspect_ratio, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return ndc_m_view * view_m_world;
}

v2 ndc_point_to_world(v2 ndc)
{
    mat4 ndc_m_world = get_ndc_m_world();

    v4 ndc4 = v4(ndc, 0.0f, 1.0f);
    v4 world4 = inverse(ndc_m_world) * ndc4;

    return v2(world4.x, world4.y);
}




GLuint make_texture(const char *path)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *texture_data = stbi_load(path, &width, &height, &channels, 4);
    if(texture_data == NULL) return 0;

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    check_gl_errors("make texture");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
    check_gl_errors("send texture data");

    glGenerateMipmap(GL_TEXTURE_2D);
    check_gl_errors("generate mipmap");

    stbi_image_free(texture_data);

    return texture;
}

GLuint get_or_make_texture(const char *texture)
{
    if(graphics_data->textures_map->find(texture) != graphics_data->textures_map->end())
    {
        return (*graphics_data->textures_map)[texture]; 
    }
    else
    {
        GLuint new_texture = make_texture(texture);
        (*graphics_data->textures_map)[texture] = new_texture;
        return new_texture;
    }
}

RenderTexture make_render_texture(int width, int height)
{
    RenderTexture result;

    result.width = width;
    result.height = height;

    glGenFramebuffers(1, &result.frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, result.frame_buffer);

    glGenTextures(1, &result.texture);
    glBindTexture(GL_TEXTURE_2D, result.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLuint depth_render_buffer;
    glGenRenderbuffers(1, &depth_render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, result.texture, 0);
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        log_error("Could not create frame buffer");
    }

    // TODO: Set to previous render target
    reset_render_target();

    return result;
}

void set_render_target(RenderTexture render_texture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, render_texture.frame_buffer);
    glViewport(0, 0, render_texture.width, render_texture.height);
}

void reset_render_target()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, get_screen_width(), get_screen_height());
}


void create_gl_context()
{
    HDC dc = get_device_context();

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = /*PFD_DOUBLEBUFFER | */PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(dc, &pfd);
    if(pixel_format == 0) return;

    BOOL result = SetPixelFormat(dc, pixel_format, &pfd);
    if(!result) return;

    HGLRC temp_context = wglCreateContext(dc);
    wglMakeCurrent(dc, temp_context);

    glewExperimental = true;
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
        graphics_data->gl_context = wglCreateContextAttribsARB(dc, 0, attribs);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temp_context);
        wglMakeCurrent(dc, graphics_data->gl_context);
    }
    else
    {   //It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
        graphics_data->gl_context = temp_context;
    }

    //Checking GL version
    const GLubyte *GLVersionString = glGetString(GL_VERSION);

    //Or better yet, use the GL3 way to get the version number
    int OpenGLVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

    if(!graphics_data->gl_context) return; // Bad
}

static void make_mesh(Mesh *mesh, int num_vertices, Vertex *vertices)
{
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    check_gl_errors("making vao");

    GLuint vbo;
    glGenBuffers(1, &mesh->vbo);
    check_gl_errors("making vbo");

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    check_gl_errors("send vbo data");

    float stride = sizeof(Vertex);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);

    // UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(v2)));
    glEnableVertexAttribArray(1);

    check_gl_errors("vertex attrib pointer");
}

void init_common_graphics()
{
    graphics_data = (GraphicsData *)calloc(1, sizeof(GraphicsData));



    create_gl_context();



    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    graphics_data->camera.position = v2();
    graphics_data->camera.width = 10.0f;



    Vertex vertices[] =
    {
        // pos             uv
        { v2(-0.5f, -0.5f),  v2(0.0f, 0.0f) },
        { v2( 0.5f, -0.5f),  v2(1.0f, 0.0f) },
        { v2( 0.5f,  0.5f),  v2(1.0f, 1.0f) },

        { v2( 0.5f,  0.5f),  v2(1.0f, 1.0f) },
        { v2(-0.5f,  0.5f),  v2(0.0f, 1.0f) },
        { v2(-0.5f, -0.5f),  v2(0.0f, 0.0f) },
    };
    make_mesh(&graphics_data->quad_mesh, 6, vertices);

    Vertex bottom_left_vertices[] =
    {
        // pos            uv
        { v2(0.0f,  0.0f),  v2(0.0f, 0.0f) },
        { v2(1.0f,  0.0f),  v2(1.0f, 0.0f) },
        { v2(1.0f,  1.0f),  v2(1.0f, 1.0f) },

        { v2(1.0f,  1.0f),  v2(1.0f, 1.0f) },
        { v2(0.0f,  1.0f),  v2(0.0f, 1.0f) },
        { v2(0.0f,  0.0f),  v2(0.0f, 0.0f) },
    };
    make_mesh(&graphics_data->bottom_left_quad_mesh, 6, bottom_left_vertices);

    Vertex triangle_vertices[] =
    {
        // pos
        { v2(-0.5f,  0.0f), v2() },
        { v2( 0.5f,  0.0f), v2() },
        { v2( 0.0f,  1.0f), v2() },
    };
    make_mesh(&graphics_data->triangle_mesh, 3, triangle_vertices);

    graphics_data->quad_shader = make_shader("shaders/quad.shader");
    graphics_data->flat_color_shader = make_shader("shaders/flat_color.shader");

    graphics_data->textures_map = new std::map<std::string, GLuint>();
}

void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        log_error("Error %i: %s\n", error, desc);
        assert(false);
    }
}

