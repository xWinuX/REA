#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler[48];

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat int textureIndex;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = texture(texSampler[textureIndex], fragTexCoord) * fragColor;
    if (color.a == 0) {
        discard;
    }
    outColor = color;
}