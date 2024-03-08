#version 450

layout(location = 0) in uint inIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int textureIndex;

#include "../globals.glsl"

struct TextureData {
    vec3 pageIndexAndSize;
    vec4[4] uvs;
};

/**
 *  Uniform Buffers and Shader Storage Objects are double buffered by default and host visible, coherant and cached
 *  Use the following prefixes to change that behaviour.
 *  singleInst_, si_ -> Disables double buffering so buffer will only exist once
 *  deviceLocal_, dl_ -> Makes buffer device local, buffer is not host mapped and data needs to be staged into it.
 *  noCache_, nc_ -> Removes cached bit
 *  Prefixes can be chained and are delimited by "_" by default
 *  Because of that the delimter can't be used anywhere in the name itself
 *  Prefixes and the delimiter can all be changed by passign a modified ApplicationInfo struct to the Application constructor
 */

layout(std140, set = 1, binding = 0) readonly buffer si_TextureStore {
    TextureData textures[10240];
} textureStore;

layout(std430, set = 2, binding = 0) readonly buffer ObjectBuffer {
    vec4 positions[2048000];
    uint textureIDs[2048000];
    uint numObjects;
} objectBuffer;

const vec2 POSITIONS[4] = vec2[4] (
    vec2(-1.0f, 1.0),
    vec2(1.0f, 1.0f),
    vec2(-1.0f, -1.0f),
    vec2(1.0f, -1.0f)
);

void main() {
    int i = gl_InstanceIndex*10240 + (gl_VertexIndex / 6);
    vec3 position = objectBuffer.positions[i].xyz;
    TextureData textureData = textureStore.textures[objectBuffer.textureIDs[i]];

    float angle = radians(objectBuffer.positions[i].w);

    mat2 rotationMatrix = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    vec2 rotatedPosition = rotationMatrix * (POSITIONS[inIndex] * vec2(textureData.pageIndexAndSize.y, textureData.pageIndexAndSize.z));

    vec3 adjustedPosition = vec3(rotatedPosition + position.xy, position.z);

    gl_Position = cameraProperties.viewProj *  vec4(adjustedPosition, 1.0);
    fragColor = vec4(1.0f, 1.0f, 1.0f, float(i < objectBuffer.numObjects));
    fragTexCoord = textureData.uvs[inIndex].xy;
    textureIndex = int(textureData.pageIndexAndSize.x);
}