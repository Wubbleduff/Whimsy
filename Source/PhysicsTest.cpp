#include "my_math.h"
#include "renderer2D.h"
#include "Random.h"

#include "profiling.h"

#include "imgui.h"

#include <vector>
#include <array>

bool ButtonDown(unsigned button);
bool MouseDown(unsigned button);
v2 MouseWindowPosition();

struct Circle
{
  float radius;
};

// Object origin is the center of the rect
struct Rect
{
  v2 scale;
};

struct Triangle
{
  v3 p, q, r;
};

struct Collider
{
  enum Type
  {
    CIRCLE,
    RECT,
    TRIANGLE,

  } type;

  union
  {
    Circle circle;
    Rect rect;
    Triangle triangle;
  };

  //std::vector<Collider *> enteredColliding = {};
  //std::vector<Collider *> currentlyColliding = {};

  Collider() {}
};

struct Entity
{
  v2 position = v2();
  float invMass = 1.0f;
  v2 velocity = v2();
  bool stuck = false;

  Collider collider;

  Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);

  ModelHandle model;
};

struct CollisionInfo
{
  Entity *a;
  Entity *b;

  bool collided;
  v2 resolveNormal;
  float depth;
};




static std::vector<Entity> entities;




static bool PointOnEntity(v2 point, Entity *entity)
{
  if(entity->collider.type == Collider::CIRCLE)
  {
    v2 diff = entity->position - point;
    if(length(diff) < entity->collider.circle.radius)
    {
      return true;
    }
  }
  else if(entity->collider.type == Collider::RECT)
  {
    Rect &rect = entity->collider.rect;
    if(point.x < entity->position.x + rect.scale.x / 2.0f &&
       point.x > entity->position.x - rect.scale.x / 2.0f &&
       point.y < entity->position.y + rect.scale.y / 2.0f &&
       point.y > entity->position.y - rect.scale.y / 2.0f)
    {
      return true;
    }
  }

  return false;
}

static bool CircleCircleCollision(Entity *a, Entity *b, CollisionInfo *info)
{
  Circle &circleA = a->collider.circle;
  Circle &circleB = b->collider.circle;

  v2 diff = a->position - b->position;
  if(circleA.radius + circleB.radius > length(diff))
  {
    v2 normal = unit(diff);

    info->a = a;
    info->b = b;
    info->resolveNormal = -normal;
    info->depth = (circleA.radius + circleB.radius) - length(diff);
    info->collided = true;
    return true;
  }
  info->collided = false;
  return false;
}

static bool CircleRectCollision(Entity *aEntity, Entity *bEntity, CollisionInfo *info)
{
  Rect &rect = bEntity->collider.rect;
  v2 circlePos = aEntity->position;
  float radius = aEntity->collider.circle.radius;
  float l = bEntity->position.x - rect.scale.x / 2.0f;
  float r = bEntity->position.x + rect.scale.x / 2.0f;
  float t = bEntity->position.y + rect.scale.y / 2.0f;
  float b = bEntity->position.y - rect.scale.y / 2.0f;

  v2 test = v2(clamp(circlePos.x, l, r), clamp(circlePos.y, b, t));
  v2 diff = circlePos - test;

  if(length(diff) < radius)
  {
    v2 normal = unit(diff);

    info->a = aEntity;
    info->b = bEntity;
    info->resolveNormal = -normal;
    info->depth = radius - length(diff);
    info->collided = true;
    return true;
  }


  return false;
}

static bool RectRectCollision(Entity *a, Entity *b, CollisionInfo *info)
{
  Rect &aRect = a->collider.rect;
  Rect &bRect = b->collider.rect;
  float aL = a->position.x - aRect.scale.x / 2.0f;
  float aR = a->position.x + aRect.scale.x / 2.0f;
  float aT = a->position.y + aRect.scale.y / 2.0f;
  float aB = a->position.y - aRect.scale.y / 2.0f;
  float bL = b->position.x - bRect.scale.x / 2.0f;
  float bR = b->position.x + bRect.scale.x / 2.0f;
  float bT = b->position.y + bRect.scale.y / 2.0f;
  float bB = b->position.y - bRect.scale.y / 2.0f;

  // With respect to a
  float lDiff = aL - bR;
  float rDiff = bL - aR;
  float tDiff = aB - bT;
  float bDiff = bB - aT;

  if(lDiff < 0.0f && rDiff < 0.0f && tDiff < 0.0f && bDiff < 0.0f)
  {
    info->a = a;
    info->b = b;

    v2 normal = v2();
    float minDiff = max(max(max(lDiff, rDiff), tDiff), bDiff); // max because they're all negative
    if(minDiff == lDiff) normal = v2(-1.0f, 0.0f);
    if(minDiff == rDiff) normal = v2(1.0f, 0.0f);
    if(minDiff == tDiff) normal = v2(0.0f, -1.0f);
    if(minDiff == bDiff) normal = v2(0.0f, 1.0f);

    info->resolveNormal = normal;
    info->depth = absf(minDiff);
    info->collided = true;
    return true;
  }
  else
  {
    info->collided = false;
    return false;
  }
}


// NOTE:
// I'm not going to consider points exactly on lines OR collinear lines
// to be intersecting. The reason is is because it's more a complex and
// the "resolution depth" would just be 0 anyways...
static bool LineLineSegmentCollision(v2 a, v2 b, v2 p, v2 q, v2 *n, float *t)
{
  v2 ab = b - a;
  v2 pq = q - p;
  v2 abN = -find_normal(ab);
  v2 pqN = -find_normal(pq);
  v2 ap = p - a;
  v2 aq = q - a;
  v2 pb = b - p;

  float distPAB = dot(ap, abN);
  float distQAB = dot(aq, abN);
  float distAPQ = dot(-ap, pqN);
  float distBPQ = dot(pb, pqN);

  // Make sure they're crossing
  if(distPAB * distQAB > 0.0f) return false;
  if(distAPQ * distBPQ > 0.0f) return false;
  if(dot(ab, pqN) == 0.0f)     return false; // Collinear

  float ratio = absf(dot(ap, abN)) / absf(dot(pq, abN));
  if(ratio > 1.0f) ratio = absf(dot(aq, abN)) / absf(dot(ap, abN));

  *n = abN;
  *t = 1.0f - ratio;
  return true;
}

static bool TriangleTriangleCollision(Entity *a, Entity *b, CollisionInfo *info)
{
  for(int i = 0; i < 3; i++)
  {

    for(int j = 0; j < 3; j++)
    {
      v3 *aPoints = &(a->collider.triangle.p);
      v3 *bPoints = &(b->collider.triangle.p);

      v2 a = v2(aPoints[i].x, aPoints[i].y);
      v2 b = v2(aPoints[(i + 1) % 3].x, aPoints[(i + 1) % 3].y);

      v2 p = v2(bPoints[i].x, bPoints[i].y);
      v2 q = v2(bPoints[(i + 1) % 3].x, bPoints[(i + 1) % 3].y);

      v2 normal;
      float depth;
      if(LineLineSegmentCollision(a, b, p, q, &normal, &depth))
      {
        return true;
      }
    }
  }
}

static void CollideEntities(Entity *a, Entity *b, std::vector<CollisionInfo> *collisions)
{
  CollisionInfo info = {};

  if(a->collider.type == Collider::CIRCLE)
  {
    // A is circle
    //
    switch(b->collider.type)
    {
      case Collider::CIRCLE:
      {
        CircleCircleCollision(a, b, &info);
      } break;

      case Collider::RECT:
      {
        CircleRectCollision(a, b, &info);
      } break;
    }
  }
  else if(a->collider.type == Collider::RECT)
  {
    // A is rect
    //
    switch(b->collider.type)
    {
      case Collider::CIRCLE:
      {
        CircleRectCollision(b, a, &info);
      } break;

      case Collider::RECT:
      {
        RectRectCollision(a, b, &info);
      } break;
    }
  }

  if(info.collided) collisions->push_back(info);
}














void InitPhysicsTest()
{
  for(int i = 0; i < 1; i++)
  {
    v2 position = v2();
    Collider c;

    c.triangle.p = v3(-1.0f, -1.0f, 0.0f);
    c.triangle.q = v3( 1.0f, -1.0f, 0.0f);
    c.triangle.r = v3( 0.0f,  1.0f, 0.0f);

    Color color = Color(0.6f, 0.0f, 0.8f, 1.0f);

    Entity entity;
    entity.position = position;
    entity.collider = c;
    entity.invMass = 1.0f / 1.0f;//(PI * radius * radius);
    entity.color = color;

    unsigned indices[3] = {0, 1, 2};
    //entity.model = create_model(&(c.triangle.p), 3, indices, 3, color);


    //entities.push_back(entity);
  }

#if 1

  v2 position = v2(RandomFloat(-30.0f, 30.0f), RandomFloat(0.0f, 20.0f));
  v2 scale = v2(80.0f, 1.0f);
  Color color = Color(0.8f, 0.8f, 0.0f, 1.0f);

  Collider collider;
  collider.type = Collider::RECT;
  Entity entity;
  entity.stuck = true;
  entity.invMass = 0.0f;
  entity.color = color;

  position = v2(0.0f, -25.0f);
  scale = v2(90.0f, 10.0f); 

  entity.position = position;
  collider.rect.scale = scale;
  entity.collider = collider;
  entity.model = create_model(PRIMITIVE_QUAD, 0);
  {
    Model *temp_handle = get_temp_model_pointer(entity.model);
    temp_handle->scale = scale;
    temp_handle->blend_color = v4(color.r, color.b, color.g, color.a);
  }
  entities.push_back(entity);



  position = v2(-45.0f, 0.0f);
  scale = v2(10.0f, 40.0f);

  entity.position = position;
  collider.rect.scale = scale;
  entity.collider = collider;
  //entities.push_back(entity);

  position = v2(45.0f, 0.0f);

  entity.position = position;
  collider.rect.scale = scale;
  entity.collider = collider;
  //entities.push_back(entity);
#endif
}

void UpdatePhysicsTest()
{
  static v2 lastMousePos = v2();
  v2 mousePos = window_to_world_space(MouseWindowPosition());
  v2 mouseDelta = mousePos - lastMousePos;

  static bool prevLeftMouseState = false;
  bool leftMouseToggledDown = false;
  bool leftMouseToggledUp = false;
  if(MouseDown(0) && !prevLeftMouseState) leftMouseToggledDown = true;
  if(!MouseDown(0) && prevLeftMouseState) leftMouseToggledUp = true;



  time_block("Moving entities");
  static Entity *clickedEntity = 0;
  for(unsigned i = 0; i < entities.size(); i++)
  {
    float mass = 0.0f;
    if(entities[i].invMass != 0.0f) mass = 1.0f / entities[i].invMass;
    if(!entities[i].stuck) entities[i].velocity.y -= 0.0000098f * mass;

    //entities[i].color = Color(0.4f, 0.0f, 0.5f, 1.0f);

    if(leftMouseToggledDown)
    {
      if(PointOnEntity(mousePos, &entities[i]))
      {
        clickedEntity = &entities[i];
      }
    }


    //entities[i].collider.enteredColliding.clear();
  }
  end_time_block();


  if(clickedEntity)
  {
    clickedEntity->position += mouseDelta;
    //clickedEntity->velocity = v2();
    clickedEntity->velocity = mouseDelta;
    clickedEntity->stuck = true;

    if(leftMouseToggledUp)
    {
      //clickedEntity->velocity += mouseDelta;
      clickedEntity->stuck = false;
      clickedEntity = 0;
    }
  }


  static int numIts = 20;
  ImGui::InputInt("its", &numIts);
  for(unsigned simIteration = 0; simIteration < numIts; simIteration++)
  {
    static std::vector<CollisionInfo> collisions;
    time_block("Checking collisions");
    for(unsigned i = 0; i < entities.size(); i++)
    {
      Entity &a = entities[i];


      for(unsigned j = i + 1; j < entities.size(); j++)
      {
        Entity &b = entities[j];

        CollisionInfo info = {};
        CollideEntities(&a, &b, &collisions);
      }
    }
    end_time_block();




    time_block("Resolving collisions");

    static float percent = 0.5f;
    static float slop = 0.1f;

    //ImGui::DragFloat("resolve percent", &percent, 0.01f);
    //ImGui::DragFloat("pen slop", &slop, 0.01f);

    for(unsigned i = 0; i < collisions.size(); i++)
    {
      CollisionInfo &info = collisions[i];

      if(info.collided)
      {
        Entity *a = info.a;
        Entity *b = info.b;

        //a->color = Color(1.0f, 1.0f, 0.0f, 1.0f);
        //b->color = Color(1.0f, 1.0f, 0.0f, 1.0f);

        v2 velA = a->velocity;
        v2 velB = b->velocity;
        v2 n = info.resolveNormal;
        v2 rv = velB - velA;
        float mA = 0.0f;
        if(a->invMass != 0.0f) mA = 1.0f / a->invMass;

        float mB = 0.0f;
        if(b->invMass != 0.0f) mB = 1.0f / b->invMass;

        float velAlongNormal = dot(rv, n);
        if(velAlongNormal > 0.0f) continue;

        float e = 0.5f;

        float j = -(1.0f + e) * velAlongNormal;
        j /= a->invMass + b->invMass;

        v2 impulse = j * n;


        v2 correction = (max(info.depth - slop, 0.0f) / (a->invMass + b->invMass)) * percent * n; 
        if(!a->stuck)
        {
          a->velocity -= a->invMass * impulse;
          a->position -= a->invMass * correction;
        }
        if(!b->stuck)
        {
          b->velocity += b->invMass * impulse;
          b->position += b->invMass * correction;
        }
      }
    }
    collisions.clear();

  }
  end_time_block();



  time_block("Apply velocity");
  for(unsigned i = 0; i < entities.size(); i++)
  {
    float xBound = 40.0f;
    float yBound = 25.0f;
#if 0
    if(entities[i].position.x < -xBound) entities[i].velocity.x = absf(entities[i].velocity.x);
    if(entities[i].position.x >  xBound) entities[i].velocity.x = -absf(entities[i].velocity.x);
    if(entities[i].position.y < -yBound) entities[i].velocity.y = absf(entities[i].velocity.y);
    if(entities[i].position.y >  yBound) entities[i].velocity.y = -absf(entities[i].velocity.y);
#else
    if(entities[i].position.x < -xBound) entities[i].position.x =  xBound;
    if(entities[i].position.x >  xBound) entities[i].position.x = -xBound;
    if(entities[i].position.y < -yBound) entities[i].position.y =  yBound;
    if(entities[i].position.y >  yBound) entities[i].position.y = -xBound;
#endif

    if(!entities[i].stuck)
    {
      entities[i].position += entities[i].velocity;
    }

  }
  end_time_block();






  time_block("Copying graphics data");
  for(unsigned i = 0; i < entities.size(); i++)
  {
    Model *model = get_temp_model_pointer(entities[i].model);

    model->position = v3(entities[i].position, 0.0f);
  }
  end_time_block();

  lastMousePos = mousePos;
  prevLeftMouseState = MouseDown(0);

  static v2 a = v2(-1.0f,  1.0f);
  static v2 b = v2( 1.0f, -1.0f);
  static v2 p = v2(-1.0f, -1.0f);
  static v2 q = v2( 1.0f,  1.0f);
  ImGui::DragFloat2("a", &a.x);
  ImGui::DragFloat2("b", &b.x);
  ImGui::DragFloat2("p", &p.x);
  ImGui::DragFloat2("q", &q.x);
  float t;
  Color c = Color(1.0f, 0.0f, 0.0f, 1.0f);
  v2 n;
  if(LineLineSegmentCollision(a, b, p, q, &n, &t))
  {
    c = Color(1.0f, 1.0f, 0.0f, 1.0f);
    ImGui::InputFloat("t", &t);
  }
  immediate_line(v3(a, 0.0f), v3(b, 0.0f), 1.0f, c);
  immediate_line(v3(p, 0.0f), v3(q, 0.0f), 1.0f, c);
  //immediate_triangle(v3(a, 0.0f), v3(b, 0.0f), v3(q, 0.0f), Color(1, 0, 0, 1));
}

