#version 330 core

// Vertex inputs supplied via VAO attribute bindings.
layout (location = 0) in vec3 aPosition; // Object-space vertex position
layout (location = 1) in vec3 aNormal;   // Object-space vertex normal

// Matrices provided by the CPU every frame.
uniform mat4 uModel;      // Object -> world transform
uniform mat4 uView;       // World -> camera transform
uniform mat4 uProjection; // Camera -> clip transform

// Data we hand off to the fragment shader for lighting.
out vec3 vNormal;  // World-space surface normal
out vec3 vFragPos; // World-space fragment position

void main()
{
    // Move the vertex from object space into world space so both stages agree.
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vFragPos = worldPos.xyz;

    // Transform normals with the inverse-transpose to handle non-uniform scale.
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    // Complete the usual MVP transform into clip space for rasterization.
    gl_Position = uProjection * uView * worldPos;
}
