#include "mplat.types.hlsl"

struct SkyVertexOutput
{
	float4 pos : SV_POSITION;
	float3 viewDir : TEXCOORD0;
};

struct SkyVertexInput
{
	float3 pos : POSITION0;
};

SkyVertexOutput main(SkyVertexInput v)
{
	SkyVertexOutput output;

	float3 pos = v.pos;
	float4x4 proj = global.cameraToScreenMatrix;
	float4x4 w = global.worldToCameraMatrix;
	float4x4 wSky = {
		w._11, w._12, w._13, 0,
		w._21, w._22, w._23, 0,
		w._31, w._32, w._33, 0,
		w._41, w._42, w._43, 1
	};

	float4x4 wSkyToScreen = mul(proj, wSky);

	output.pos = mul(wSkyToScreen, float4 (pos, 1.0f));
	output.viewDir = float3 (pos.x, pos.z, pos.y );

	return output;
}
