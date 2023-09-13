#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p): SV_TARGET
{
	float4 texel = p.colour;
    float3 incident = ComputeEyeToWorldDirection(p);
	float clarity = GetClarity(p);
	
	texel.xyz *= clarity;

	return texel * ambience.localLight;
}

