#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p): SV_TARGET
{
    float4 texel = SampleMaterial(p);
	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	texel.xyz *= clarity;
	return texel * ambience.localLight;
}

