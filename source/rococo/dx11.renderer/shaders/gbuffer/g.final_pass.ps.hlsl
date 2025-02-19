#include "..\mplat.api.hlsl"

struct FullScreenQuadPixelSpec
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// normal element range is -1 to 1, so add + 1 and halve to normalize to [0,1]
float3 NormalizeSignedValues(float3 s)
{
    return 0.5f * (s + float3(1.0f, 1.0f, 1.0f));
}

float4 main(FullScreenQuadPixelSpec spec) : SV_TARGET
{
    //float4 txColour = tx_GBuffer_Colour.Sample(spriteSampler, spec.uv);
    //return txColour;
    
    
    float4 n = tx_GBuffer_Normal.Sample(spriteSampler, spec.uv);       
    return float4(NormalizeSignedValues(n.xyz), 1.0f);
}
