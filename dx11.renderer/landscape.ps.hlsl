#include "mplat.api.hlsl"

static float4 sunlightDirection = float4(0.707106781f, 0, 0.707106781f, 0.0f);

float4 main(ObjectPixelVertex p) : SV_TARGET
{
//	float i = dot(p.normal.xyz, sunlightDirection.xyz);
	float i = 1.0f;
	float4 texel = tx_materials.Sample(matSampler, float3(p.uv_material_and_gloss.xy, 2.0f));
	return float4(texel.xyz * i, 1.0f);
}