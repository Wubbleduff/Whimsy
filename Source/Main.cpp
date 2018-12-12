#include "Graphics.h"
#include "Input.h"
#include "Random.h"
#include "Logging.h"

#include <stdio.h>
#include "GLFW/glfw3.h" // For getting time

// Worlds included here for now
#include "DrawCollisions.h"
#include "Particles.h"


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
#else
  InitParticles();
#endif
}

static void Update()
{
  while(WindowExists() && running)
  {
    // Time how long the last frame took
    float currentTime = (float)glfwGetTime();
    float dt = (currentTime - previousTime);

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
    simTimer += dt;

    // Update all systems of the engine for how many timesteps the timeline has crossed
    while (simTimer >= timeStep && sims <= maxSims)
    {
#if 0
      UpdateDrawCollisions();
#else
      UpdateParticles();
#endif

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

    BeginGraphicsFrame();

    DrawGraphicsInstanceList();

    EndGraphicsFrame();

    //LogMessage("ms: %f\n", dt * 1000.0f);
  }
}

int main()
{
  Init();

  Update();

  return 0;
}

