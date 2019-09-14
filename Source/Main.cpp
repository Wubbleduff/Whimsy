
#include "renderer2D.h"

#include "Random.h"
#include "Logging.h"

#include "profiling.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

//#include <windows.h>
#include <stdio.h>
#include <time.h>

// Worlds included here for now
#include "PhysicsTest.h"


static HWND window_handle;


static float previousTime;

static bool running;


#define MAX_BUTTONS 165
static bool keyStates[MAX_BUTTONS] = {};
static bool mouseStates[8] = {};

static v2 mousePosition;

static void imgui_init()
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui_ImplWin32_Init(window_handle);
  ImGui_ImplDX11_Init(get_d3d_device(), get_d3d_device_context());
}
static void imgui_frame()
{
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}
static void imgui_endframe()
{
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


bool ButtonDown(unsigned button)
{
  if(button < 0 || button > MAX_BUTTONS) return false;

  return keyStates[button];
}

bool MouseDown(unsigned button)
{
  if(button < 0 || button > 8) return false;

  return mouseStates[button];
}

v2 MouseWindowPosition()
{
  return mousePosition;
}

static void Init(unsigned monitor_width, unsigned monitor_height, bool is_fullscreen, bool is_vsync)
{
  InitLog();
  init_renderer(window_handle, monitor_width, monitor_height, is_fullscreen, is_vsync);
  InitRand();
  InitPhysicsTest();

  imgui_init();

}

static void Update()
{
  imgui_frame();
  ImGui::Begin("Hello");

  // Time how long the last frame took
  float currentTime = (float)time(NULL);
  float dt = (currentTime - previousTime);

  ImGui::Text("ms: "); ImGui::SameLine();
  ImGui::InputFloat("ms", &dt, 0.0f, 0.0f, "%.5f");

  // Mark the current frame's time so we can find the difference in time next frame
  previousTime = currentTime;

  // A timer that will progress along the engine's timeline
  static float simTimer;
  float maxTime = 1.0f;

  // This is how long each frame will be or the set time between frames
  float timeStep = 1.0f / 120.0f;

  // How many simulations has this frame done so far
  int sims = 0;

  // The maximum number of simulations the game loop can do each frame in case it gets too far behind.
  // You don't want an infinite loop of it taking too long and getting further behind.
  int maxSims = 5;

  // Count up the timeline
  simTimer += dt;// * 0.1f;

  // Update all systems of the engine for how many timesteps the timeline has crossed
  //while(simTimer >= timeStep && sims <= maxSims)
  {
#if 0
    UpdateDrawCollisions();
    UpdateParticles();
#endif
    UpdatePhysicsTest();

    // Count down the sim timer
    simTimer -= timeStep;
    sims++;
  }

  // Cap the sim timer if the program can't keep up
  if(simTimer >= maxTime)
  {
    simTimer = maxTime;
  }




  ImGui::End();
  render();
  imgui_endframe();
  swap_frame();


  //LogMessage("ms: %f\n", dt * 1000.0f);
}

static void ExitProgram()
{
  dump_profile_info();

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  DestroyWindow(window_handle);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  if(ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam)) return true;

  switch (message)
  {
    case WM_DESTROY:
    {
      running = false;
      PostQuitMessage(0);
      return 0;
    }
    break;

    case WM_CLOSE: 
    {
      running = false;
      return 0;
    }  
    break;

    case WM_KEYDOWN: 
    {
      keyStates[wParam] = true;
    }
    break;

    case WM_KEYUP:
    {
      keyStates[wParam] = false;
    }
    break;

    case WM_LBUTTONDOWN:
    {
      mouseStates[0] = true;
    }
    break;

    case WM_LBUTTONUP:
    {
      mouseStates[0] = false;
    }
    break;
    
    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
  }
  
  return result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Create the window class
  WNDCLASS window_class = {};
  window_class.style = CS_HREDRAW|CS_VREDRAW;
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = hInstance;
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  window_class.lpszClassName = "Windows Program Class";
  if(!RegisterClass(&window_class)) { return 1; }

  
  unsigned monitor_width = GetSystemMetrics(SM_CXSCREEN);
  unsigned monitor_height = GetSystemMetrics(SM_CYSCREEN);

  //window_handle = CreateWindowEx(0, window_class.lpszClassName, "First", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, monitor_width, monitor_height, 0, 0, hInstance, 0);                                
  window_handle = CreateWindowEx(0, window_class.lpszClassName, "First", WS_POPUP | WS_VISIBLE, 0, 0, monitor_width, monitor_height, 0, 0, hInstance, 0);                                
  if(!window_handle) { return 1; }


  // Initialize
  Init(monitor_width, monitor_height, false, false);


  // Main loop
  running = true;
  while(running)
  {
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
      if(message.message == WM_QUIT)
      {
        running = false;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    if(keyStates[VK_ESCAPE])
    {
      PostQuitMessage(0);
      running = false;
    }

    Update();
  }

  ExitProgram();
  return 0;
}

