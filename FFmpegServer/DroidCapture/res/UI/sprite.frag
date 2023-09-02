#version 410 core

layout (location = 0) in vec4 vColour;
layout (location = 1) in vec2 vTexCoord;

layout (location = 0) out vec4 FragColor;

uniform sampler2D uCoreTexture;

void main()
{
	FragColor = texture(uCoreTexture, vTexCoord) * vColour;
};
