#include "mplat.types.hlsl"

struct ScreenVertex
{
	float4 pos : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

struct SkyVertex
{
	float3 pos : POSITION0;
};

cbuffer GlobalState: register(b0)
{
	GlobalState global;
}

ScreenVertex main(SkyVertex v)
{
	ScreenVertex output;

	float3 pos = v.pos - global.eye.xyz;
	output.pos = mul(global.worldToScreenMatrix, float4 (pos, 1.0f));
	output.viewDir = float3 (pos.x, pos.z, pos.y );

	return output;
}
