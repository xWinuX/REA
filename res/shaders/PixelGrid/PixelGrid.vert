#version 450

layout(location = 0) in uint inIndex;

layout(location = 0) out vec2 fragPosition;

#include "../globals.glsl"

const vec2 POSITIONS[4] = vec2[4] (
    vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f)
);

void main() {
    vec2 position = POSITIONS[inIndex];
    gl_Position = cameraProperties.viewProj * vec4(position, -7.0f, 1.0);
    fragPosition = position;
}