//------------------------------------------------------------------------------
//
// File Name:  Random.h
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// Public Structures:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

// Gets a random int between the min and max (inclusive)
int RandomInt(int min, int max);

// Gets a random floating point number between the min and max (inclusive)
float RandomFloat(float min, float max);

// Initializes the random number generator
void InitRand();
