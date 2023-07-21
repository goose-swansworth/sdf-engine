#if !defined(FNC_LIGHTING) && !defined(lighting)
#define FNC_LIGHTING

vec4 diffuse(vec3 L, vec3 N, float ka, float kd, vec4 material) {
   return  ka * material + kd * max(dot(L, N), 0) * material;
}

#endif