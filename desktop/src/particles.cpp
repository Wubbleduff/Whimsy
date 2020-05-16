
#include "particles.h"
#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/wglew.h"

#include "imgui.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>














void init_renderer();
void shutdown_renderer();
void begin_frame();
void end_frame();
v2 mouse_world_position();

struct RenderParticle
{
    v2 position;
    v2 scale;
    float rotation;
    v4 color;
};

struct PhysicsParticle
{
    v2 velocity;
    float angular_velocity;
};


struct ParticleData
{
    int num_particles;
    RenderParticle *render_particles;
    PhysicsParticle *physics_particles;


    float update_timer;

    int tick_times_end;
    float *tick_times;
    float average_tick_time;

    FILE *log_file;
};
static ParticleData *particle_data;
static const int TIME_BUFFER_COUNT = 1000;

static const int NUM_PARTICLES = 100000;



void reset_particle_positions()
{
    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *p = &(particle_data->render_particles[i]);

        float percent = (float)i / particle_data->num_particles;
        float theta = 2.0f * PI * percent;

        float random_number = (rand() % 100000) / 100000.0f;

        p->position = v2(cos(theta), sin(theta)) * 5.0f * random_number;
        p->scale = v2(0.2f, 0.2f);
        p->color = v4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        PhysicsParticle *p = &(particle_data->physics_particles[i]);
        p->velocity = v2();
        p->angular_velocity = 0.0f;
    }
}

void init_particles()
{
    particle_data = (ParticleData *)calloc(1, sizeof(ParticleData));

    particle_data->num_particles = NUM_PARTICLES;
    particle_data->render_particles = (RenderParticle *)calloc(particle_data->num_particles, sizeof(RenderParticle));
    particle_data->physics_particles = (PhysicsParticle *)calloc(particle_data->num_particles, sizeof(PhysicsParticle));

    particle_data->tick_times_end = 0;
    particle_data->tick_times = (float *)calloc(TIME_BUFFER_COUNT, sizeof(float));

    fopen_s(&(particle_data->log_file), "output/particles_log.txt", "wt");



    init_renderer();




    reset_particle_positions();
}


void update_particles()
{
    v2 mouse_pos = mouse_world_position();

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *render = &(particle_data->render_particles[i]);
        PhysicsParticle *physics = &(particle_data->physics_particles[i]);

        v2 to_mouse = mouse_pos - render->position;
        if(mouse_state(0))
        {
            //p->velocity += unit(to_mouse) * 0.01f;
            v2 direction = unit(to_mouse);
            float mag = 1.0f / (length(to_mouse)*length(to_mouse));
            physics->velocity += direction * mag * 0.001f;
        }
        if(key_state('W'))
        {
          v2 direction = unit(to_mouse);
          float mag = length(to_mouse);
          physics->velocity += direction * mag * 0.001f;
        }

        if(key_state('E'))
        {
          v2 direction = unit(to_mouse);
          float mag = length(to_mouse) * length(to_mouse);
          physics->velocity += direction * mag * 0.001f;
        }



        //p->velocity -= p->velocity * 0.01f;
        render->position += physics->velocity;
    }

    //ImGui::Begin("E");
    if(key_toggled_down('R'))
    //if(ImGui::Button("Reset"))
    {
        reset_particle_positions();
    }
    //ImGui::End();
}


void record_tick_time(float tick_time)
{
    if(particle_data->tick_times_end >= TIME_BUFFER_COUNT) particle_data->tick_times_end = 0;
    particle_data->tick_times[particle_data->tick_times_end] = tick_time;
    particle_data->tick_times_end++;

    float average_time = 0.0f;
    int count = 0;
    for(int i = 0; i < TIME_BUFFER_COUNT; i++)
    {
        if(particle_data->tick_times[i] == 0.0f) continue;

        average_time += particle_data->tick_times[i];
        count++;
    }
    average_time /= (float)count;
    particle_data->average_tick_time = average_time;
}


























static const int NUM_VBOS = 2;
struct RendererData
{
    FILE *log;

    HWND window_handle;
    HGLRC gl_context;
    HDC device_context;
    int window_width;
    int window_height;
    float aspect_ratio;

    float camera_width;

    GLuint quad_vao;
    GLuint quad_ebo;
    GLuint quad_shader_program;

    GLuint particles_shader_program;

    GLuint particles_vao;
    GLuint particles_vbos[NUM_VBOS];
    int current_vbo_index;

    GLuint default_texture;
};
static RendererData *renderer_data;

// {major, minor}
static const int TARGET_GL_VERSION[2] = {4, 4};






static void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        fprintf(renderer_data->log, "Error: %s\n", desc);
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



static void init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init("#version 440 core");
    ImGui_ImplWin32_Init(get_window_handle());
    check_gl_errors("imgui");
}

static GLuint make_texture(const char *path)
{
    //stbi_set_flip_vertically_on_load(true);
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

static GLuint make_shader(const char *vert_source, const char *frag_source, const char *geom_source = nullptr)
{
    int  success;
    char info_log[512];

    unsigned int vert_shader;
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert_source, NULL);
    glCompileShader(vert_shader);
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
        fprintf(renderer_data->log, "SHADER ERROR: %s\n", info_log);
    }

    unsigned int frag_shader;
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_source, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        fprintf(renderer_data->log, "SHADER ERROR: %s\n", info_log);
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
            fprintf(renderer_data->log, "SHADER ERROR: %s\n", info_log);
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
        fprintf(renderer_data->log, "SHADER ERROR: %s\n", info_log);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader); 
    check_gl_errors("deleting shaders");

    return program;
}



static void set_particle_attrib_pointers()
{
    float stride = sizeof(RenderParticle);
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    // Scale
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(v2)));
    glEnableVertexAttribArray(1);
    // Rotation
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(v2)));
    glEnableVertexAttribArray(2);
    // Color
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(v2) + 1));
    glEnableVertexAttribArray(3);
}

static void init_renderer()
{
    renderer_data = (RendererData *)calloc(1, sizeof(RendererData));

    fopen_s(&renderer_data->log, "output/renderer_log.txt", "wt");

    HWND window_handle = get_window_handle();
    int width = get_window_width();
    int height = get_window_height();

    renderer_data->window_handle = window_handle;
    renderer_data->window_width = width;
    renderer_data->window_height = height;
    renderer_data->aspect_ratio = (float)width / (float)height;

    create_gl_context();

    init_imgui();

    glViewport(0, 0, width, height);


    float vertices[] =
    {
        // pos         uv
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f,
    };
    int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };


    {
        glGenVertexArrays(1, &renderer_data->quad_vao);
        glBindVertexArray(renderer_data->quad_vao);
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

        glGenBuffers(1, &renderer_data->quad_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_data->quad_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    }





    {
        glGenVertexArrays(1, &renderer_data->particles_vao);
        glBindVertexArray(renderer_data->particles_vao);
        check_gl_errors("making vao");


        for(int i = 0; i < NUM_VBOS; i++)
        {
            glGenBuffers(1, &renderer_data->particles_vbos[i]);
            check_gl_errors("making vbo");

            glBindBuffer(GL_ARRAY_BUFFER, renderer_data->particles_vbos[i]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(RenderParticle) * particle_data->num_particles,
                    0, GL_DYNAMIC_DRAW);
            check_gl_errors("send vbo data");
        }
        renderer_data->current_vbo_index = 0;

        set_particle_attrib_pointers();
        check_gl_errors("vertex attrib pointer");
    }








    {
        const char *vert_shader_source =
            "#version 440 core\n"
            "layout (location = 0) in vec2 a_pos;\n"
            "layout (location = 1) in vec2 a_uv;\n"
            "uniform mat4 mvp;\n"
            "out vec2 uvs;\n"
            "void main()\n"
            "{\n"
            "    uvs = a_uv;\n"
            "    gl_Position = mvp * vec4(a_pos.x, a_pos.y, 0.0f, 1.0);\n"
            "}\0";

        const char *frag_shader_source =
            "#version 440 core\n"
            "uniform sampler2D texture0;\n"
            "in vec2 uvs;"
            "out vec4 frag_color;\n"
            "void main()\n"
            "{\n"
            "    frag_color = texture(texture0, uvs);\n"
            //"    frag_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            //"    frag_color = vec4(uvs.x, uvs.y, 0.0f, 0.0f);\n"
            "}\0";

        renderer_data->quad_shader_program = make_shader(vert_shader_source, frag_shader_source);
    }




    {
        const char *vert_shader_source =
            "#version 440 core\n"
            "layout (location = 0) in vec2 a_pos;\n"
            "layout (location = 1) in vec2 a_scale;\n"
            "layout (location = 2) in vec2 a_rotation;\n"
            "layout (location = 3) in vec2 a_color;\n"

            "uniform mat4 vp;\n"

            "out VS_OUT\n"
            "{\n"
            "    mat4 mvp;\n"
            "} vs_out;\n"

            "void main()\n"
            "{\n"
            "    mat4 m = mat4(\n"
            "       a_scale.x, 0.0f, 0.0f, 0.0f,\n"
            "       0.0f, a_scale.y, 0.0f, 0.0f,\n"
            "       0.0f, 0.0f, 1.0f, 0.0f,\n"
            "       a_pos.x, a_pos.y, 0.0f, 1.0f\n"
            "       );\n"
            "    vs_out.mvp = vp * m;\n"
            "    gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0);\n"
            "}\0";

#if 1
        const char *geom_shader_source =
        {
            "#version 440 core\n"
            "layout (points) in;\n"
            "layout (triangle_strip, max_vertices = 4) out;\n"
            "in VS_OUT\n"
            "{\n"
            "    mat4 mvp;\n"
            "} gs_in[];\n"

            "out vec2 uvs;\n"
            
            "void main() {\n"
            "    mat4 mvp = gs_in[0].mvp;\n"
            "    uvs = vec2(0.0f, 0.0f);\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4(-0.5f, -0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(1.0f, 0.0f);\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4( 0.5f, -0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(0.0f, 1.0f);\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4(-0.5f,  0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(1.0f, 1.0f);\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4( 0.5f,  0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    EndPrimitive();\n"
            "}\n"
        };
#else
        const char *geom_shader_source =
        {
            "#version 440 core\n"
            "layout (points) in;\n"
            "layout (points, max_vertices = 1) out;\n"
            //"in mat4 mvp;\n"
            "void main() {\n"
            "    gl_Position = gl_in[0].gl_Position;\n"
            "    EmitVertex();\n"

            "    EndPrimitive();\n"
            "}\n"
        };
#endif

        const char *frag_shader_source =
            "#version 440 core\n"
            "uniform sampler2D texture0;\n"
            "in vec2 uvs;"
            "out vec4 frag_color;\n"
            "void main()\n"
            "{\n"
            "    frag_color = texture(texture0, uvs);\n"
            //"    frag_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            //"    frag_color = vec4(uvs.x, uvs.y, 0.0f, 1.0f);\n"
            "}\0";

        renderer_data->particles_shader_program = make_shader(vert_shader_source, frag_shader_source, geom_shader_source);
        check_gl_errors("particle shader");
    }


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);






    renderer_data->default_texture = make_texture("textures/particle.png");


    renderer_data->camera_width = 10.0f;
}

static void shutdown_renderer()
{
    fclose(renderer_data->log);
}


static mat4 get_ndc_m_world()
{
    float camera_width = renderer_data->camera_width;
    mat4 m =
    {
        2.0f / camera_width, 0.0f, 0.0f, 0.0f,
        0.0f, (2.0f / camera_width) * renderer_data->aspect_ratio, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    return m;
}

static v2 ndc_to_world(v2 p)
{
    float camera_width = renderer_data->camera_width;
    float x_term = (2.0f / camera_width);
    float y_term = (2.0f / camera_width) * renderer_data->aspect_ratio;
    v2 world = 
    {
        p.x * (1.0f / x_term),
        p.y * (1.0f / y_term)
    };

    return world;
}

v2 mouse_world_position()
{
    v2 p = mouse_screen_position();

    p.y = renderer_data->window_height - p.y;

    v2 ndc =
    {
        (p.x / renderer_data->window_width)  * 2.0f - 1.0f,
        (p.y / renderer_data->window_height) * 2.0f - 1.0f,
    };

    v2 world = ndc_to_world(ndc);
    return world;
}

static void render_quad(v2 pos, v2 scale)
{
    check_gl_errors("start draw");

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
    mat4 ndc_m_world = get_ndc_m_world();

    mat4 mvp = ndc_m_world * world_m_model;

    GLint loc = glGetUniformLocation(renderer_data->quad_shader_program, "mvp");
    glUniformMatrix4fv(loc, 1, true, &(mvp[0][0]));
    check_gl_errors("set uniform");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer_data->default_texture);

    glBindVertexArray(renderer_data->quad_vao);
    check_gl_errors("use vao");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_data->quad_ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    check_gl_errors("draw");

}

static void begin_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    
    check_gl_errors("begin frame");

    ImGui::NewFrame();

    
}

static __declspec(noinline) void do_the_sending()
{
    glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(RenderParticle) * particle_data->num_particles, particle_data->render_particles);
    check_gl_errors("send particle data");
}

static __declspec(noinline) void do_the_drawing()
{
    glDrawArrays(GL_POINTS, 0, particle_data->num_particles);
}

void render_particles()
{
    //glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#if 0
    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *p = &(particle_data->render_particles[i]);
        render_quad(p->position, p->scale);
    }
#else
    glUseProgram(renderer_data->particles_shader_program);


    mat4 ndc_m_world = get_ndc_m_world();

    GLint loc = glGetUniformLocation(renderer_data->particles_shader_program, "vp");
    glUniformMatrix4fv(loc, 1, true, &(ndc_m_world[0][0]));
    check_gl_errors("set uniform");


    glBindVertexArray(renderer_data->particles_vao);
    check_gl_errors("use vao");




    // Rotate VBOs
    //renderer_data->current_vbo_index = (renderer_data->current_vbo_index + 1) % NUM_VBOS;
    //GLuint next_vbo = renderer_data->particles_vbos[renderer_data->current_vbo_index];
    //glBindBuffer(GL_ARRAY_BUFFER, next_vbo);
    //set_particle_attrib_pointers();
    //check_gl_errors("rotate vbos");

    
    start_timer("send data");
    do_the_sending();
    float send_data_s = end_timer();


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer_data->default_texture);

    start_timer("render");
    do_the_drawing();
    float render_s = end_timer();

    check_gl_errors("draw");
 
#endif


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void end_frame()
{
    SwapBuffers(renderer_data->device_context);
}


/*
void add_particles(RenderParticle *particles, int num_particles)
{
}
*/
























