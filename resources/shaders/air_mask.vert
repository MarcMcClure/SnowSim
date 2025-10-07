#version 330 core

layout (location = 0) in vec3 aPosition; // Vertex position of the grid quad
layout (location = 1) in vec2 aTexCoord; // Texture coordinate covering the full grid

uniform mat4 uModel;      // Places the grid in world space
uniform mat4 uView;       // Camera view matrix
uniform mat4 uProjection; // Camera projection matrix

out vec2 vTexCoord;       // Forwarded to the fragment shader for sampling

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}

