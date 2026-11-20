#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p): SV_TARGET
{
    float4 texel = SampleMaterial(p);
	float3 incident = normalize(p.worldPosition - GetEyePosition());
	float3 normal = GetBumpedWorldNormal(p);

	float clarity = GetClarity(p);
	// -> to do, develop the HLSL console log in mhost.app, hyperverse iteration is too slow
	texel.xyz *= clarity;
		
	float2 uv = p.uv_and_gloss.xy;
	
	texel = ModulateWithEnvMap(texel, incident.xyz, normal, p.uv_and_gloss.z);
	
	return texel * ambience.localLight;
}

