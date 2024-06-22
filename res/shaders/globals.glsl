layout(set = 0, binding = 0) uniform CameraProperties {
    mat4 identity;
    mat4 view;
    mat4 proj;
    float pixelSize;
    float pixelsPerUnit;
} cameraProperties;
