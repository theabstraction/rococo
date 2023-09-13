#include "mplat.api.hlsl"

struct SkyVertex
{
	float4 pos : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

struct PixelData
{
	float4 colour : SV_Target;
};

PixelData main(SkyVertex input)
{
	PixelData output;
	output.colour = tx_cubeMap.Sample(envSampler, input.viewDir);
	return output;
}
