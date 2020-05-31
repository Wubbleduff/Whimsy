
#include "particles.h"

#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"

#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

#include "common_graphics.h"

#include "imgui.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>


struct DesktopRendererData
{
    GLuint quad_vao;
    GLuint quad_shader_program;

    GLuint background_texture;
    GLuint foreground_texture;
};
static DesktopRendererData *destkop_renderer_data;

struct DesktopData
{
    bool running;

    float update_timer;

    int tick_times_end;
    float *tick_times;
    float average_tick_time;

    FILE *log_file;

    HGLRC gl_context;
};
static DesktopData *desktop_data;
// {major, minor}
static const int TARGET_GL_VERSION[2] = { 4, 4 };

static const int TIME_BUFFER_COUNT = 1000;







GLuint make_shader(const char *vert_source, const char *frag_source, const char *geom_source)
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
        fprintf(desktop_data->log_file, "SHADER ERROR: %s\n", info_log);
    }

    unsigned int frag_shader;
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_source, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        fprintf(desktop_data->log_file, "SHADER ERROR: %s\n", info_log);
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
            fprintf(desktop_data->log_file, "SHADER ERROR: %s\n", info_log);
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
        fprintf(desktop_data->log_file, "SHADER ERROR: %s\n", info_log);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader); 
    check_gl_errors("deleting shaders");

    return program;
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

void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        fprintf(desktop_data->log_file, "Error: %s\n", desc);
        assert(false);
    }
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
        desktop_data->gl_context = wglCreateContextAttribsARB(dc, 0, attribs);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temp_context);
        wglMakeCurrent(dc, desktop_data->gl_context);
    }
    else
    {   //It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
        desktop_data->gl_context = temp_context;
    }

    //Checking GL version
    const GLubyte *GLVersionString = glGetString(GL_VERSION);

    //Or better yet, use the GL3 way to get the version number
    int OpenGLVersion[2];
    glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);

    if(!desktop_data->gl_context) return; // Bad
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
}



static void init_desktop_renderer()
{
    destkop_renderer_data = (DesktopRendererData *)calloc(1, sizeof(DesktopRendererData));



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

    glGenVertexArrays(1, &destkop_renderer_data->quad_vao);
    glBindVertexArray(destkop_renderer_data->quad_vao);
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

      


    const char *vert_shader_source =
        "#version 440 core\n"
        "layout (location = 0) in vec2 a_pos;\n"
        "layout (location = 1) in vec2 a_uv;\n"
        "out vec2 uvs;\n"
        "void main()\n"
        "{\n"
        "    uvs = a_uv;\n"
        "    gl_Position = vec4(a_pos.x, a_pos.y, 0.0f, 1.0);\n"
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

    destkop_renderer_data->quad_shader_program = make_shader(vert_shader_source, frag_shader_source);

    destkop_renderer_data->background_texture = make_texture("textures/background.png");
    destkop_renderer_data->foreground_texture = make_texture("textures/foreground.png");
}

static void begin_frame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame();
}

static void end_frame()
{
  HDC dc = get_device_context();
  SwapBuffers(dc);
}

static void record_tick_time(float tick_time)
{
  if(desktop_data->tick_times_end >= TIME_BUFFER_COUNT) desktop_data->tick_times_end = 0;
  desktop_data->tick_times[desktop_data->tick_times_end] = tick_time;
  desktop_data->tick_times_end++;

  float average_time = 0.0f;
  int count = 0;
  for(int i = 0; i < TIME_BUFFER_COUNT; i++)
  {
    if(desktop_data->tick_times[i] == 0.0f) continue;

    average_time += desktop_data->tick_times[i];
    count++;
  }
  average_time /= (float)count;
  desktop_data->average_tick_time = average_time;
}



static void render_fullscreen_quad(GLuint texture_handle)
{
    // use quad mesh
    glBindVertexArray(destkop_renderer_data->quad_vao);
    check_gl_errors("use vao");

    // use shader
    glUseProgram(destkop_renderer_data->quad_shader_program);
    check_gl_errors("use program");
    
    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    // draw
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void render_background()
{
    render_fullscreen_quad(destkop_renderer_data->background_texture);
}

static void render_foreground()
{
    render_fullscreen_quad(destkop_renderer_data->foreground_texture);
}



void start_desktop()
{
    desktop_data = (DesktopData *)calloc(1, sizeof(DesktopData));

    desktop_data->tick_times_end = 0;
    desktop_data->tick_times = (float *)calloc(TIME_BUFFER_COUNT, sizeof(float));

    fopen_s(&(desktop_data->log_file), "output/particles_log.txt", "wt");

    init_platform();
    init_input();
    init_profiling();

    create_gl_context();
    init_imgui();

    init_desktop_renderer();

    init_particles();
     
    desktop_data->running = true;
    while(desktop_data->running)
    {
        platform_events();

        if(want_to_close())
        {
            desktop_data->running = false;
            break;
        }

        static const float FRAME_TIME = 0.016f;
        static const int MAX_UPDATES = 5;

        int update_count = 0;
        float dt = get_dt();
        if(dt >= 0.033f) dt = 0.033f;
        desktop_data->update_timer += dt;
        while(desktop_data->update_timer >= FRAME_TIME && update_count < MAX_UPDATES)
        {
            start_timer("tick");

            begin_frame();

            read_input();

            update_particles();

            //ImGui::ShowDemoWindow();
            ImGui::Begin("profiling");
            ImGui::Text("average tick seconds: %f", desktop_data->average_tick_time);
            ImGui::End();

            //glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            render_background();

            render_particles();

            render_foreground();

            end_frame();

            Sleep(14);

            update_count++;
            desktop_data->update_timer -= FRAME_TIME;

            float tick_time = end_timer();
            record_tick_time(tick_time);
        }
    }


    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
}
