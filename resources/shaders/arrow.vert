#version 330 core

layout (location = 0) in vec2 aPosition;    // Local arrow vertex (pointing along +X)
layout (location = 1) in vec2 iCenter;      // Arrow center in XY plane
layout (location = 2) in vec2 iDirection;   // Wind direction vector (not normalized)
layout (location = 3) in float iLength;     // Arrow length already scaled to world units
layout (location = 4) in float iDensity;    // Snow density for coloring

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uPlaneZ;          // Z offset so arrows sit just above the mask
uniform float uArrowHalfWidth;  // Half-width of the arrow shaft in world units

out float vDensity;

void main()
{
    vec2 dir = iDirection;
    float mag = length(dir);
    vec2 dirNorm = (mag > 1e-5) ? dir / mag : vec2(1.0, 0.0);
    vec2 perp = vec2(-dirNorm.y, dirNorm.x);

    vec2 local = vec2(aPosition.x * iLength, aPosition.y * uArrowHalfWidth);
    vec2 worldXY = iCenter + dirNorm * local.x + perp * local.y;
    vec3 worldPos = vec3(worldXY, uPlaneZ);

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
    vDensity = iDensity;
}

