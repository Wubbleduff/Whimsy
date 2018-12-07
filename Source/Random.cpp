//------------------------------------------------------------------------------
//
// File Name:  Random.cpp
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "Random.h"
#include <stdlib.h> // For rand
#include <time.h> // For time

#include <assert.h>

//------------------------------------------------------------------------------
// Private Structures:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Variables:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Functions:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

// Gets a random int between the min and max (inclusive)
int RandomInt(int min, int max)
{
  // Make sure the user gives proper ranges
  if(max <= min)
  {
    return min;
  }

  return rand() % (max - min + 1) + min;
}

// Gets a random floating point number between the min and max (inclusive)
float RandomFloat(float min, float max)
{
  // Make sure the user gives proper ranges
  if(max <= min)
  {
    return min;
  }

  // Find a percent between 0% and 100% to 3 decimal places
  float percent = (float)(rand() % 1001) / 1000.0f;

  // Return a value in the range from the percent
  return ((max - min) * percent) + min;
}

// Initializes the random number generator
void InitRand()
{
  srand((unsigned int)time(0));
}
