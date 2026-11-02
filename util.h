#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <psptypes.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef ScePspFVector3 V3;

static inline V3 v3(float x, float y, float z) { V3 v = {x, y, z}; return v; }
static inline V3 v3add(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 v3sub(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 v3scale(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float v3dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 v3cross(V3 a, V3 b) {
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static inline float v3len(V3 a) { return sqrtf(v3dot(a, a)); }
static inline V3 v3norm(V3 a) {
    float l = v3len(a);
    if (l < 1e-6f) return v3(0, 0, 0);
    return v3scale(a, 1.0f / l);
}
static inline float v3distxz(V3 a, V3 b) {
    float dx = a.x - b.x, dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float smoothstep(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static inline float randf(void) { return (float)rand() / (float)RAND_MAX; }
static inline int randi(int n) { return n > 0 ? (int)(randf() * n) % n : 0; }
static inline float deg2rad(float d) { return d * (float)M_PI / 180.0f; }

/* Matrices 4x4 colonne-majeur (convention GL / sceGum). */
typedef struct { float m[16]; } M4;

static inline M4 m4identity(void) {
    M4 r; memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}
static inline M4 m4mul(M4 a, M4 b) {
    M4 r;
    for (int c = 0; c < 4; c++)
        for (int i = 0; i < 4; i++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a.m[k * 4 + i] * b.m[c * 4 + k];
            r.m[c * 4 + i] = s;
        }
    return r;
}
/* clip = m * v (v homogene, w=1) */
static inline void m4transform(M4 m, V3 v, float out[4]) {
    float x[4] = {v.x, v.y, v.z, 1.0f};
    for (int i = 0; i < 4; i++) {
        float s = 0;
        for (int k = 0; k < 4; k++) s += m.m[k * 4 + i] * x[k];
        out[i] = s;
    }
}
static inline M4 m4perspective(float fovy_deg, float aspect, float near, float far) {
    M4 r; memset(&r, 0, sizeof(r));
    float f = 1.0f / tanf(deg2rad(fovy_deg) * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * far * near / (near - far);
    return r;
}
static inline M4 m4ortho(float l, float r, float b, float t, float n, float f) {
    M4 m = m4identity();
    m.m[0] = 2.0f / (r - l);
    m.m[5] = 2.0f / (t - b);
    m.m[10] = -2.0f / (f - n);
    m.m[12] = -(r + l) / (r - l);
    m.m[13] = -(t + b) / (t - b);
    m.m[14] = -(f + n) / (f - n);
    return m;
}
static inline M4 m4lookat(V3 eye, V3 center, V3 up) {
    V3 f = v3norm(v3sub(center, eye));
    V3 s = v3norm(v3cross(f, up));
    V3 u = v3cross(s, f);
    M4 m = m4identity();
    m.m[0] = s.x; m.m[4] = s.y; m.m[8] = s.z;
    m.m[1] = u.x; m.m[5] = u.y; m.m[9] = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -v3dot(s, eye);
    m.m[13] = -v3dot(u, eye);
    m.m[14] = v3dot(f, eye);
    return m;
}

/* Couleurs 0xAABBGGRR */
#define RGBA(r, g, b, a) ((((unsigned)(a)) << 24) | (((unsigned)(b)) << 16) | (((unsigned)(g)) << 8) | ((unsigned)(r)))
#define RGB(r, g, b) RGBA(r, g, b, 255)

#endif
