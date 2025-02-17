#include "mplat.api.hlsl"

struct FullScreenQuadPixelSpec
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(FullScreenQuadPixelSpec spec) : SV_TARGET
{
    float4 txColour = tx_GBuffer_Colour.Sample(spriteSampler, spec.uv);
    return txColour;
}
