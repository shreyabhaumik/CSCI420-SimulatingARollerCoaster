#version 150

in vec2 tc;

out vec4 c;

uniform sampler2D textureImage;

void main()
{
  // compute the final pixel color
  // by looking up into the texture map
  c = texture(textureImage, tc);
}