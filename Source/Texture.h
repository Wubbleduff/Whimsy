//------------------------------------------------------------------------------
//
// File Name:  Texture.h
// Author(s):  daniel.berger
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#pragma once

#include <string>
#include <map>

//------------------------------------------------------------------------------
// Public Structures:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Gets the ID associated with the texture name
unsigned int GetTextureID(std::string texture);

// Gets the beginning of the texture list so you can loop through all textures
//std::map<std::string, unsigned int> *GetTextureMap();

// Load a texture given a file name.
// This should be the file name of the asset in one of the specified folders
// including the extension.
// Does not need to include path.
//void LoadTexture(std::string file);

// Frees a texture using a handle
void FreeTextures();

std::map<std::string, int> *GetTextures();
