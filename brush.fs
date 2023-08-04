#version 400

uniform float brush_shape;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main()
{
    vec2 uv = abs(fragTexCoord * 2 - 1);

    if (pow(uv.x, brush_shape) + pow(uv.y, brush_shape) > 1) discard;

    finalColor = fragColor;
}