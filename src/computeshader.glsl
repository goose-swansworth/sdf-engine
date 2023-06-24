#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;

const int MAX_ITERS = 100;
const float T_MAX = 50;
const float EPSILON = 0.01;

float sdfSphere(vec3 center, float radius, vec3 point) {
    return length(center - point) - radius;
}

float marchScene(vec3 r0, vec3 d) {
    float t = 0;
    float dist;
    for (int i = 0; i < MAX_ITERS; i++) {
        dist = abs(sdfSphere(vec3(0, 0, 2.0), 1.0, r0 + t * d));
        if (dist < EPSILON) {
            t += dist;
            return t;
        } else {
            t += dist;
            if (t > T_MAX) {
                return -1.0;
            }
        }
    }
    return -1.0;
}

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    ivec2 texel_coords = ivec2(gl_GlobalInvocationID.xy);
    
    ivec2 dims = imageSize(imgOutput);
    float x = -(float(texel_coords.x * 2 - dims.x) / dims.x); // transform to [-1, 1]
    float y = -(float(texel_coords.y * 2 - dims.y) / dims.y); // transform to [-1, 1]
    float fov = 90;
    vec3 camera_orgin = vec3(0.0, 0.0, -tan(fov / 2.0));
    
    vec3 r0 = vec3(x, y, 0.0);
    vec3 d = normalize(r0 - camera_orgin);

    float t = marchScene(r0, d);
    if (t > 0) {
        color.x = 1;
    }

	
    imageStore(imgOutput, texel_coords, color);
}