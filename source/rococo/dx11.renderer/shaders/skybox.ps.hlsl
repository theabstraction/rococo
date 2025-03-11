#include "mplat.api.hlsl"

struct SkyVertex
{
	float4 pos : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

struct PixelData
{
	float4 colour : SV_Target;
	float depth : SV_Depth;
};

PixelData main(SkyVertex input)
{
	PixelData output;
	output.colour = tx_cubeMap.Sample(envSampler, input.viewDir);
	output.depth = 1.0f;
	return output;
}
