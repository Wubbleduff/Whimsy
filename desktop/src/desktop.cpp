
#include "stars.h"

#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"
#include "scene.h"

#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

#include "common_graphics.h"

#include "imgui.h"

//#include "easy/profiler.h"



#include <stdio.h>



struct DesktopData
{
    bool running;

    int ticks;
    float tick_timer;
    float target_tick_seconds;

    v2 scene_position;
    v2 scene_dimensions;;
    float horizon_t;
    float side_favor;

    bool paused = false;
};
static DesktopData *desktop_data = nullptr;

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
    //EASY_FUNCTION();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();
}

__declspec(noinline) static void end_frame()
{
    //EASY_FUNCTION();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    static bool done_once = false;
    if(!done_once)
    {
        HDC dc = get_device_context();
        SwapBuffers(dc);
        done_once = true;
    }
    

    glFinish();
}

float get_scene_width()
{
    return desktop_data->scene_dimensions.x;
}

float get_scene_height()
{
    return desktop_data->scene_dimensions.y;
}

float get_sky_height()
{
    float scene_height = get_scene_height();
    return scene_height * (1.0f - desktop_data->horizon_t);
}

v2 get_scene_bottom_left()
{
    return desktop_data->scene_position - desktop_data->scene_dimensions / 2.0f;
}

v2 get_scene_top_right()
{
    return desktop_data->scene_position + desktop_data->scene_dimensions / 2.0f;
}

static void preload_textures()
{
    get_or_make_texture("textures/Background/Stars_Back001.png");
    get_or_make_texture("textures/Background/Sky_Transparent003.png");
    get_or_make_texture("textures/Midground/Mountains_Back004.png");
    get_or_make_texture("textures/Midground/Mountains_Mid_Transparent005.png");
    get_or_make_texture("textures/Foreground/Forest_Foreground006.png");
    get_or_make_texture("textures/Foreground/Forest_CloserForeground007.png");
    get_or_make_texture("textures/Foreground/Forest_ClosestForeground008.png");
    get_or_make_texture("textures/Foreground/Wizard_Foreground009.png");
    get_or_make_texture("textures/Misc. TOP/Signature.png");
}

static void adjust_camera()
{
    //EASY_FUNCTION();

    ImGui::SliderFloat("side favor", &desktop_data->side_favor, 0.0f, 1.0f);

    float scene_width = get_scene_width();
    float scene_height = get_scene_height();
    float scene_aspect_ratio = scene_width / scene_height;
    float screen_aspect_ratio = get_aspect_ratio();

    v2 camera_position = v2();
    float max_offset;
    if(screen_aspect_ratio > scene_aspect_ratio)
    {
        set_camera_width(scene_width);
        float camera_height = 1.0f / screen_aspect_ratio;

        float diff = camera_height - scene_height;

        float t = 1.0f - desktop_data->side_favor;
        camera_position.y += diff * (t - 0.5f);
    }
    else
    {
        float camera_width = scene_height * screen_aspect_ratio;
        set_camera_width(camera_width);

        float diff = camera_width - scene_width;

        float t = 1.0f - desktop_data->side_favor;
        camera_position.x += diff * (t - 0.5f);
    }

    float half_scene_height = scene_height / 2.0f;
    float scene_offset = half_scene_height - desktop_data->horizon_t * (half_scene_height * 2.0f);
    v2 pos = v2(0.0f, scene_offset);

    camera_position.y += scene_offset;
    set_camera_position(camera_position);
}

v2 mouse_world_position()
{
    v2 p = mouse_screen_position();
    float screen_width = get_screen_width();
    float screen_height = get_screen_height();

    p.y = screen_height - p.y;

    v2 ndc =
    {
        (p.x / screen_width)  * 2.0f - 1.0f,
        (p.y / screen_height) * 2.0f - 1.0f,
    };

    return ndc_point_to_world(ndc);
}

static void draw_scene()
{
    //EASY_FUNCTION();

    v2 pos = desktop_data->scene_position;
    float scene_width = get_scene_width();
    float scene_height = get_scene_height();

    //EASY_BLOCK("Background");
    v2 scale = v2(scene_width, scene_height);
    draw_quad("textures/Background/Stars_Back001.png", pos, scale);
    //EASY_END_BLOCK;

    draw_stars();

    draw_reflected_stars();

    //EASY_BLOCK("Foreground");
    draw_quad("textures/Background/Sky_Transparent003.png", pos, scale);
    draw_quad("textures/Midground/Mountains_Back004.png", pos, scale);
    draw_quad("textures/Midground/Mountains_Mid_Transparent005.png", pos, scale);
    draw_quad("textures/Foreground/Forest_Foreground006.png", pos, scale);
    draw_quad("textures/Foreground/Forest_CloserForeground007.png", pos, scale);
    draw_quad("textures/Foreground/Forest_ClosestForeground008.png", pos, scale);
    draw_quad("textures/Foreground/Wizard_Foreground009.png", pos, scale);
    draw_quad("textures/Misc. TOP/Signature.png", pos, scale);
    //EASY_END_BLOCK;

    //draw_quad(v4(1, 1, 1, 1), v2(), v2(1, 1) * 0.9f);
    float the_x = -0.5f;
    float gap = scene_width / 9.0f;
    for(int i = 0; i < 10; i++)
    {
        v2 d_pos = v2(the_x, 0.0f);
        the_x += gap;
        v2 d_scale = v2(1, 1) * 0.01f;
        draw_quad(v4(1, 1, 1, 1), d_pos, d_scale);
    }


    glFlush();
    glFinish();
}


static void do_one_tick()
{
    //EASY_FUNCTION(0xfff080aa);

    begin_frame();

    read_input();

    update_particles();

    adjust_camera();


    glViewport(0, 0, get_screen_width(), get_screen_height());
    //glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    draw_scene();



    //ImGui::ShowDemoWindow();
    static bool a = false;
    ImGui::Checkbox("144", &a);
    if(a)
    {
        desktop_data->target_tick_seconds = 1.0f / 144.0f;
    }
    else
    {
        desktop_data->target_tick_seconds = 1.0f / 60.0f;
    }



    end_frame();
}





void start_desktop()
{
    desktop_data = (DesktopData *)calloc(1, sizeof(DesktopData));

    desktop_data->horizon_t = 1.0f - 0.634f;

    //EASY_PROFILER_ENABLE;

    init_platform();
    init_input();
    init_profiling();

    init_common_graphics();
    init_imgui();

    {
        float texture_aspect_ratio = 4800.0f / 3000.0f;
        float scene_width = 1.0f;
        float scene_height = 1.0f / texture_aspect_ratio;
        float scene_aspect_ratio = scene_width / scene_height;
        float screen_aspect_ratio = get_aspect_ratio();

        float half_scene_height = scene_height / 2.0f;
        float scene_offset = half_scene_height - desktop_data->horizon_t * (half_scene_height * 2.0f);
        v2 pos = v2(0.0f, scene_offset);
        desktop_data->scene_position = pos;
        desktop_data->scene_dimensions = v2(scene_width, scene_height);
    }

    init_particles();

#if 0
    int freq = get_monitor_frequency();
#else
    int freq = 60;
#endif
    desktop_data->target_tick_seconds = 1.0f / (float)freq;


    //preload_textures();


     
    desktop_data->running = true;
    desktop_data->tick_timer = 0.0f;
    while(desktop_data->running)
    {
        //EASY_BLOCK("Main loop");

        platform_events();

        bool bwant_to_close = want_to_close();
        if(bwant_to_close)
        {
            desktop_data->running = false;
            break;
        }

        static const int MAX_TICKS = 10;

        int current_tick_count = 0;
        float dt = get_dt();
        dt = min(dt, 0.033f);
        desktop_data->tick_timer += dt;

        // Do ticks
        while((desktop_data->tick_timer >= desktop_data->target_tick_seconds) && !desktop_data->paused)
        {
            do_one_tick();

            current_tick_count++;
            desktop_data->tick_timer -= desktop_data->target_tick_seconds;

            if(current_tick_count >= MAX_TICKS)
            {
                log_error("Exceeded max tick count! tick: %i", desktop_data->ticks);
                desktop_data->tick_timer = 0.0f;
                break;
            }
        }

        if(desktop_data->paused)
        {
            Sleep(desktop_data->target_tick_seconds * 1000.0f);
        }
        else
        {
            //EASY_BLOCK("Sleep", 0x22222222);
            float time_til_end_of_tick = desktop_data->target_tick_seconds - desktop_data->tick_timer;
            float pad_ms = desktop_data->target_tick_seconds * 0.1f;
            float sleep_time = time_til_end_of_tick - pad_ms;
            sleep_time = max(sleep_time, 0.0f);
            Sleep(sleep_time * 1000.0f);
            //EASY_END_BLOCK;
        }

    }


    //profiler::dumpBlocksToFile("output/test_profile.prof");


    ImGui_ImplWin32_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
}

void pause_desktop()
{
    if(desktop_data != nullptr)
    {
        desktop_data->paused = true;
    }
}

void unpause_desktop()
{
    if(desktop_data != nullptr)
    {
        desktop_data->paused = false;
    }
}

