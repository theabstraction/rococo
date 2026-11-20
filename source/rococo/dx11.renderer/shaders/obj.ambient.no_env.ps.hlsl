#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p): SV_TARGET
{
    float4 texel = SampleMaterial(p);	
	float3 N  = GetBumpedWorldNormal(p);
	float3 Ng = normalize(p.worldNormal.xyz);
	float hemi = 0.5f + 0.5f * dot(N, WORLD_UP);           // sky vs ground
	float cavity = saturate(dot(N, Ng));             // crevices darken
	cavity *= cavity;
	float ambientScale = lerp(0.6f, 1.0f, hemi) * cavity;
	return texel * ambience.localLight * ambientScale * GetClarity(p);
}

