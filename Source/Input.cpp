#include "Input.h"
#include "Graphics.h"
#include "../Libraries/glfw/Include/GLFW/glfw3.h"

bool ButtonDown(int key)
{
  if(key == 0 || key == 1)
  {
    return glfwGetMouseButton(GetEngineWindow(), key);
  }

  return glfwGetKey(GetEngineWindow(), key);
}

v2 MouseWorldPosition()
{
  double x, y;
  v2 mousePos;

  //Gather world position in pixels first
  glfwGetCursorPos(GetEngineWindow(), &x, &y);

  // Now it's pixels
  mousePos.x = float(x);
  mousePos.y = float(y);

  // now it's device coords
  mousePos = PixelsToDeviceCoords(mousePos);
  mousePos -= v2(1.0f, 1.0f);

  // now it's world coords
  mousePos = DeviceCoordinateToWorldPosition(mousePos);

  return mousePos;
}

