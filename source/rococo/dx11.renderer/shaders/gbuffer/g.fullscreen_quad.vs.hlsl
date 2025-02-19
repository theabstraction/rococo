#include <mplat.api.hlsl>

struct FullScreenQuadVertex
{
    float2 normalizedCoordinates : POSITION;
    float2 uv : TEXCOORD0;
};

struct OutputVertex
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

OutputVertex main(FullScreenQuadVertex v)
{	
    OutputVertex output;
    output.pos = float4(v.normalizedCoordinates.xy, 0.0f, 1.0f);
    output.uv = v.uv;
    return output;
}