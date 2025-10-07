#version 330 core

in vec2 vTexCoord; // Texture coordinate coming from the vertex shader

uniform sampler2D uMaskTexture; // Air-mask texture (R8: 0 = ground, 1 = air)
uniform vec3 uAirColor;         // Color used for air cells
uniform vec3 uGroundColor;      // Color used for ground cells

out vec4 FragColor;             // Final color written to the framebuffer

void main()
{
    float mask = texture(uMaskTexture, vTexCoord).r; // Sample mask value (0-1)
    vec3 color = mix(uGroundColor, uAirColor, mask); // Interpolate between ground and air colors
    FragColor = vec4(color, 1.0);
}

