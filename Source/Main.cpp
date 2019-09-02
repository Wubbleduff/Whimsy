#include "Graphics.h"
#include "Input.h"
#include "Random.h"
#include "Logging.h"

#include "profiling.h"

#include "GLFW/glfw3.h" // For getting time

#include "imgui.h"
#include "examples\imgui_impl_glfw.h"
#include "examples\imgui_impl_opengl3.h"

#include <stdio.h>

// Worlds included here for now
#include "DrawCollisions.h"
#include "Particles.h"
#include "PhysicsTest.h"


static float previousTime;

static bool running;

static void Init()
{
  InitLog();
  InitGraphics();
  InitRand();

  running = true;

#if 0
  InitDrawCollisions();
  InitParticles();
#endif

  InitPhysicsTest();

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui_ImplGlfw_InitForOpenGL(GetEngineWindow(), true);
  ImGui_ImplOpenGL3_Init("#version 150");
}

static void Update()
{
  while(WindowExists() && running)
  {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello");



    // Time how long the last frame took
    float currentTime = (float)glfwGetTime();
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
    while(simTimer >= timeStep && sims <= maxSims)
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

    // Escape
    if(ButtonDown(256))
    {
      running = false;
    }


    ImGui::End();


    BeginGraphicsFrame();

    DrawGraphicsInstanceList();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    EndGraphicsFrame();


    //LogMessage("ms: %f\n", dt * 1000.0f);
  }
}

static void ExitProgram()
{
  ExitParticles();
  dump_profile_info();
}

int main()
{
  Init();

  Update();

  ExitProgram();

  return 0;
}

