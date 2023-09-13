#include <mplat.api.hlsl>

struct BillboardVertex
{
	float3 position : POSITION;
	float4 colour: COLOR;
	float4 geometry: TEXCOORD;
};

BillboardVertex main(ParticleVertex input)
{
	BillboardVertex output;
	output.position = input.position;
	output.colour = input.colour;
	output.geometry = input.geometry;
	return output;
}
