#version 330 core

layout (location = 0) in vec4 vColour;
layout (location = 1) in vec2 vTexCoord;

uniform sampler2D uCoreTexture;

out vec4 FragColor;

void main()
{
	FragColor = texture(uCoreTexture, vTexCoord) * vColour;
};
