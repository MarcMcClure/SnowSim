#version 330 core

// Interpolated values coming from the vertex shader.
in vec3 vNormal;  // World-space surface normal per fragment
in vec3 vFragPos; // World-space position (unused now, useful for specular/shadows later)

out vec4 FragColor;

// Uniforms supplied each draw call by the renderer.
uniform vec3 uViewPos;        // Camera position (reserved for future specular term)
uniform vec3 uLightDirection; // Direction the light travels across the scene
uniform vec3 uLightColor;     // RGB intensity of the light source
uniform vec3 uObjectColor;    // Base albedo of the rendered geometry

void main()
{
    // Ambient lighting component keeps surfaces from going completely dark.
    float ambientStrength = 0.25f;
    vec3 ambient = ambientStrength * uLightColor;

    // Diffuse lighting follows Lambert's law: depends on angle between normal and light.
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // No specular lighting component at this time

    vec3 color = (ambient + diffuse) * uObjectColor;
    FragColor = vec4(color, 1.0);
}
