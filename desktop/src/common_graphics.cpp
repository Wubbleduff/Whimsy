
#include "common_graphics.h"
#include "platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <map>



struct GraphicsData
{
    std::map<std::string, GLuint> *textures_map;

    HGLRC gl_context;

    GLuint quad_vao;
    GLuint quad_shader_program;
};
static GraphicsData *graphics_data;
// {major, minor}
static const int TARGET_GL_VERSION[2] = { 4, 4 };




void draw_fullscreen_quad(const char *texture)
{
    GLuint texture_handle = get_or_make_texture(texture);
    draw_fullscreen_quad(texture_handle);
}

void draw_fullscreen_quad(GLuint texture_handle)
{
    draw_screen_quad(v2(), v2(1.0f, 1.0f), texture_handle);
}

void draw_screen_quad(v2 position, v2 scale, GLuint texture_handle)
{
    // use quad mesh
    glBindVertexArray(graphics_data->quad_vao);
    check_gl_errors("use vao");

    // use shader
    glUseProgram(graphics_data->quad_shader_program);
    check_gl_errors("use program");

    mat4 mat =
    {
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        position.x, position.y, 0.0f, 1.0f
    };
    GLint mvp_location = glGetUniformLocation(graphics_data->quad_shader_program, "mvp");
    glUniformMatrix4fv(mvp_location, 1, false, &(mat[0][0]));
    
    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
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

static bool is_newline(char *c)
{
    if(c[0] == '\n' || c[0] == '\r' ||
      (c[0] == '\r' && c[1] == '\n'))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void read_shader_file(const char *path, char **memory, char **vert_source, char **geom_source, char **frag_source)
{
    *memory = nullptr;
    *vert_source = nullptr;
    *geom_source = nullptr;
    *frag_source = nullptr;

    char *file = read_file_as_string(path);
    if(file == nullptr) return;

    *memory = file;

    int current_tag_length = 0;
    char *current_tag = nullptr;
    char *current_source_start = nullptr;

    char *character = file;
    while(*character != '\0')
    {
        if(*character == '@')
        {
            // Finish reading current shader source
            if(current_tag == nullptr)
            {
            }
            else if(strncmp("vertex", current_tag, current_tag_length) == 0)
            {
                *vert_source = current_source_start;
            }
            else if(strncmp("geometry", current_tag, current_tag_length) == 0)
            {
                *geom_source = current_source_start;
            }
            else if(strncmp("fragment", current_tag, current_tag_length) == 0)
            {
                *frag_source = current_source_start;
            }


            // Null terminate previous shader string
            *character = '\0';

            // Read tag
            character++;
            char *tag = character;

            // Move past tag
            while(!is_newline(character))
            {
                character++;
            }
            char *one_past_end_tag = character;
            while(is_newline(character)) character++;

            current_tag_length = one_past_end_tag - tag;
            current_tag = tag;
            current_source_start = character;
        }
        else
        {
            character++;
        }
    }

    // Finish reading current shader source
    if(current_tag == nullptr)
    {
    }
    else if(strncmp("vertex", current_tag, current_tag_length) == 0)
    {
        *vert_source = current_source_start;
    }
    else if(strncmp("geometry", current_tag, current_tag_length) == 0)
    {
        *geom_source = current_source_start;
    }
    else if(strncmp("fragment", current_tag, current_tag_length) == 0)
    {
        *frag_source = current_source_start;
    }
}

GLuint make_shader(const char *shader_path)
{
    int  success;
    char info_log[512];


    char *memory = nullptr;
    char *vert_source = nullptr;
    char *geom_source = nullptr;
    char *frag_source = nullptr;
    read_shader_file(shader_path, &memory, &vert_source, &geom_source, &frag_source);



    unsigned int vert_shader;
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert_source, NULL);
    glCompileShader(vert_shader);
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
        log_error("VERTEX SHADER ERROR : %s\n%s\n", shader_path, info_log);
    }

    unsigned int frag_shader;
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_source, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        log_error("VERTEX SHADER ERROR : %s\n%s\n", shader_path, info_log);
    }


    unsigned int geom_shader;
    if(geom_source != nullptr)
    {
        geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geom_shader, 1, &geom_source, NULL);
        glCompileShader(geom_shader);
        glGetShaderiv(geom_shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(geom_shader, 512, NULL, info_log);
            log_error("VERTEX SHADER ERROR : %s\n%s\n", shader_path, info_log);
        }
    }

    check_gl_errors("compiling shaders");

    GLuint program = glCreateProgram();
    check_gl_errors("making program");

    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    if(geom_source != nullptr) glAttachShader(program, geom_shader);
    glLinkProgram(program);
    check_gl_errors("linking program");

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        log_error("SHADER LINKING ERROR : %s\n%s\n", shader_path, info_log);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader); 
    check_gl_errors("deleting shaders");

    return program;
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
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

void init_common_graphics()
{
    graphics_data = (GraphicsData *)calloc(1, sizeof(GraphicsData));



    create_gl_context();



    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    float vertices[] =
    {
        // pos         uv
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f,

        1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
    };

    glGenVertexArrays(1, &graphics_data->quad_vao);
    glBindVertexArray(graphics_data->quad_vao);
    check_gl_errors("making vao");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    check_gl_errors("making vbo");

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    check_gl_errors("send vbo data");

    float stride = 4 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(v2)));
    glEnableVertexAttribArray(1);
    check_gl_errors("vertex attrib pointer");



    graphics_data->quad_shader_program = make_shader("shaders/quad.shader");



    graphics_data->textures_map = new std::map<std::string, GLuint>();
}

void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        log_error("Error: %s\n", desc);
        assert(false);
    }
}

