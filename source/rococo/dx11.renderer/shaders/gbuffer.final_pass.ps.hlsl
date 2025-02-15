#include "mplat.api.hlsl"

struct FullScreenQuadPixelSpec
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(FullScreenQuadPixelSpec spec) : SV_TARGET
{
    return float4(spec.uv.x, spec.uv.y, 0.0f, 1.0f);
}
