#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p): SV_TARGET
{
	float4 texel = SampleMaterial(p.uv_material_and_gloss.xyz, p.colour);
	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);
	float3 normal = p.worldNormal.xyz;
	texel = ModulateWithEnvMap(texel, incident.xyz, normal, p.uv_material_and_gloss.w);

	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	
	texel.xyz *= clarity;
	
//	return float4(SignedToUnsignedV3(normal), 1.0f);

	return texel * ambience.localLight;
}

