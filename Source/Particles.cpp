#include "Particles.h"
#include "Graphics.h"
#include "Input.h"
#include "Texture.h"
#include "Random.h"
#include "Logging.h"

#include <vector>
#include <thread>

#define Squared(x) (x * x)

struct Particle
{
  SpriteHandle sprite;

  v2 velocity;
};

static std::vector<Particle> particles;

static int textureID;

static int numParticlesToFire = 200;
static v2 scaleRange = v2(0.04f, 0.1f);
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
  Color color = colors[RandomInt(0, _countof(colors))];


  Particle particle;
  particle.sprite = AddSprite(mousePos, scale, 0.0f, color, 0, textureID);
  particle.velocity = v2(speed, 0.0f).Rotated(angle);
  particles.push_back(particle);
}

void InitParticles()
{
  textureID = GetTextureID("DefaultCircle.png");

  for(int i = 0; i < 200; i++)
  {
    AddParticle(v2());
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

#define MULTI_THREADEDx

#ifdef MULTI_THREADED
#define NUM_THREADS 1

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
#else
  UpdateParticleRange(0, particles.size(), mousePos, buttonFlags);
#endif


}
