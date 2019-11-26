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

	float3 centredMeshPos = v.pos - global.eye.xyz;
	float4 centredMeshPos4 = float4 (centredMeshPos.xyz, 1.0f);
	output.pos = mul(global.worldToScreenMatrix, centredMeshPos4);
	output.viewDir = mul(global.worldToCameraMatrix, centredMeshPos4).xyz;

	return output;
}
