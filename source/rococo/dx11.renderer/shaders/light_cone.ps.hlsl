#include "mplat.api.hlsl"

struct ConeVertex
{
	float4 position : SV_POSITION0;
	float4 colour: COLOR0;
	float2 coneData: TEXCOORD;
};

float4 main(ConeVertex p) : SV_TARGET
{
	// For light cone vertices only p.position and p.colour are defined. Everything else is zero
	// The light cone is rendered as a cross section triangle parallel to the screen

	float distToApex = p.coneData.x;
	float radius = p.coneData.y;

	float intensity = (1.0f - sqrt(abs(radius))) * (1.0f - sqrt(distToApex));
	float4 finalColour = float4 ( p.colour.xyz, intensity);

	return finalColour;
}