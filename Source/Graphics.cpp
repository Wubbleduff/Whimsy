//----------------------------------------------------------------------
//
//  File name: Graphics.cpp
//  Author(s): Daniel Onstott
//  Description:
//    The goal of this file is to set up our window as well as process
//    the draw step of our engine.
//  following https://learnopengl.com/Getting-started/Hello-Window
//
//  Copyright © 2018 DigiPen (USA) Corporation.
//
//----------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>

#include "Graphics.h"
#include "Texture.h"
#include "glew.h"
#include "GLFW/glfw3.h"

#include <thread>
#include <list>

//------------------------------------------------------------------------------
// Private Structures:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Variables:
//------------------------------------------------------------------------------

// Window
static GLFWwindow *window;
static bool fullScreen = false;

// Window dimensions
static v2 viewportDimensions = {1920, 1080};

// Mesh handle
static unsigned int meshHandle;
static GLuint instanceDataVBO;

// Shader handle
static GLuint instanceShaderProgram;

// Default texture
GLuint defaultTexture;

// Camera
float cameraZoom = 20.0f;

// Maximum number of instances per group
static const unsigned MAX_INSTANCES = 300000;

// Vertices of the unit square mesh
static float vertices[] = {
  // Shape            // Texture
  -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
   0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
  -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
   0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
};

#define TEXTURE_CHUNK 32

struct SpriteHandleData
{
  unsigned int index;
  struct InstanceGroup *group;

  struct SpriteHandleData *next;
  struct SpriteHandleData *prev;

  SpriteHandleData(unsigned int i, InstanceGroup *inGroup) : index(i), group(inGroup), next(0), prev(0) {}
};

// Instance group memory must be contiguous
struct InstanceGroup
{
  std::vector<InstanceData> data;
  SpriteHandleData *tail;
  int shader;

  InstanceGroup() : tail(0), shader(0) {}
};

// Maps texture chunk values to instance groups
typedef std::vector<InstanceGroup> InstanceGroupList;

// Maps layers to instance group lists
static std::map<int, InstanceGroupList> instanceGroups;

// Maps layers to instance group lists
#if 0
static std::vector<InstanceGroupList> instanceScreenGroups;
#endif

//------------------------------------------------------------------------------
// Private Functions:
//------------------------------------------------------------------------------
static void CreateWindow()
{
  window = glfwCreateWindow((int)viewportDimensions.x, (int)viewportDimensions.y, "", NULL, NULL);

  // create a window of (sizeX, sizeY, "name", monitor, share) we ignore the last two
  if (window == NULL)
  {
    // if it failed
    glfwTerminate();
    exit(-1);
  }

  glfwMakeContextCurrent(window);

  // tell openGL to render within these relative coordinates
  int x;
  int y;
  glfwGetFramebufferSize(window, &x, &y);
  viewportDimensions.x = (float)x;
  viewportDimensions.y = (float)y;
  glViewport(0, 0, (GLsizei)viewportDimensions.x, (GLsizei)viewportDimensions.y);
}

// Read source code from a shader file, places the data into the given string
char *ReadFile(const char *fname)
{
  // open a file for input
  std::ifstream file;
  file.open(fname);
  if (!file)
  {
    return 0;
  }
  // find the size of the read file
  file.seekg(0, std::ios::end);

  // denote the size
  unsigned size = static_cast<unsigned>(file.tellg());

  // return to beginning
  file.seekg(0, std::ios::beg);

  // Make a buffer for the contents
  char *data = new char[size];

  // Read the contents in
  file.read(data, size);

  // Check if there will be leftover data. This could happen because the read
  // interprets a carriage return and a newline as 1 character.
  if(file.gcount() != size)
  {
    // Null terminate the string
    data[file.gcount()] = 0;
  }

  return data;
}

// generate and apply VBO, VAO, EBO and instance VBO!
static void GenerateBufferObjects()
{
  GLuint VAOindex = 0;
  
  // create a VAO to bind or VBO to 
  // without this we would be SOL
  GLuint VAO;
  glGenVertexArrays(1, &VAO);

  // bind the VAO as we have a triangle to draw
  glBindVertexArray(VAO);

  // Generate a new buffer object
  GLuint VBO;
  glGenBuffers(1, &VBO);
  
  // bind the vertex buffer object to the array buffer slot
  // we can have many vbos in different slots
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  // copies previously defined vertex data into this buffer's memory
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // tell open gl how to interpret our vbos
  // the triangle is three floats, total size 3 * float
  glVertexAttribPointer(VAOindex, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(VAOindex++);
  // handles the textures
  glVertexAttribPointer(VAOindex, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
  glEnableVertexAttribArray(VAOindex++);


  // unbinds this VBO so I can do the next one, probably
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Generate one for the full list
  glGenBuffers(1, &instanceDataVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceDataVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * MAX_INSTANCES, 0, GL_STREAM_DRAW);

  // Enable the other pointers here too
  // do this part for each thing in the instance data

  // Position
  glVertexAttribPointer(VAOindex, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)0);
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);
  
  // Scale
  glVertexAttribPointer(VAOindex, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(float) * 2));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  // Rotation
  glVertexAttribPointer(VAOindex, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(float) * 4));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  // Color
  glVertexAttribPointer(VAOindex, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(float) * 5));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  // Bottom left UV
  glVertexAttribPointer(VAOindex, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(float) * 9));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  // Top right UV
  glVertexAttribPointer(VAOindex, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(float) * 11));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  // Texture
  glVertexAttribIPointer(VAOindex, 1, GL_UNSIGNED_INT, sizeof(InstanceData), (void*)(sizeof(float) * 13));
  glVertexAttribDivisor(VAOindex, 1);
  glEnableVertexAttribArray(VAOindex++);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

}

// Window resize callback function (called every time the window resizes)
static void WindowResizeCallbackFunction(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
  viewportDimensions.x = (float)width;
  viewportDimensions.y = (float)height;
}

static SpriteHandleData *AddHandleToEndOfGroup(unsigned int index, InstanceGroup *group)
{
  SpriteHandleData *newNode = new SpriteHandleData(index, group);

  int nextIndex = 0;

  if(group->tail != 0)
  {
    SpriteHandleData *tail = group->tail;

    nextIndex = tail->index + 1;
    tail->next = newNode;
    newNode->prev = tail;
  }

  group->tail = newNode;
  group->tail->index = nextIndex;

  return newNode;
}

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

// Draw an arrow using world coordinates
#if 0
void DebugDrawArrow(v2 begin, v2 end, float thickness, Color color, int layer)
{
  v2 vector = end - begin;
  float rot = atan2f(vector.y, vector.x);
  float length = vector.Length();
  v2 scale;
  v2 pos = begin + (vector / 2.0f);

  scale.x = length;
  scale.y = thickness;

  AddSprite(pos, scale, rot, color, layer, GetTextureID("Default.png"));
}
#endif

// Draw an arrow using screen coordinates
#if 0
void DebugDrawScreenArrow(v2 begin, v2 end, float thickness, Color color, int layer)
{
  v2 vector = end - begin;
  float rot = atan2f(vector.y, vector.x);
  float length = vector.Length();
  v2 scale;
  v2 pos = begin + (vector / 2.0f);

  scale.x = length;
  scale.y = thickness;

  AddScreenSprite(pos, scale, rot, color, layer, GetTextureID("Default.png"));
}
#endif

static void UseShader(int shader)
{
  // Prep texture uniform for binding
  // Use the instancing shader
  glUseProgram(shader);

  // Set up the samler2D array to use the bound texture values
  int texLocation = glGetUniformLocation(shader, "ourTexture");
  int a[TEXTURE_CHUNK];
  for(int i = 0; i < TEXTURE_CHUNK; i++)
  {
    a[i] = i;
  }

  glUniform1iv(texLocation, TEXTURE_CHUNK, a);
}

static void UseCamera(int shader, bool isScreenSprite)
{
  // Set the new transform matrix
  float matrix[4][4] =
  {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  if(!isScreenSprite)
  {
    matrix[0][0] = 2.0f / viewportDimensions.x * cameraZoom;
    matrix[1][1] = 2.0f / viewportDimensions.y * cameraZoom;
  }

  // Send the camera matrix to the shader
  int cameraMatrixLocation = glGetUniformLocation(shader, "cameraMatrix");
  if(cameraMatrixLocation == -1)
  {
  }

  glProgramUniformMatrix4fv(shader, cameraMatrixLocation, 1, 0, &(matrix[0][0]));
}

//------------------------------------------------------------------------------
// Public Draw Functions:
//------------------------------------------------------------------------------

// Sets the information in the shader program to draw the mesh transformation list
void DrawGraphicsInstanceList()
{
  // For each layer
  for(std::pair<const int, InstanceGroupList> &groupListPair : instanceGroups)
  {
    InstanceGroupList &groupList = groupListPair.second;

    // For each instance group
    for(InstanceGroup &group : groupList)
    {
      // Verify size of list
      if(group.data.size() == 0)
      {
        continue;
      }
      if(group.data.size() >= MAX_INSTANCES)
      {
      }

      UseShader(group.shader);
      UseCamera(group.shader, false);

      // Bind unique textures
      static int bound[TEXTURE_CHUNK];
      for(InstanceData &data : group.data)
      {
        // Get the index that the texture will be put in
        int textureIndex = data.texture % TEXTURE_CHUNK;
        // Check if that texture is already bound
        // This won't work if the fragment shader unbinds textures between frames?
        if(data.texture != bound[textureIndex])
        {
          glActiveTexture(GL_TEXTURE0 + textureIndex);
          glBindTexture(GL_TEXTURE_2D, data.texture);
        }
        // Store the texture index in the instance data
        data.texture = textureIndex;
      }

      // Send the new chunk of data to the VBO
      unsigned int numElements = group.data.size();
      glBindBuffer(GL_ARRAY_BUFFER, instanceDataVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * numElements, &(group.data[0]));

      // Call draw with this chunk of data
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numElements);

      // Clear the vector so it can be refilled
      //group.data.clear();
    }
  }

  // The reason this is here (we believe) is to synchroize with OpenGL
  // and to not write to a buffer too early and cause a massive stutter.
  // Need to sleep here because DrawGraphicsScreenInstaceList is called
  // immediately after. This writes to the same VBO as this function so
  // it's cause a delay. This waits for some amount of time and hopefully
  // we're done writing to the VBO by the time the other function is
  // called.
  //std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // glFinish seems to be a better solution
  glFinish();
}

#if 0
void DrawGraphicsScreenInstanceList()
{
  // For each layer
  for(InstanceGroupList &groupList : instanceScreenGroups)
  {
    // For each instance group
    for(InstanceGroup &group : groupList)
    {
      // Verify size of list
      if(group.data.size() == 0)
      {
        continue;
      }
      if(group.data.size() >= MAX_INSTANCES)
      {
      }

      UseShader(group.shader);
      UseCamera(group.shader, false);

      // Bind unique textures
      static int bound[TEXTURE_CHUNK];
      for(InstanceData &data : group.data)
      {
        // Get the index that the texture will be put in
        int textureIndex = data.texture % TEXTURE_CHUNK;
        // Check if that texture is already bound
        // This won't work if the fragment shader unbinds textures between frames?
        if(data.texture != bound[textureIndex])
        {
          glActiveTexture(GL_TEXTURE0 + textureIndex);
          glBindTexture(GL_TEXTURE_2D, data.texture);
        }
        // Store the texture index in the instance data
        data.texture = textureIndex;
      }

      // Send the new chunk of data to the VBO
      unsigned int numElements = group.data.size();
      glBindBuffer(GL_ARRAY_BUFFER, instanceDataVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * numElements, &(group.data[0]));

      // Call draw with this chunk of data
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numElements);

      // Clear the vector so it can be refilled
      group.data.clear();
    }
  }
}
#endif

//------------------------------------------------------------------------------
// Public General Functions:
//------------------------------------------------------------------------------

// Adds an item to the working vector of sprites
SpriteHandle AddSprite(v2 pos, v2 scale, float rotation, Color color, int layer, unsigned int texture,
                       v2 bottomLeftUV, v2 topRightUV, int shader)
{
  if(texture == 0)
  {
    texture = defaultTexture;
  }

  InstanceData instance =
  {
    pos,
    scale,
    rotation,
    color,
    bottomLeftUV,
    topRightUV,
    texture
  };

  InstanceGroupList &groupList = instanceGroups[layer];

  unsigned int textureChunk = texture / TEXTURE_CHUNK;

  if(textureChunk >= groupList.size())
  {
    groupList.resize(textureChunk + 1);
  }
  groupList[textureChunk].data.push_back(instance);

  // Set the shader for the instance
  if(shader == 0)
  {
    groupList[textureChunk].shader = instanceShaderProgram;
  }
  else
  {
    groupList[textureChunk].shader = shader;
  }

  // Get data for handle
  unsigned int index = groupList[textureChunk].data.size() - 1;
  InstanceGroup *group = &groupList[textureChunk];

  // Add handle to back of the list
  SpriteHandleData *handle = AddHandleToEndOfGroup(index, group);

  // Return a handle to the end of the list
  return handle;
}

InstanceData *GetSpriteData(SpriteHandle handle)
{
  return &handle->group->data[handle->index];
}

void RemoveSprite(SpriteHandle *toRemovePtr)
{
  SpriteHandle &toRemove = *toRemovePtr;
  std::vector<InstanceData> &groupData = toRemove->group->data;
  InstanceData &dataToOverwrite = groupData[toRemove->index];
  InstanceData &endData = groupData[toRemove->group->tail->index];

  // Copy vector data
  dataToOverwrite = endData;
  groupData.pop_back();

  {
    SpriteHandle &toRemove = *toRemovePtr;
    SpriteHandle &tail = toRemove->group->tail;

    // Check 3 cases
    if(toRemove == tail)
    {
      // Want to remove the tail
      tail = tail->prev;
      if(tail)
      {
        tail->next = 0;
      }

      delete toRemove;
      toRemove = 0;
    }
    else if(toRemove == tail->prev)
    {
      // Want to remove one before the tail
      if(toRemove->prev)
      {
        toRemove->prev->next = tail;
      }
      
      tail->prev = toRemove->prev;

      tail->index = toRemove->index;
      delete toRemove;
      toRemove = 0;
    }
    else
    {
      // Want to remove from the middle of the list
      SpriteHandle target = tail;

      // Move the tail pointer back one
      // Don't have to worry about the tail becoming null here
      tail = tail->prev;
      tail->next = 0;

      // Insert into the middle
      if(toRemove->prev)
      {
        toRemove->prev->next = target;
      }

      // toRemove should have a next because it's not at the end
      toRemove->next->prev = target;

      target->prev = toRemove->prev;
      target->next = toRemove->next;

      target->index = toRemove->index;

      delete toRemove;
      toRemove = 0;
    }
  }
}

#if 0
void AddScreenSprite(v2 pos, v2 scale, float rotation, Color color, int layer, unsigned int texture,
                     v2 bottomLeftUV, v2 topRightUV, int shader)
{
  InstanceData instance =
  {
    pos,
    scale,
    rotation,
    color,
    bottomLeftUV,
    topRightUV,
    texture
  };

  if(layer < 0)
  {
  }
  if((unsigned)layer >= instanceScreenGroups.size())
  {
    instanceScreenGroups.resize(layer + 1);
  }
  InstanceGroupList &groupList = instanceScreenGroups[layer];

  unsigned int textureChunk = texture / TEXTURE_CHUNK;

  if(textureChunk >= groupList.size())
  {
    groupList.resize(textureChunk + 1);
  }
  groupList[textureChunk].data.push_back(instance);

  if(shader == 0)
  {
    groupList[textureChunk].shader = instanceShaderProgram;
  }
  else
  {
    groupList[textureChunk].shader = shader;
  }
}
#endif

// Initialize glfw and glew systems. Create the program window
void InitGraphics()
{
  // initialize our openGL helper
  glfwInit();

  // these two denote the version of glfw we are using at least ( using 4.3 but 3.3 is good enough for this tutorial)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  // This says we are not planning on using any backwards compatibility
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  //Ccreate the game window
  CreateWindow();

  // Initialize glew
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
  }
  
  // Make shader
  instanceShaderProgram = MakeShaderProgram("Shaders/InstanceVertexShader.vs", "Shaders/InstanceFragShader.fs");

  GenerateBufferObjects();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //glEnable(GL_DEPTH_TEST);
  //glDepthFunc(GL_LESS); 

  // Set the callback function for the window resizing
  glfwSetFramebufferSizeCallback(window, WindowResizeCallbackFunction);
  
  // collect information on primary monitor
  glfwSetWindowPos(window, 100, 100);

  glfwSwapInterval(0);

  unsigned int texel[1];
  texel[0] = 0xFFFFFFFF;
  glGenTextures(1, &defaultTexture);
  glBindTexture(GL_TEXTURE_2D, defaultTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, texel);

  //glfwSetDropCallback(window, &DropCallBack);
}

void BeginGraphicsFrame()
{
  // flushes the color bit (one of three options)
  glClear(GL_COLOR_BUFFER_BIT /*| GL_DEPTH_BUFFER_BIT*/);
}

// swap the frame buffers
void EndGraphicsFrame()
{
  glfwSwapBuffers(window);
  glfwPollEvents();
}

bool WindowExists()
{
  if(glfwWindowShouldClose(window))
  {
    return false;
  }

  return true;
}

// Shutdown glfw and free any resources used by the system
void ShutdownGraphics()
{
  // Frees allocated resources and exits the program cleanly
  glfwTerminate();
}

// Create a shader program with the specified files
unsigned int MakeShaderProgram(const char *vertexFile, const char *fragFile)
{
  int  success;
  char infoLog[512];

  // Open and parse the files given
  char *vertexShaderSource = ReadFile(vertexFile);
  char *fragShaderSource = ReadFile(fragFile);

  if(!vertexShaderSource)
  {
    printf("Could not find vertex shader source file %s\n", vertexFile);
    return 0;
  }

  if(!fragShaderSource)
  {
    printf("Could not find fragment shader source file %s\n", fragFile);
    return 0;
  }

  // Create a new vertex shader and call createshader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

  // Add source code to object and compile
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // Check errors
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
  }

  // Create a fragment shader in a similar way
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragShaderSource, NULL);
  glCompileShader(fragmentShader);

  // Check errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
  }

  // create a shader program to link the newly created shaders to
  GLuint newProgram = glCreateProgram();

  // attach the shaders made to the shader program
  glAttachShader(newProgram, vertexShader);
  glAttachShader(newProgram, fragmentShader);
  glLinkProgram(newProgram);

  // delete the shaders after linkage because they are unnecessary
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  delete [] vertexShaderSource;
  delete [] fragShaderSource;

  return newProgram;
}

// These functions convert a QUANTITY of pixels to a measurement in device coordinates,
// NOT a position. Things shouldn't have to worry about position in pixels ever.
// 
// If a screen is 1920 pixels wide and 1920 pixels gets converted to device coordinates, this will
// correspond to 2.0f in device coordinates. This is correct for scaling but NOT position.
float PixelsToHorizontalDeviceCoordinates(int pixels)
{
  return (((float)pixels / viewportDimensions.x) * 2.0f);
}

float PixelsToVerticalDeviceCoordinates(int pixels)
{
  return (((float)pixels / viewportDimensions.y) * 2.0f);
}

int HorizontalDeviceCoordinatesToPixels(float horizontalDeviceCoordinate)
{
  return (int)((viewportDimensions.x / 2.0f) * horizontalDeviceCoordinate);
}

int VerticalDeviceCoordinatesToPixels(float verticalDeviceCoordinate)
{
  return (int)((viewportDimensions.y / 2.0f) * verticalDeviceCoordinate);
}

// This conveniently avoids calling two of the above functions
v2 PixelsToDeviceCoords(v2 pixelCoords)
{
  v2 deviceCoords;
  deviceCoords.x = PixelsToHorizontalDeviceCoordinates((int)pixelCoords.x);
  deviceCoords.y = PixelsToVerticalDeviceCoordinates((int)pixelCoords.y);
  return deviceCoords;
}

v2 DeviceToPixelCoords(v2 &deviceCoords)
{
  v2 pixelCoords;
  pixelCoords.x = (float)HorizontalDeviceCoordinatesToPixels(deviceCoords.x);
  pixelCoords.y = (float)VerticalDeviceCoordinatesToPixels(deviceCoords.y);
  return pixelCoords;
}

