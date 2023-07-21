#version 430 core

#include "lighting.glsl"

struct Camera {
    vec3 position;
    vec3 fowards;
    vec3 up;
    vec3 right;
};

struct Box {
    vec3 dims;
    vec3 position;
    vec4 color;
};

struct Material {
    vec3 diffuse;
    float bump;
    vec3 normal;
};

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;
layout(std430, binding = 1) readonly buffer sceneData {
    Box boxes[];
};
layout(rgba32f, binding = 2) readonly uniform image2D lumpedTexture;

uniform Camera viewer;
uniform bool AA;
uniform vec3 light;
uniform float FLOOR;
uniform vec4 FLOOR1;
uniform vec4 FLOOR2;
uniform vec4 sky_color;
uniform int num_boxes;

#define EPSILON  1e-3
#define MAX_ITERS 128
#define T_MIN 1
#define T_MAX 40
#define TEX_SIZE 1024
#define PI 3.1415926538

#define DIFFUSE_OFFSET 0
#define NORMAL_OFFSET 1

#define SHADOW_HARDNESS 8

const float inf = uintBitsToFloat(0x7F800000);
const vec3 zeros = vec3(0);
const vec3 up = vec3(0, 1, 0);
const vec4 white = vec4(1.0);

Material sampleMaterial(int texture_index, float s, float t) {
    Material material;
    material.diffuse = imageLoad(lumpedTexture, ivec2(floor(TEX_SIZE * (s + DIFFUSE_OFFSET)), floor(TEX_SIZE * (t + texture_index)))).rgb;
    material.normal = imageLoad(lumpedTexture, ivec2(floor(TEX_SIZE * (s + NORMAL_OFFSET)), floor(TEX_SIZE * (t + texture_index)))).rgb;
    material.normal = 2 * material.normal - white.xyz;
    return material;
}

vec2 stMapSphere(vec3 center, vec3 p) {
    vec3 d = normalize(center - p);
    vec2 txCoords;
    txCoords.s = 0.5 + atan(d.z, d.x) / (2 * PI);
    txCoords.t = 0.5 + asin(d.y) / PI;
    return txCoords;
}

vec2 txCoordsBox(int index, vec3 p) {
    Box box = boxes[index];
    vec3 t = p - box.position;
    vec2 txCoords;
    if (abs(t.x) >= box.dims.x) {
        txCoords.s = (box.dims.z - t.z) / (2 * box.dims.z);
        txCoords.t = (box.dims.y - t.y) / (2 * box.dims.y);
    }
    if (abs(t.y) >= box.dims.y) {
        txCoords.s = (box.dims.z - t.z) / (2 * box.dims.z);
        txCoords.t = (box.dims.x - t.x) / (2 * box.dims.x);
    }
    if (abs(t.z) >= box.dims.z) {
        txCoords.s = (box.dims.x - t.x) / (2 * box.dims.x);
        txCoords.t = (box.dims.y - t.y) / (2 * box.dims.y);
    }
    return txCoords;

}



vec3 sdTranslate(vec3 trans, vec3 point) {
    return point - trans;
}

float sdSphere(float radius, vec3 point) {
    return length(point) - radius;
} 

float floor_intersection(vec3 r0, vec3 d) {
    if (d.y != 0) {
        float t = -(float(FLOOR) + r0.y) / d.y;
        if (t > 0) {
            return t;
        } 
    }
    return -1.0;
}

float sdBox( vec3 b, vec3 p) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

vec2 sdfScene(vec3 point) {
    float smallest;
    float dist;
    int i = -1;
    smallest = sdSphere(2, point - vec3(0, 1, 0));
    for (int j = 0; j < num_boxes; j++) {
        dist = sdBox(boxes[j].dims, sdTranslate(boxes[j].position, point));
        if (dist < smallest) {
            smallest = dist;
            i = j;
        }
    }
    return vec2(smallest, i);
}

vec2 marchScene(vec3 r0, vec3 d) {
    float t = 1;
    vec2 res;
    for (int i = 0; i < MAX_ITERS && t <= T_MAX; i++) {
        res = sdfScene(r0 + t * d);
        if (abs(res.x) < EPSILON * t) {
            return vec2(t, res.y);
        } else {
            t += res.x;
        }
    }
    return vec2(-1.0, -1.0);
}

vec3 sceneNormal(vec3 p) {
    vec3 dx = vec3(EPSILON, 0.0, 0.0);
    vec3 dy = vec3(0.0, EPSILON, 0.0);
    vec3 dz = vec3(0.0, 0.0, EPSILON);
    vec3 v1 = vec3(
        sdfScene(p + dx).x,
        sdfScene(p + dy).x,
        sdfScene(p + dz).x
    );
    vec3 v2 = vec3(
        sdfScene(p - dx).x,
        sdfScene(p - dy).x,
        sdfScene(p - dz).x
    );
    return normalize(v1 - v2);
}

// vec4 diffuse(vec3 L, vec3 N, float ka, float kd, vec4 material) {
//    return  ka * material + kd * max(dot(L, N), 0) * material;
// }

vec4 check(vec3 point) {
    vec2 q = floor(point.xz);
    if (mod(q.x + q.y, 2.0) == 0) {
        return FLOOR1;
    }
    return FLOOR2;
}


vec2 trace(vec3 r0, vec3 d) {
    vec2 res = marchScene(r0, d);
    if (res.x < 0) {
        res.x = floor_intersection(r0, d);
        if (res.x > 0) {
            res.y = -2.0;
        }
    }
    if (res.x > T_MAX) {
        res.x = -1.0;
    }
    return res;
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
}

float softShadow(in vec3 r0, in vec3 d, float tinit, float k) {
    float sFactor = 1.0;
    float t = tinit;
    for (int i = 0; i < MAX_ITERS / 4 && t < T_MAX; i++) {
        float dist = sdfScene(r0 + t * d).x;
        sFactor = min(sFactor, clamp(k * dist / t, 0.0, 1.0));
        t += dist;
        if (dist < EPSILON) {
            return 0.2;
        }
    }
    sFactor = clamp(sFactor, 0.0, 1.0);
    sFactor = sFactor*sFactor*(3.0 - 2.0 * sFactor);
    return map(sFactor, 0.0, 1.0, 0.2, 1.0);
}

vec4 shadeFloor(vec3 L, vec3 P) {
    return softShadow(P, L, EPSILON, SHADOW_HARDNESS) * diffuse(L, up, 0.2, 0.8, check(P));
}

vec4 shadeBox(int index, vec3 L, vec3 P) {
    vec2 txCoords = txCoordsBox(index, P);
    Box box = boxes[index];
    vec3 dims = box.dims;
    Material mat = sampleMaterial(index % 2 + 1, txCoords.s, txCoords.t);
    vec3 N = sceneNormal(P);
    vec3 T, B;
    vec3 t = P - box.position;
    if (t.x > dims.x || t.x < -dims.x) {
        T = vec3(0, 0, 1);
    } else if (t.y > dims.y || t.y < -dims.y) {
        T = vec3(1, 0, 0);
    } else {
        T = vec3(-1, 0, 0);
    }
    B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    N = TBN * mat.normal;
    return diffuse(L, N, 0.2, 0.8, vec4(mat.diffuse, 1.0));
}

vec4 shadeSphere(vec3 L, vec3 p, vec3 I) {
    vec2 coords = stMapSphere(vec3(0), p);
    Material material = sampleMaterial(0, coords.s, coords.t);
    vec4 diff_col = vec4(material.diffuse.rgb * 0.75, 1.0);
    // Normal mapping
    vec3 N = normalize(p - vec3(0, 1, 0));
    float theta = atan(p.y / p.x);
    vec3 T = vec3(-sin(theta), 0, cos(theta));
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    N = TBN * material.normal;
    // --------------
    vec4 c1 = diffuse(L, N, 0.2, 0.8, diff_col);
    vec4 c2;
    // Refection
    vec3 d = reflect(I, N);
    vec2 res = trace(p + EPSILON * d, d);
    float t = res.x;
    if (t > 0) {
        int i = int(res.y);
        if (i == -2) {
            c2 = shadeFloor(L, p + t * d);
        } else {
            c2 = shadeBox(i, L, p + t * d);
        }
    } else {
        c2 = mix(sky_color, white, I.y);
    }
    return c1 + 0.2 * c2;
}



vec4 shade(int obj_index, vec3 p, vec3 I) {
    vec3 L = normalize(light - p);
    if (obj_index == -1) {
        return shadeSphere(L, p, I);
    }
    if (obj_index == -2) {
        return shadeFloor(L, p);
    }
    if (obj_index > -1) {
        return shadeBox(obj_index, L, p);
    }
}






vec4 render(vec3 r0, vec3 d) {
    vec2 res = trace(r0, d);
    float t = res.x;
    int i = int(res.y);
    vec4 color;
    if (t > 0) {
        vec3 p = r0 + t * d;
        color = shade(i, r0 + t * d, d);
        return color;
    }
    return mix(sky_color, white, d.y);

}

const vec2 s4[] = {{0.375f, 0.125f}, {0.875f, 0.375f}, {0.625f, 0.875f}, {0.125f, 0.625f}};

vec3 get_ray_dir(vec2 c) {
    return normalize(viewer.fowards + c.x * viewer.right + c.y * viewer.up);
}

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    ivec2 texel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = imageSize(imgOutput);
    float aspect = float(dims.x) / dims.y;
    float x = ((float(texel_coords.x) * 2 - dims.x) / dims.x); // transform to [-h/w , h/w]
    float y = ((float(texel_coords.y) * 2 - dims.y) / dims.x); // transform to [-1, 1]
    vec2 pixel_center = vec2(x, y);
    vec3 r0 = viewer.position;
    vec3 d, d1, d2;
    vec2 offset;
    float pixel_width = 2.0 / dims.x;
    if (AA) {
        for (int i = 0; i < 4; i++) {
            offset = pixel_center + s4[i] * pixel_width - vec2(0.5, 0.5) * pixel_width;
            d = normalize(viewer.fowards + offset.x * viewer.right + offset.y * viewer.up);
            color += 0.25 * render(r0, d);
        }
    } else {
        d = get_ray_dir(pixel_center);
        color = render(r0, d);
    }
    ivec2 mid = dims / 2;
    if (mid.x - 1 < texel_coords.x && texel_coords.x < mid.x + 1 && mid.y - 1 < texel_coords.y && texel_coords.y < mid.y + 1) {
        color = mix(white, color, 0.75);
    }

    imageStore(imgOutput, texel_coords, color);
}