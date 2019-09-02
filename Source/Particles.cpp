#include "Particles.h"
#include "Graphics.h"
#include "Input.h"
#include "Texture.h"
#include "Random.h"
#include "Logging.h"

#include <vector>
#include <thread>

#define Squared(x) (x * x)





#define SCARY
static void UpdateParticleRange(int startIndex, int endIndex, v2 mousePos, int buttonFlags);


#ifdef SCARY
#include <iostream>
#include <deque>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <random>
#include <atomic>


struct ThreadArgs
{
  int startIndex;
  int endIndex;
  v2 mousePos;
  int buttonFlags;
};

static std::vector< std::thread > workers;
static std::deque< ThreadArgs > workItems;
static std::mutex queue_mutex;
static std::condition_variable cv_task;
static std::condition_variable cv_finished;
static unsigned int busy;
static bool stop;

void thread_proc()
{
  while (true)
  {
    std::unique_lock<std::mutex> latch(queue_mutex);
    cv_task.wait(latch, [](){ return stop || !workItems.empty(); });
    if (!workItems.empty())
    {
      // got work. set busy.
      ++busy;

      // pull from queue
      ThreadArgs work = workItems.front();
      workItems.pop_front();

      // release lock. run async
      latch.unlock();

      // run function outside context
      UpdateParticleRange(work.startIndex, work.endIndex, work.mousePos, work.buttonFlags);


      latch.lock();
      --busy;
      cv_finished.notify_one();
    }
    else if (stop)
    {
      break;
    }
  }
}

// generic function push
void enqueue(ThreadArgs args)
{
  std::unique_lock<std::mutex> lock(queue_mutex);
  workItems.push_back(args);
  cv_task.notify_one();
}

// waits until the queue is empty.
void waitFinished()
{
  std::unique_lock<std::mutex> lock(queue_mutex);
  cv_finished.wait(lock, [](){ return workItems.empty() && (busy == 0); });
}


#endif // SCARY










struct Particle
{
  SpriteHandle sprite;

  v2 velocity;
};

static std::vector<Particle> particles;

static int textureID;

static int numParticlesToFire = 200;
//static v2 scaleRange = v2(0.04f, 0.1f);
static v2 scaleRange = v2(1.5f, 4.0f);
static v2 speedRange = v2(0.01f, 0.2f);
static v2 angleRange = v2(0.0f, 2.0f * PI);

static Color colors[] = 
{
  Color(0.0f, 0.0f, 1.0f, 1.0f),
  Color(0.75f, 0.75f, 0.75f, 1.0f),
#if 0
  Color(0.0f, 1.0f, 0.0f, 1.0f),
  Color(0.5f, 0.0f, 0.5f, 1.0f),
  Color(1.0f, 1.0f, 0.0f, 1.0f)
#endif
};

static unsigned int numThreads;

static v2 MoveInCircle(v2 particlePos, v2 mousePos)
{
  float r = 2.0f;

  v2 a;
  v2 b;

  particlePos -= mousePos;

  //if((mousePos - particlePos).lengthSquared() < squared(r))
  if(particlePos.Length() < r)
  {
    return particlePos - mousePos;
  }

  float radicand = (Squared(particlePos.y) * Squared(r)) * (Squared(particlePos.x) + Squared(particlePos.y) - Squared(r));

  if(radicand >= 0.0f && (particlePos.x + particlePos.y != 0.0f))
  {
    a.x = ((particlePos.x * Squared(r)) + sqrt(radicand)) / (Squared(particlePos.x) + Squared(particlePos.y));
    a.y = (Squared(r) - particlePos.x * a.x) / particlePos.y;

    b.x = ((particlePos.x * Squared(r)) - sqrt(radicand)) / (Squared(particlePos.x) + Squared(particlePos.y));
    b.y = (Squared(r) - particlePos.x * b.x) / particlePos.y;
  }
  else
  {
    return v2(0.0f, 0.0f);
  }

  float posY = particlePos.y;

  a += mousePos;
  b += mousePos;
  particlePos += mousePos;

  
  v2 f;

  if(particlePos.y == 0.0f)
  {
    return v2(0.0f, 0.0f);
  }
  else if(posY < 0.0f)
  {
    f = a - particlePos;
  }
  else
  {
    f = b - particlePos;
  }

  f = f.Rotated(RandomFloat(-PI / 5.0f, PI / 5.0f));
  f *= RandomFloat(1.0f, 20.0f);

  return f;
}

static void UpdateParticle(v2 *positionPtr, v2 *velocityPtr, v2 mousePos, int buttonFlags)
{
  v2 &pos = *positionPtr;
  v2 &velocity = *velocityPtr;
#if 1
  v2 f = v2(0.0f, 0.0f);
  v2 v = mousePos - pos;

  // Q
  if(buttonFlags & (1 << 0))
  {
    // 1 / d^2
    f += 1.5f * v / v.LengthSquared();
    //f = 1.0f * f / (pow(f.length(), 4));
  }

  // W
  if(buttonFlags & (1 << 1))
  {
    // d^2
    f += 0.02f * v * (v.LengthSquared());
  }

  // E
  if(buttonFlags & (1 << 2))
  {
    // Linear
    f += v.Unit();
  }

  // A
  if(buttonFlags & (1 << 3))
  {
    f += MoveInCircle(pos, mousePos);

    f *= f.Length();
  }

  f = f.ClampLength(2.0f);

  // Shift
  if(buttonFlags & (1 << 4))
  {
    f *= -1.0f;
  }
  // Space
  if(buttonFlags & (1 << 5))
  {
    velocity *= 0.9f;
  }

  velocity += f * 0.01f;
  velocity = velocity.ClampLength(3.0f);


  velocity -= velocity * 0.005f;


  velocity = velocity.ClampLength(2.0f);
#endif
  pos += velocity;


  //particles[particleIndex] = particle;
  //data->position = pos;

  //return pos;
}

static void UpdateParticleRange(int startIndex, int endIndex, v2 mousePos, int buttonFlags)
{
  if(endIndex == 0)
  {
    return;
  }

  std::vector<v2> velocities;
  std::vector<v2> positions;

  for(int i = startIndex; i < endIndex; i++)
  {
    Particle particle = particles[i];
    InstanceData data = *GetSpriteData(particle.sprite);

    UpdateParticle(&data.position, &particle.velocity, mousePos, buttonFlags);

    particles[i].velocity = particle.velocity;
    GetSpriteData(particle.sprite)->position = data.position;
  }
}

static void AddParticle(v2 mousePos)
{
  v2 scale = v2(1.0f, 1.0f) * RandomFloat(scaleRange.x, scaleRange.y);
  float angle = RandomFloat(angleRange.x, angleRange.y);
  float speed = RandomFloat(speedRange.x, speedRange.y);
  Color color = colors[RandomInt(0, _countof(colors) - 1)];


  Particle particle;
  particle.sprite = AddSprite(mousePos, scale, 0.0f, color, 0, textureID);
  particle.velocity = v2(speed, 0.0f).Rotated(angle);
  particles.push_back(particle);
}

void InitParticles()
{
  textureID = GetTextureID("DefaultCircle.png");

  for(int i = 0; i < 100; i++)
  {
    AddParticle(v2());
  }

  numThreads = std::thread::hardware_concurrency();
  //numThreads = 8;

  for (unsigned int i = 0; i < numThreads; i++)
  {
    workers.push_back(std::thread(thread_proc));
  }
}

void UpdateParticles()
{
  v2 mousePos = MouseWorldPosition();

  if(ButtonDown(0))
  {
    for(int i = 0; i < 100; i++)
    {
      AddParticle(mousePos);
    }
  }

  int buttonFlags = 0;
  if(ButtonDown('Q'))
  {
    buttonFlags |= 1 << 0;
  }
  if(ButtonDown('W'))
  {
    buttonFlags |= 1 << 1;
  }
  if(ButtonDown('E'))
  {
    buttonFlags |= 1 << 2;
  }
  if(ButtonDown('A'))
  {
    buttonFlags |= 1 << 3;
  }
  // Shift
  if(ButtonDown(340))
  {
    buttonFlags |= 1 << 4;
  }
  if(ButtonDown(' '))
  {
    buttonFlags |= 1 << 5;
  }
  
#define MULTI_THREADED

#ifdef MULTI_THREADED

#ifdef SCARY

  int numChunks = numThreads * 32;
  int chunk = particles.size() / numChunks;
  for(int i = 0; i < numChunks; i++)
  {
    ThreadArgs args;
    args.startIndex = i * chunk;
    args.endIndex = (i + 1) * chunk;
    args.mousePos = mousePos;
    args.buttonFlags = buttonFlags;

    enqueue(args);
  }

  waitFinished();
#else // SCARY

  std::vector<std::thread> threads;
  int chunk = particles.size() / NUM_THREADS;
  for(int i = 0; i < NUM_THREADS; i++)
  {
    int start = i * chunk;
    int end = (i + 1) * chunk;
    threads.push_back(std::thread(UpdateParticleRange, start, end, mousePos, buttonFlags));
  }

  for(int i = 0; i < NUM_THREADS; i++)
  {
    threads[i].join();
  }

  threads.clear();
#endif // SCARY
#else // MULTI_THREADED
  UpdateParticleRange(0, particles.size(), mousePos, buttonFlags);
#endif // MUTI_THREADED


}

void ExitParticles()
{
  // set stop-condition
  std::unique_lock<std::mutex> latch(queue_mutex);
  stop = true;
  cv_task.notify_all();
  latch.unlock();

  // all threads terminate, then we're done.
  for (auto& t : workers)
    t.join();
}
