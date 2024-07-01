#version 450

layout(location = 0) in vec2 inPosition;

#include "../globals.glsl"

void main() {
    gl_PointSize = 1.0f;
    gl_Position = cameraProperties.proj * cameraProperties.view * vec4(inPosition, 0.0f, 1.0);
}