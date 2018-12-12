#include "DrawCollisions.h"
#include "Vector2D.h"
#include "Graphics.h"
#include "Input.h"
#include "Texture.h"
#include "Random.h"

#include <vector>

struct Line
{
  v2 start;
  v2 end;

  SpriteHandle sprite;
};

static SpriteHandle circleSprite;
static v2 circleVel;

static std::vector<Line> lines;

v2 ReflectVector(v2 v, v2 n)
{
  if((v.x == 0.0f && v.y == 0.0f) ||
    (n.x == 0.0f && n.y == 0.0f)
    )
  {
    return v;
  }

  v2 normal = n.FindNormal();

  float dist = dot(v, normal);

  return v - (normal * dist * 2.0f);
}

bool LineLineCollision(v2 p1, v2 p2, v2 q1, v2 q2)
{
  v2 normalP; // A vector that is normal to line segment p
  v2 p1Toq1; // A vector from point p1 to point q1
  v2 p2Toq2; // A vector from point p2 to point q2
  float dotp1Toq1; // The dot product of the normal and vector p1 to q1
  float dotp2Toq2; // The dot product of the normal and vector p2 to q2

                   // Get a vector along line segment p
  normalP.x = p2.x - p1.x;
  normalP.y = p2.y - p1.y;
  // Get a normal vector to line segment p
  normalP = normalP.FindNormal();

  // Get vectors from p to q
  p1Toq1.x = q1.x - p1.x;
  p1Toq1.y = q1.y - p1.y;

  p2Toq2.x = q2.x - p2.x;
  p2Toq2.y = q2.y - p2.y;

  // Find the dot product of the normal with each vector
  dotp1Toq1 = dot(normalP, p1Toq1);
  dotp2Toq2 = dot(normalP, p2Toq2);

  // Check p's normal with its vectors
  if(dotp1Toq1 == 0.0f && dotp2Toq2 == 0.0f)
  {
    // Check if points overlap
    if((p1.x <= q2.x &&
      q1.x <= p2.x &&
      p1.y <= q2.y &&
      q1.y <= p2.y
      )
      ||
      (q2.x <= p1.x &&
        p2.x <= q1.x &&
        q2.y <= p1.y &&
        p2.y <= q1.y
        )
      )
    {
      // The dots overlap, return true
      return true;
    }
    else
    {
      // Otherwise, return false
      return false;
    }
  }
  else if((dotp1Toq1 < 0.0f && dotp2Toq2 < 0.0f) ||
    (dotp1Toq1 > 0.0f && dotp2Toq2 > 0.0f)
    )
  {
    // If they're the same sign, they're not intersecting, return false
    return false;
  }
  else
  {

    /*
    * Otherwise, check the other line to see if p's points are on the same
    * side of q 
    */
    v2 normalQ; // A vector that is normal to line segment q

                // Get a vector along line segment q
    normalQ.x = q2.x - q1.x;
    normalQ.y = q2.y - q1.y;
    // Get a normal vector to line segment q
    normalQ = normalQ.FindNormal();

    // Find the dot products of the normal with the vectors
    dotp1Toq1 = dot(normalQ, p1Toq1);
    dotp2Toq2 = dot(normalQ, p2Toq2);

    // Compare the signs of the dot products of q's normal with the vectors
    if((dotp1Toq1 < 0.0f && dotp2Toq2 < 0.0f) ||
      (dotp1Toq1 > 0.0f && dotp2Toq2 > 0.0f)
      )
    {
      // The signs are the same, return false
      return false;
    }
    else
    {
      // The signs are different, return true
      return true;
    }
  }
}

static bool CircleLineCollision(v2 circlePos, v2 circleVel, v2 lineStart, v2 lineEnd, v2 *resolvePos, v2 *resolveVel)
{
  if(LineLineCollision(circlePos, circlePos + circleVel, lineStart, lineEnd))
  {
    v2 lineNormal = (lineEnd - lineStart).FindNormal();
    lineNormal = lineNormal.Unit();
    float v0Dot = dot(circlePos, lineNormal);
    float v1Dot = dot(circlePos + circleVel, lineNormal);
    float totalDist = fabs(v1Dot - v0Dot);
    float intersectDist = fabs(dot(lineStart, lineNormal) - v0Dot);
    float ratio = intersectDist / totalDist;

    v2 toLine = circleVel * ratio;
    *resolvePos = circlePos + toLine;
    //collideInfo.normal = lineNormal;
    *resolveVel -= toLine;
    *resolveVel = ReflectVector(circleVel, lineNormal);
    *resolveVel *= -1.0f;

    if(dot(*resolveVel, lineNormal) < 0.0f)
    {
      *resolveVel += lineNormal * intersectDist * 0.6f;
    }
    else
    {
      *resolveVel -= lineNormal * intersectDist * 0.6f;
    }

    return true;
  }

  return false;
}

static void UpdateCollisions()
{
  InstanceData *data = GetSpriteData(circleSprite);

  // Only use circle for now
  v2 posCopy = data->position;
  v2 velCopy = circleVel;

  if(velCopy.LengthSquared() == 0.0f)
  {
    return;
  }

  // Detect closest collision against all objects and update the posCopy and velCopy
  // Do again with new posCopy and velCopy and loop until there are no more collisions
  
  bool collided = false;
  int maxChecks = 50;
  int checks = 0;
  do
  {
    collided = false;
    float closestDist = 0.0f;
    v2 closestPos;
    v2 closestVel;

    // Find closest collision
    for(Line &line : lines)
    {
      // Compare distance to current line collision if collided
      v2 resolvePos;
      v2 resolveVel;

      if(CircleLineCollision(posCopy, velCopy, line.start, line.end, &resolvePos, &resolveVel))
      {
        collided = true;

        float dist = (resolvePos - posCopy).LengthSquared();
        if(dist < closestDist || closestDist == 0.0f)
        {
          closestDist = dist;
          closestPos = resolvePos;
          closestVel = resolveVel;
        }
      }
    }

    // Update position and velocity based on closest collision
    if(collided)
    {
#define RESOLVE_EPSILON 0.001f
      posCopy = closestPos - (closestPos - posCopy).Unit() * RESOLVE_EPSILON;
      velCopy = closestVel;
    }

    checks++;

    // Loop again
  } while(collided && checks < maxChecks);

  // Update the real position and velocity

  data->position = posCopy;
  circleVel = velCopy;
}

static void UpdatePhysics()
{
  circleVel -= v2(0.0f, 0.003f);
  circleVel -= circleVel * 0.001f;

  if(fabs(circleVel.x) < 0.0025f)
  {
    circleVel.x = 0.0f;
  }
  if(fabs(circleVel.y) < 0.0025f)
  {
    circleVel.y = 0.0f;
  }

  // At this point, velocity should not be changed by any outside
  // code until it is integrated into position
  // Updating collisions will change the velocity

  UpdateCollisions();


  InstanceData *data = GetSpriteData(circleSprite);
  data->position += circleVel;
}

static void LineDrawing()
{
  static v2 lastPoint = v2();


  v2 mousePos = MouseWorldPosition();
  v2 difference = lastPoint - mousePos;

  float minDist = 0.5f;
  //float minDist = 5.0f;
  if(ButtonDown(0))
  {
    if(difference.Length() > minDist)
    {
      Line line;
      line.sprite = AddLine(lastPoint, mousePos, 0.1f, Color(0.0f, 0.0f, 1.0f, 1.0f), 0);
      
      line.start = lastPoint;
      line.end = mousePos;
      lines.push_back(line);
    }
  }

  if(difference.Length() > minDist)
  {
    lastPoint = mousePos;
  }
}

static void InitLines()
{
  float radius = 25.0f;
  for(int i = 0; i < 6; i++)
  {
    float angle0 = i / 3.0f * PI + (PI / 2.0f);
    float angle1 = (i + 1) / 3.0f * PI + (PI / 2.0f);

    Line line;
    line.start = v2(cos(angle0), sin(angle0)) * radius;
    line.end = v2(cos(angle1), sin(angle1)) * radius;
    line.sprite = AddLine(line.start, line.end, 0.1f, Color(0.0f, 0.0f, 1.0f, 1.0f), 0);
    lines.push_back(line);
  }
}

void InitDrawCollisions()
{
  unsigned int circleTexture = GetTextureID("DefaultCircle.png");

  circleSprite = AddSprite(v2(0.0f, 10.0f), v2(1.0f, 1.0f), 0.0f, Color(0.4f, 0.0f, 0.5f, 1.0f), 1, circleTexture);

  InitLines();
}

void UpdateDrawCollisions()
{
  UpdatePhysics();

  LineDrawing();

  if(ButtonDown('R'))
  {
    for(Line &line : lines)
    {
      RemoveSprite(&line.sprite);
    }
    lines.clear();
    InstanceData *data = GetSpriteData(circleSprite);
    data->position = v2(0.0f, 10.0f);
    circleVel = v2();

    InitLines();
  }

  if(ButtonDown(32))
  {
    circleVel += v2(RandomFloat(-1.0f, 0.1f), RandomFloat(-1.0f, 0.1f));
    //circleVel.x = 10.0f;
  }
}
