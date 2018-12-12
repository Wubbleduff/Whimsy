//------------------------------------------------------------------------------
//
// File Name:  Graphics.h
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#pragma once

#include "Vector2D.h"

//------------------------------------------------------------------------------
// Public Structures:
//------------------------------------------------------------------------------

struct Color
{
  Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

  Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

  float r, g, b, a;
};

struct InstanceData
{
  // Information for calculation the transform on the GPU
  v2 position;
  v2 scale;
  float rotation;

  // Other information from the sprite
  Color color;
  v2 bottomLeftUV;
  v2 topRightUV;

  unsigned texture;
};

// Sprite handle
struct SpriteHandleData;
typedef SpriteHandleData *SpriteHandle;

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

void InitGraphics();

// Adds an item to the working vector of sprites
SpriteHandle AddSprite(v2 pos, v2 scale, float rotation, Color color, int layer, unsigned int texture,
                       v2 bottomLeftUV = { 0, 0 }, v2 topRightUV = { 1, 1 }, int shader = 0);

InstanceData *GetSpriteData(SpriteHandle handle);

void RemoveSprite(SpriteHandle *spritePtr);

SpriteHandle AddLine(v2 begin, v2 end, float thickness, Color color, int layer);

#if 0
void AddScreenSprite(v2 pos, v2 scale, float rotation, Color color, int layer, unsigned int texture,
                     v2 bottomLeftUV = { 0, 0 }, v2 topRightUV = { 1, 1 }, int shader = 0);
#endif

// Call this at the beginning of the frame to clear the screen, etc
void BeginGraphicsFrame();

// Call this at the end of a frame to swap buffers, poll events, etc
void EndGraphicsFrame();

// Create a shader for a sprite
unsigned int MakeShaderProgram(const char *vertexFile, const char *fragFile);

// Sets the information in the shader program to draw the mesh transformation list
void DrawGraphicsInstanceList();
void DrawGraphicsScreenInstanceList();

// Check if the window exists
bool WindowExists();

// These functions convert a QUANTITY of pixels to a measurement in device coordinates,
// NOT a position. Things shouldn't have to worry about position in pixels ever.
// 
// If a screen is 1920 pixels wide and 1920 pixels gets converted to device coordinates, this will
// correspond to 2.0f in device coordinates. This is correct for scaling but NOT position.
float PixelsToHorizontalDeviceCoordinates(int pixels);
float PixelsToVerticalDeviceCoordinates(int pixels);
int HorizontalDeviceCoordinatesToPixels(float horizontalDeviceCoordinate);
int VerticalDeviceCoordinatesToPixels(float verticalDeviceCoordinate);

// Converts full vectors of device coordinates or pixel coordinates from one to the other
v2 PixelsToDeviceCoords(v2 pixelCoords);
v2 DeviceToPixelCoords(v2 &deviceCoords);

// Convert a position on screen to a world position or vice versa
v2 DeviceCoordinateToWorldPosition(v2 deviceCoord);
v2 WorldPositionToDeviceCoordinate(v2 deviceCoord);

struct GLFWwindow;
GLFWwindow *GetEngineWindow();

