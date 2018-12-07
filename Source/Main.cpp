#include "Graphics.H"

#include "Random.h"

int main()
{
  InitGraphics();
  //InitRand();
#define NUM 10

  static SpriteHandle handles[NUM];

  for(int iterations = 0; iterations < 1; iterations++)
  {
    for(int i = 0; i < NUM; i++)
    {
      handles[i] = AddSprite(v2(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f)), v2(1.0f, 1.0f), RandomFloat(0.0f, 2 * PI), Color(1.0f, 0.0f, 0.0f, 1.0f), 1000, 0);
    }

#if 0
    for(int i = 0; i < NUM; i++)
    {
      int index = NUM - 1;
      while(handles[index] == 0)
      {
        index = RandomInt(0, NUM);
      }

      RemoveSprite(&handles[index]);
    }
#else
    for(int i = 0; i < NUM; i++)
    {
      RemoveSprite(&handles[i]);
    }
#endif
  }

  /*
  SpriteHandle handle = AddSprite(v2(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f)), v2(1.0f, 1.0f), RandomFloat(0.0f, 2 * PI), Color(1.0f, 0.0f, 0.0f, 1.0f), 1000, 0);
  handle = AddSprite(v2(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f)), v2(1.0f, 1.0f), RandomFloat(0.0f, 2 * PI), Color(1.0f, 0.0f, 0.0f, 1.0f), 1000, 0);
  handle = AddSprite(v2(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f)), v2(1.0f, 1.0f), RandomFloat(0.0f, 2 * PI), Color(1.0f, 0.0f, 0.0f, 1.0f), 1000, 0);

  RemoveSprite(&handle);
  */


  while(WindowExists())
  {
    BeginGraphicsFrame();

    DrawGraphicsInstanceList();

    EndGraphicsFrame();
  }

  return 0;
}
