#include "Graphics.H"

#include "Random.h"

int main()
{
  InitGraphics();
  InitRand();

#define NUM 200000

  static SpriteHandle handles[NUM];
  static float speeds[NUM];

#if 1
  for(int i = 0; i < NUM; i++)
  {
    handles[i] = AddSprite(v2(RandomFloat(-20.0f, 20.0f), RandomFloat(-20.0f, 20.0f)),
                           v2(1.0f, 1.0f),
                           RandomFloat(0.0f, 2 * PI),
                           Color(75.0f / 255.0f * 2, 0.0f, 130.0f / 255.0f * 2, RandomFloat(0.1f, 1.0f)),
                           1000,
                           0);
    speeds[i] = RandomFloat(0.1f, 1.0f);
  }
#endif

#if 0
  SpriteHandle handle = AddSprite(v2(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f)),
    v2(1.0f, 1.0f),
    RandomFloat(0.0f, 2 * PI),
    Color(1.0f, 0.0f, 0.0f, 1.0f),
    1000,
    0);
#endif


  while(WindowExists())
  {
    BeginGraphicsFrame();

    for(int i = 0; i < NUM; i++)
    {
      InstanceData *data = GetSpriteData(handles[i]);
      v2 v = data->position;
      v2 direction = v2(-v.y, v.x);
      data->position += direction * speeds[i] * 0.02f;
    }

    DrawGraphicsInstanceList();

    EndGraphicsFrame();
  }

  return 0;
}
