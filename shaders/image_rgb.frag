#version 400

in vec2 TexCoord0;

layout( location = 0 ) out vec4 FragColor;

uniform sampler2D uTexY;

void main()
{
	vec4 color = texture(uTexY, TexCoord0);
	FragColor = color;
}
