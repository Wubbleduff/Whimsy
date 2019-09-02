#include "DrawCollisions.h"
#include "Vector2D.h"
#include "Graphics.h"
#include "Texture.h"
#include "Random.h"
#include "Input.h"

#include "profiling.h"

#include "imgui.h"

#include <vector>
#include <array>

constexpr float Abs(float a) { return (a < 0.0f) ? -a : a; }
constexpr float Min(float a, float b) { return (a < b) ? a : b; }
constexpr float Max(float a, float b) { return (a > b) ? a : b; }
constexpr float Clamp(float a, float minValue, float maxValue) { return Min(Max(a, minValue), maxValue); }

struct Circle
{
  float radius;
};

// Object origin is the center of the rect
struct Rect
{
  v2 scale;
};

struct Collider
{
  enum Type
  {
    CIRCLE,
    RECT,

  } type;

  union
  {
    Circle circle;
    Rect rect;
  };

  //std::vector<Collider *> enteredColliding = {};
  //std::vector<Collider *> currentlyColliding = {};

  Collider() {}
};

struct Entity
{
  v2 position;
  float invMass = 1.0f;
  v2 velocity = v2();
  bool stuck = false;

  Collider collider;

  Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
  SpriteHandle sprite;
};

struct CollisionInfo
{
  Entity *a;
  Entity *b;

  bool collided;
  v2 aNormal;
  v2 bNormal;
  float depth;

};

struct Cell
{
  std::vector<unsigned> entityList;
  SpriteHandle sprite;
};




static std::vector<Entity> entities;

const unsigned int GRID_WIDTH = 64;
typedef std::array<Cell, GRID_WIDTH * GRID_WIDTH> CellList;

const float CELL_WIDTH = 1.0f;
const Color CELL_COLOR = Color(0.5f, 0.5f, 0.5f, 0.25f);
static CellList cellGrid;






static bool PointOnEntity(v2 point, Entity *entity)
{
  if(entity->collider.type == Collider::CIRCLE)
  {
    v2 diff = entity->position - point;
    if(diff.Length() < entity->collider.circle.radius)
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
  if(circleA.radius + circleB.radius > diff.Length())
  {
    v2 normal = diff.Unit();

    info->a = a;
    info->b = b;
    info->aNormal = normal;
    info->bNormal = -normal;
    info->depth = (circleA.radius + circleB.radius) - diff.Length();
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

  v2 test = v2(Clamp(circlePos.x, l, r), Clamp(circlePos.y, b, t));
  v2 diff = circlePos - test;

  if(diff.Length() < radius)
  {
    v2 normal = diff.Unit();

    info->a = aEntity;
    info->b = bEntity;
    info->aNormal = normal;
    info->bNormal = -normal;
    info->depth = radius - diff.Length();
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
    float minDiff = Max(Max(Max(lDiff, rDiff), tDiff), bDiff); // max because they're all negative
    if(minDiff == lDiff) normal = v2(-1.0f, 0.0f);
    if(minDiff == rDiff) normal = v2(1.0f, 0.0f);
    if(minDiff == tDiff) normal = v2(0.0f, -1.0f);
    if(minDiff == bDiff) normal = v2(0.0f, 1.0f);

    info->aNormal = -normal;
    info->bNormal = normal;
    info->depth = Abs(minDiff);
    info->collided = true;
    return true;
  }
  else
  {
    info->collided = false;
    return false;
  }
}

static void WorldToGrid(v2 world, int *row, int *column)
{
  v2 gridPos = world;
  gridPos /= CELL_WIDTH;
  gridPos.y *= -1.0f;
  gridPos.x += GRID_WIDTH / 2.0f;
  gridPos.y += GRID_WIDTH / 2.0f;

  *column = (int)gridPos.x;
  *row = (int)gridPos.y;
}

static void GetCellsToCheckIndices(Entity *entity, unsigned *startRow, unsigned *startColumn, unsigned *numRows, unsigned *numColumns)
{
  v2 worldTopLeft;
  v2 worldBottomRight;

  v2 worldPos = entity->position;
  Collider *c = &entity->collider;
  switch(entity->collider.type)
  {
    case Collider::CIRCLE:
    {
      worldTopLeft = worldPos + v2(-c->circle.radius, c->circle.radius);
      worldBottomRight = worldPos + v2(c->circle.radius, -c->circle.radius);
    } break;

    case Collider::RECT:
    {
      worldTopLeft = worldPos + v2(-c->rect.scale.x / 2.0f, c->rect.scale.y / 2.0f);
      worldBottomRight = worldPos + v2(c->rect.scale.x / 2.0f, -c->rect.scale.y / 2.0f);
    } break;
  }

  int leftColumn, rightColumn, topRow, bottomRow;
  WorldToGrid(worldTopLeft, &topRow, &leftColumn);
  WorldToGrid(worldBottomRight, &bottomRow, &rightColumn);

  leftColumn = Clamp(leftColumn, 0, GRID_WIDTH - 1);
  rightColumn = Clamp(rightColumn, 0, GRID_WIDTH - 1);
  topRow = Clamp(topRow, 0, GRID_WIDTH - 1);
  bottomRow = Clamp(bottomRow, 0, GRID_WIDTH - 1);

  *startRow = topRow;
  *startColumn = leftColumn;
  *numRows = bottomRow - topRow;
  *numColumns = rightColumn - leftColumn;
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
  unsigned int circleTexture = GetTextureID("DefaultCircle.png");

  for(int i = 0; i < 100; i++)
  {

    v2 position = v2(RandomFloat(-30.0f, 30.0f), RandomFloat(0.0f, 20.0f));
    //float radius = RandomFloat(0.14f, 0.34f);
    float radius = RandomFloat(2.0f, 2.5f);
    //float radius = 3.0f;// * (i + 1);
    //v2 position = v2(0.0f, i * (radius + 0.5f) * 2.0f - 10.0f);
    Collider c;

#if 1
    unsigned int tex = circleTexture;
    c.type = Collider::CIRCLE;
    c.circle.radius = radius;
#else
    unsigned int tex = 0;
    c.type = Collider::RECT;
    c.rect.scale = v2(radius * 2.0f, radius * 2.0f);
#endif

    Color color = Color(0.6f, 0.0f, 0.8f, 1.0f);

    Entity entity;
    entity.position = position;
    entity.collider = c;
    entity.invMass = 1.0f / (PI * radius * radius);
    entity.color = color;
    entity.sprite = AddSprite(position, v2(radius * 2.0f, radius * 2.0f), 0.0f, color, 1, tex);
    entities.push_back(entity);
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
  entity.sprite = AddSprite(position, scale, 0.0f, color, 1, 0);
  entities.push_back(entity);



  position = v2(-45.0f, 0.0f);
  scale = v2(10.0f, 40.0f);

  entity.position = position;
  collider.rect.scale = scale;
  entity.collider = collider;
  entity.sprite = AddSprite(position, scale, 0.0f, color, 1, 0);
  entities.push_back(entity);

  position = v2(45.0f, 0.0f);

  entity.position = position;
  collider.rect.scale = scale;
  entity.collider = collider;
  entity.sprite = AddSprite(position, scale, 0.0f, color, 1, 0);
  entities.push_back(entity);
#endif



  float startX = -(GRID_WIDTH / 2.0f - 1.0f) * CELL_WIDTH - (CELL_WIDTH / 2.0f);
  float startY =  (GRID_WIDTH / 2.0f - 1.0f) * CELL_WIDTH + (CELL_WIDTH / 2.0f);
  v2 topLeft = v2(startX, startY);
  for(unsigned row = 0; row < GRID_WIDTH; row++)
  {
    for(unsigned column = 0; column < GRID_WIDTH; column++)
    {
      v2 pos = topLeft;
      pos.x += column * CELL_WIDTH;
      pos.y -= row * CELL_WIDTH;
      cellGrid[row * GRID_WIDTH + column].sprite = AddSprite(pos, v2(CELL_WIDTH, CELL_WIDTH) * 0.99f, 0.0f, Color(1.0f, 1.0f, 0.0f, 0.3f), 0, 0);
    }
  }
}

void UpdatePhysicsTest()
{
  static v2 lastMousePos = v2();
  v2 mousePos = MouseWorldPosition();
  v2 mouseDelta = mousePos - lastMousePos;

  static bool prevLeftMouseState = false;
  bool leftMouseToggledDown = false;
  bool leftMouseToggledUp = false;
  if(ButtonDown(0) && !prevLeftMouseState) leftMouseToggledDown = true;
  if(!ButtonDown(0) && prevLeftMouseState) leftMouseToggledUp = true;



  time_block("Moving entities");
  static Entity *clickedEntity = 0;
  for(unsigned i = 0; i < entities.size(); i++)
  {
    float mass = 0.0f;
    if(entities[i].invMass != 0.0f) mass = 1.0f / entities[i].invMass;
    if(!entities[i].stuck) entities[i].velocity.y -= 0.00098f * mass;

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

    // Spatial partitioning
    time_block("Clearing");
    for(unsigned i = 0; i < cellGrid.size(); i++) cellGrid[i].entityList.clear();
    end_time_block();

    time_block("Partitioning");
    for(unsigned i = 0; i < entities.size(); i++)
    {
      v2 worldPos = entities[i].position;
      int gridColumn;
      int gridRow;
      WorldToGrid(worldPos, &gridRow, &gridColumn);

      gridColumn = Clamp(gridColumn, 0, GRID_WIDTH - 1);
      gridRow = Clamp(gridRow, 0, GRID_WIDTH - 1);

      cellGrid[gridRow * GRID_WIDTH + gridColumn].entityList.push_back(i);
    }
    end_time_block();

#if 1
    for(unsigned i = 0; i < cellGrid.size(); i++)
    {
      InstanceData *data = GetSpriteData(cellGrid[i].sprite);
      if(cellGrid[i].entityList.size() != 0)
      {
        data->color = CELL_COLOR;
        data->color.a += 0.5f;
      }
      else
      {
        data->color = CELL_COLOR;
      }
    }
#endif



    time_block("Checking collisions");
    static std::vector<CollisionInfo> collisions;

    static int numChecks = 0;
#if 0
    // Go through all cells
    for(unsigned cell_index = 0; cell_index < cellGrid.size(); cell_index++)
    {
      time_block("Go through all cells");
      Cell cell = cellGrid[cell_index];

      // Go through all entities in the current cell
      for(unsigned i = 0; i < cell.entityList.size(); i++)
      {
        Entity *a = &entities[cell.entityList[i]];

        // Get the rect of cells to check against this entity
        unsigned startRow;
        unsigned startColumn;
        unsigned numRows;
        unsigned numColumns;
        GetCellsToCheckIndices(a, &startRow, &startColumn, &numRows, &numColumns);

        // Go through all other cells
        for(unsigned row = startRow; row <= startRow + numRows; row++)
        {
          for(unsigned column = startColumn; column <= startColumn + numColumns; column++)
          {
            Cell otherCell = cellGrid[row * GRID_WIDTH + column];

            // Go through each entity in the other current cell
            for(unsigned j = 0; j < otherCell.entityList.size(); j++)
            {
              numChecks++;

              Entity *b = &entities[otherCell.entityList[j]];

              if(a == b) continue;
              CollideEntities(a, b, &collisions);
            }
          }
        }
      }
      end_time_block();
    }
#else
    for(unsigned i = 0; i < entities.size(); i++)
    {
      Entity &a = entities[i];


      for(unsigned j = i + 1; j < entities.size(); j++)
      {
        numChecks++;
        Entity &b = entities[j];

        CollisionInfo info = {};

        if(a.collider.type == Collider::CIRCLE)
        {
          // A is circle
          //
          switch(b.collider.type)
          {
            case Collider::CIRCLE:
            {
              if(CircleCircleCollision(&a, &b, &info))
              {
                collisions.push_back(info);
              }
            } break;

            case Collider::RECT:
            {
              if(CircleRectCollision(&a, &b, &info))
              {
                collisions.push_back(info);
              }
            } break;
          }
        }
        else if(a.collider.type == Collider::RECT)
        {
          // A is rect
          //
          switch(b.collider.type)
          {
            case Collider::CIRCLE:
            {
              if(CircleRectCollision(&b, &a, &info))
              {
                collisions.push_back(info);
              }
            } break;

            case Collider::RECT:
            {
              if(RectRectCollision(&a, &b, &info))
              {
                collisions.push_back(info);
              }
            } break;
          }
        }

#if 0
        if(info.collided)
        {
          auto it = std::find(a.collider.currentlyColliding.begin(), a.collider.currentlyColliding.end(), &b.collider);
          if(it == a.collider.currentlyColliding.end())
          {
            a.collider.enteredColliding.push_back(&b.collider);
            a.collider.currentlyColliding.push_back(&b.collider);

            b.collider.enteredColliding.push_back(&a.collider);
            b.collider.currentlyColliding.push_back(&a.collider);
          }
        }
        else
        {
          auto it = std::find(a.collider.currentlyColliding.begin(), a.collider.currentlyColliding.end(), &b.collider);
          if(it != a.collider.currentlyColliding.end())
          {
            a.collider.currentlyColliding.erase(it);
            b.collider.currentlyColliding.erase(std::find(b.collider.currentlyColliding.begin(),
                                                b.collider.currentlyColliding.end(), &a.collider));
          }
        }
#endif
      }
    }
#endif
    ImGui::InputInt("Checks", &numChecks);
    numChecks = 0;
    end_time_block();

    time_block("Resolving collisions");

    static float percent = 0.5f;
    static float slop = 0.1f;

    ImGui::DragFloat("resolve percent", &percent, 0.01f);
    ImGui::DragFloat("pen slop", &slop, 0.01f);

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
        v2 n = info.bNormal;
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


        v2 correction = (Max(info.depth - slop, 0.0f) / (a->invMass + b->invMass)) * percent * n; 
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
#if 1
    if(entities[i].position.x < -xBound) entities[i].velocity.x = Abs(entities[i].velocity.x);
    if(entities[i].position.x >  xBound) entities[i].velocity.x = -Abs(entities[i].velocity.x);
    if(entities[i].position.y < -yBound) entities[i].velocity.y = Abs(entities[i].velocity.y);
    if(entities[i].position.y >  yBound) entities[i].velocity.y = -Abs(entities[i].velocity.y);
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
    InstanceData *data = GetSpriteData(entities[i].sprite);

    data->position = entities[i].position;

    switch(entities[i].collider.type)
    {
      case Collider::CIRCLE: data->scale = v2(entities[i].collider.circle.radius * 2.0f, entities[i].collider.circle.radius * 2.0f); break;
      case Collider::RECT: data->scale = entities[i].collider.rect.scale; break;
    }

    data->color = entities[i].color;
  }
  end_time_block();

  lastMousePos = mousePos;
  prevLeftMouseState = ButtonDown(0);
}

