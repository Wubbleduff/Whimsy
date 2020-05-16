
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



struct DesktopData
{
    bool running;

    float update_timer;
    float average_tick_time;
};
static DesktopData *desktop_data;

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


void start_desktop()
{
    desktop_data = (DesktopData *)calloc(1, sizeof(DesktopData));

    init_platform();
    init_input();
    init_profiling();

    create_gl_context();
    init_imgui();

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

            //start_timer("render");
            render();
            float render_secs = end_timer();

                





            end_frame();

            update_count++;
            desktop_data->update_timer -= FRAME_TIME;

            float tick_time = end_timer();
            record_tick_time(tick_time);
        }


    }


    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    shutdown_renderer();
}
