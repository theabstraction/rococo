#include <mplat.api.hlsl>

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float4 texel = GetPointSpriteTexel(p.uv, p.colour);

	float clarity = GetClarityAcrossSpan(p.cameraSpacePosition);

	texel.xyz *= clarity;
	texel.xyz * ambience.localLight.xyz;

	return float4(texel.xyz, texel.w);
	
}

