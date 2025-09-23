#version 330 core

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 uViewPos;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uObjectColor;

void main()
{
    vec3 norm = normalize(vNormal);
    float diff = max(dot(norm, normalize(-uLightDirection)), 0.0);
    vec3 diffuse = diff * uLightColor;
    vec3 ambient = 0.1 * uLightColor;
    vec3 color = (ambient + diffuse) * uObjectColor;
    FragColor = vec4(color, 1.0);
}
