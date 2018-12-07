#version 330 core
out vec4 fragColor;

smooth in vec4 color;
smooth in vec2 texCoord;
flat in int textureIndex;
in vec2 bottomLeftUV;
in vec2 topRightUV;

uniform sampler2D ourTexture[32];

void main()
{
  vec2 samplePos;
  samplePos.x = bottomLeftUV.x + texCoord.x*(topRightUV.x - bottomLeftUV.x);
  samplePos.y = bottomLeftUV.y + texCoord.y*(topRightUV.y - bottomLeftUV.y);

  // Evan Kau said that subscripting into an array of sampler2D's using
  // a variable breaks on some GPUs. This is to avoid that bug.
  switch(textureIndex)
  {
    case 0:
      fragColor = color * texture(ourTexture[0], samplePos);
      break;
    case 1:
      fragColor = color * texture(ourTexture[1], samplePos);
      break;
    case 2:
      fragColor = color * texture(ourTexture[2], samplePos);
      break;
    case 3:
      fragColor = color * texture(ourTexture[3], samplePos);
      break;
    case 4:
      fragColor = color * texture(ourTexture[4], samplePos);
      break;
    case 5:
      fragColor = color * texture(ourTexture[5], samplePos);
      break;
    case 6:
      fragColor = color * texture(ourTexture[6], samplePos);
      break;
    case 7:
      fragColor = color * texture(ourTexture[7], samplePos);
      break;
    case 8:
      fragColor = color * texture(ourTexture[8], samplePos);
      break;
    case 9:
      fragColor = color * texture(ourTexture[9], samplePos);
      break;
    case 10:
      fragColor = color * texture(ourTexture[10], samplePos);
      break;
    case 11:
      fragColor = color * texture(ourTexture[11], samplePos);
      break;
    case 12:
      fragColor = color * texture(ourTexture[12], samplePos);
      break;
    case 13:
      fragColor = color * texture(ourTexture[13], samplePos);
      break;
    case 14:
      fragColor = color * texture(ourTexture[14], samplePos);
      break;
    case 15:
      fragColor = color * texture(ourTexture[15], samplePos);
      break;
    case 16:
      fragColor = color * texture(ourTexture[16], samplePos);
      break;
    case 17:
      fragColor = color * texture(ourTexture[17], samplePos);
      break;
    case 18:
      fragColor = color * texture(ourTexture[18], samplePos);
      break;
    case 19:
      fragColor = color * texture(ourTexture[19], samplePos);
      break;
    case 20:
      fragColor = color * texture(ourTexture[20], samplePos);
      break;
    case 21:
      fragColor = color * texture(ourTexture[21], samplePos);
      break;
    case 22:
      fragColor = color * texture(ourTexture[22], samplePos);
      break;
    case 23:
      fragColor = color * texture(ourTexture[23], samplePos);
      break;
    case 24:
      fragColor = color * texture(ourTexture[24], samplePos);
      break;
    case 25:
      fragColor = color * texture(ourTexture[25], samplePos);
      break;
    case 26:
      fragColor = color * texture(ourTexture[26], samplePos);
      break;
    case 27:
      fragColor = color * texture(ourTexture[27], samplePos);
      break;
    case 28:
      fragColor = color * texture(ourTexture[28], samplePos);
      break;
    case 29:
      fragColor = color * texture(ourTexture[29], samplePos);
      break;
    case 30:
      fragColor = color * texture(ourTexture[30], samplePos);
      break;
    case 31:
      fragColor = color * texture(ourTexture[31], samplePos);
      break;
  }
}
