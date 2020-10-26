#include "mplat.types.hlsl"

struct SkyVertex
{
	float4 pos : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

struct PixelData
{
	float4 colour : SV_Target;
};

SamplerState skySampler: register(s0);
TextureCube skyCube: register(t0);

PixelData main(SkyVertex input)
{
	PixelData output;
	output.colour = skyCube.Sample(skySampler, input.viewDir);
	return output;
}
