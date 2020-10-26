#include "mplat.api.hlsl"

float4 main(LandPixelVertex p) : SV_TARGET
{
	float i = dot(p.normal.xyz, sunlight.direction.xyz);
	float4 texel = tx_materials.Sample(matSampler, float3(p.uv_material_and_gloss.xy, 2.0f));
	return float4(texel.xyz * i, 1.0f);
}