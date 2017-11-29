#include <mplat.api.hlsl>

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

float4 main(PixelVertex p) : SV_TARGET
{
	// float clarity = GetClarity(p.cameraSpacePosition.xyz);
	return GetPointSpriteTexel(p.uv, p.colour);
}
