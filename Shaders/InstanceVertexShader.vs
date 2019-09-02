#version 330 core

// location of vertices
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// The new instance stuff my mans
layout (location = 2) in vec2 offset;
layout (location = 3) in vec2 scale;
layout (location = 4) in float rotation;
layout (location = 5) in vec4 aColor;
layout (location = 6) in vec2 aBottomLeftUV;
layout (location = 7) in vec2 aTopRightUV;
layout (location = 8) in int aTexture;

// texture coords
smooth out vec2 texCoord;
smooth out vec4 color;
flat out int textureIndex;
out vec2 bottomLeftUV;
out vec2 topRightUV;

out vec2 center;
out float radius;

uniform mat4 cameraMatrix;

void main()
{
  // begin by calculating our matrices
  mat4 scaleMtx = mat4(scale.x, 0, 0, 0,
                    0, scale.y, 0, 0,
                    0, 0, 1, 0, 
                    0, 0, 0, 1
                   );

  mat4 rotMtx = mat4( cos(rotation), sin(rotation), 0, 0,
                  -sin(rotation), cos(rotation), 0, 0,
                   0, 0, 1, 0, 
                   0, 0, 0, 1
                 );

  mat4 transMtx = mat4(1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0, 
                    offset.x, offset.y, 0, 1
                   );

  // Multiply them
  mat4 transform = transMtx * rotMtx * scaleMtx;
  transform = cameraMatrix * transform;

  gl_Position = transform * vec4(aPos, 1.0f);


  texCoord = aTexCoord;
  color = aColor;
  textureIndex = aTexture;

  bottomLeftUV = aBottomLeftUV;
  topRightUV = aTopRightUV;

  vec4 pixelPos = cameraMatrix * vec4(offset, 0.0f, 1.0f);
  pixelPos.x = ((pixelPos.x + 1.0f) / 2.0f) * 1920;
  pixelPos.y = ((pixelPos.y + 1.0f) / 2.0f) * 1080;
  center = pixelPos.xy;
  vec4 radiusValues = vec4(scale, 0.0f, 1.0f);
  radiusValues = cameraMatrix * radiusValues;
  radius = radiusValues.x;
  radius = ((radius + 1.0f) / 2.0f) * 1920;
}
