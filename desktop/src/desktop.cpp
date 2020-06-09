
#include "stars.h"

#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"

#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

#include "common_graphics.h"

#include "imgui.h"



#include <stdio.h>



struct DesktopData
{
    bool running;

    float update_timer;

    int tick_times_end;
    float *tick_times;
    float average_tick_time;

};
static DesktopData *desktop_data;

static const int TIME_BUFFER_COUNT = 1000;











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




static void begin_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
}

static void end_frame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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




static void draw_background()
{
    draw_fullscreen_quad("textures/background.png");
}

static void draw_foreground()
{
    draw_fullscreen_quad("textures/foreground.png");
}




void start_desktop()
{
    desktop_data = (DesktopData *)calloc(1, sizeof(DesktopData));

    desktop_data->tick_times_end = 0;
    desktop_data->tick_times = (float *)calloc(TIME_BUFFER_COUNT, sizeof(float));


    init_platform();
    init_input();
    init_profiling();

    init_common_graphics();
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


            glViewport(0, 0, get_screen_width(), get_screen_height());

            //glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            draw_background();

            draw_stars();

            draw_foreground();

            draw_reflected_stars();

            end_frame();

            //Sleep(14);

            update_count++;
            desktop_data->update_timer -= FRAME_TIME;

            float tick_time = end_timer();
            record_tick_time(tick_time);
        }
    }


    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
}

