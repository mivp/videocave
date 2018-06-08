#version 400
layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

out vec2 TexCoord0;

void main(void)
{
   gl_Position = vec4(inPosition, 0, 1);
   TexCoord0 = inTexCoord;
}
