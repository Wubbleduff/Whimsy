//------------------------------------------------------------------------------
//
// File Name:  Texture.cpp
// Author(s):  Daniel Berger
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "Texture.h"
#include "Graphics.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../Libraries/stb/stb_image.h"

//------------------------------------------------------------------------------
// Private Variables:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static std::map<std::string, int> textureMap;

static std::string path = "";

//------------------------------------------------------------------------------
// Forward References:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Functions:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Load a texure and return a "handle" to it
static bool LoadTexture(std::string file)
{
  int width = 0;
  int height = 0;
  int nChannels = 0;
  int format = GL_RGBA;
  int charType = GL_UNSIGNED_BYTE;
  GLenum errorboi = 0;

  // genenorate a texture
  unsigned int texture;
  glGenTextures(1, &texture);
  errorboi = glGetError();
  // bind the texture
  glBindTexture(GL_TEXTURE_2D, texture);
  errorboi = glGetError();
  // wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  errorboi = glGetError();
  // filtering 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  errorboi = glGetError();
  // load a image with paramiters into a channel 
  stbi_set_flip_vertically_on_load(true);
  
  // Try to load the file
  unsigned char *data = stbi_load(file.c_str(), &width, &height, &nChannels, 4);

  if(!data)
  {
    // Check the art assets folder
    std::string string = path + file;
    const char *fullPath = string.c_str();
    data = stbi_load(fullPath, &width, &height, &nChannels, 4);
  }

  if (data)
  {
    // creates the texture object to load the given data texture
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, charType, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // free the image data
    stbi_image_free(data);

    textureMap.emplace(file, texture);
  }
  else
  {

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

unsigned int GetTextureID(std::string texture)
{
  std::map<std::string, int>::iterator found;

  found = textureMap.find(texture);

  // Check if the texture is already loaded
  if(found != textureMap.end())
  {
    // Found the texture
    return found->second;
  }
  else
  {
    // Didn't find texture, load it
    bool loaded = LoadTexture(texture);

    if(!loaded)
    {
      return 0;
    }

    return textureMap[texture];
  }
}

// Frees a texture using a handle
void FreeTextures()
{
  std::map<std::string, int>::iterator it = textureMap.begin();
  std::map<std::string, int>::iterator last = it;

  ++it;

  for(; it != textureMap.end(); ++it)
  {
    GLuint *id = (GLuint *)(&last->second);
    glDeleteTextures(1, id);
    textureMap.erase(last->first);

    last = it;
  }

  if(last != textureMap.begin())
  {
    GLuint *id = (GLuint *)(&last->second);
    glDeleteTextures(1, id);
    textureMap.erase(last->first);
  }

}

std::map<std::string, int> *GetTextures()
{
  return &textureMap;
}

