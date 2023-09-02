#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColour;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec4 vColour;
layout (location = 1) out vec2 vTexCoord;

uniform mat4 mat2DOrthogonalTransform;

void main()
{
	gl_Position = mat2DOrthogonalTransform * vec4(aPos, 0.0, 1.0);
	vColour = aColour;
	vTexCoord = aTexCoord;
};
