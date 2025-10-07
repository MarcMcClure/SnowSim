#version 330 core

in float vDensity;

out vec4 FragColor;

uniform float uDensityMax; // Density mapped to red (linear scale)

vec3 density_to_color(float d)
{
    float t = clamp(d / uDensityMax, 0.0, 1.0);
    if (t < 0.5)
    {
        float k = t / 0.5;
        return mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0), k); // Blue -> Yellow
    }
    else
    {
        float k = (t - 0.5) / 0.5;
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), k); // Yellow -> Red
    }
}

void main()
{
    vec3 color = density_to_color(max(vDensity, 0.0));
    FragColor = vec4(color, 1.0);
}

