#include "mplat.api.hlsl"

// ---- Hash: pcg3d from Jarzynski & Olano, "Hash Functions for GPU Rendering" (JCGT 2020)
uint3 pcg3d(uint3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

// ---- Gradient: Perlin's 12 cube-edge directions (16 with repeats) chosen by the low 4 bits,
// as in his reference implementation for "Improving Noise" (SIGGRAPH 2002)
float GradDot(uint h, float3 p)
{
    h &= 15u;
    float u = (h < 8u) ? p.x : p.y;
    float v = (h < 4u) ? p.y : ((h == 12u || h == 14u) ? p.x : p.z);
    return ((h & 1u) ? -u : u) + ((h & 2u) ? -v : v);
}

float CornerNoise(int3 cell, float3 f, int3 o, uint seed)
{
    uint h = pcg3d(asuint(cell + o) + uint3(0, 0, seed)).x;
    return GradDot(h, f - float3(o));
}

// Range is roughly -1..1. Precision: keep |p| below ~1e4 (wrap UVs before scaling).
float GradientNoise3(float3 p, uint seed)
{
    float3 pf = floor(p);
    int3 c = int3(pf);
    float3 f = p - pf;
    float3 u = fade(f);

    float n000 = CornerNoise(c, f, int3(0, 0, 0), seed);
    float n100 = CornerNoise(c, f, int3(1, 0, 0), seed);
    float n010 = CornerNoise(c, f, int3(0, 1, 0), seed);
    float n110 = CornerNoise(c, f, int3(1, 1, 0), seed);
    float n001 = CornerNoise(c, f, int3(0, 0, 1), seed);
    float n101 = CornerNoise(c, f, int3(1, 0, 1), seed);
    float n011 = CornerNoise(c, f, int3(0, 1, 1), seed);
    float n111 = CornerNoise(c, f, int3(1, 1, 1), seed);

    return lerp(lerp(lerp(n000, n100, u.x), lerp(n010, n110, u.x), u.y),
                lerp(lerp(n001, n101, u.x), lerp(n011, n111, u.x), u.y), u.z);
}

// ---- 2D version: 4 corners instead of 8, for UV-space detail where z would always be 0
float GradDot2(uint h, float2 p)
{
    // 8 directions: axes and diagonals
    static const float2 g[8] =
    {
        float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1),
                                 float2(0.7071, 0.7071), float2(-0.7071, 0.7071),
                                 float2(0.7071, -0.7071), float2(-0.7071, -0.7071)
    };
    return dot(g[h & 7u], p);
}

float GradientNoise2(float2 p, uint seed)
{
    float2 pf = floor(p);
    int2 c = int2(pf);
    float2 f = p - pf;
    float2 u = f * f * f * (f * (f * 6 - 15) + 10);

    uint h00 = pcg3d(uint3(asuint(c), seed)).x;
    uint h10 = pcg3d(uint3(asuint(c + int2(1, 0)), seed)).x;
    uint h01 = pcg3d(uint3(asuint(c + int2(0, 1)), seed)).x;
    uint h11 = pcg3d(uint3(asuint(c + int2(1, 1)), seed)).x;

    return lerp(lerp(GradDot2(h00, f), GradDot2(h10, f - float2(1, 0)), u.x),
                lerp(GradDot2(h01, f - float2(0, 1)), GradDot2(h11, f - float2(1, 1)), u.x), u.y) * 1.4142;
}

// ---- Octaves with anti-aliasing: each octave fades out as its cells approach pixel size.
// Fixed loop count so ddx/ddy stay in uniform control flow.
#define NOISE_OCTAVES 5
float FBm2(float2 p, uint seed)
{
    float sum = 0, amp = 0.5, norm = 0;
    [unroll]
    for (int i = 0; i < NOISE_OCTAVES; ++i)
    {
        float2 w = fwidth(p); // cells per pixel at this octave
        float fadeOut = 1.0 - smoothstep(0.25, 0.5, max(w.x, w.y));
        sum += amp * fadeOut * GradientNoise2(p, seed + i);
        norm += amp;
        p *= 2.0;
        amp *= 0.5;
    }
    return sum / norm; // roughly -1..1
}

// ---- Fixed replacement for SampleMaterialWithNoise: modulate rgb only, keep alpha
float4 SampleMaterialWithNoise(ObjectPixelVertex v)
{
    float4 texel = SampleMaterialByVectors(v.uv_and_gloss.xy, v.textureIndices.x, v.colour);
    float n = FBm2(frac(v.uv_and_gloss.xy) * 64.0f, 0u); // wrap UVs first for precision
    texel.rgb *= 1.0f + 0.15f * n; // +/-15% brightness variation
    return texel;
}