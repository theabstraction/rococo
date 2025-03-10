#include "mplat.api.hlsl"

struct GBufferSVTarget
{
    float4 colour : SV_TARGET;
};

GBufferSVTarget main(ObjectPixelVertex p)
{
    float4 texel = SampleMaterial(p);
//	float3 incident = normalize(p.worldPosition - GetEyePosition());
//	float3 normal = p.worldNormal.xyz;
//	float3 reflectionVector = normalize(reflect(incident, normal));
//	texel = ModulateWithEnvMap(texel, incident.xyz, normal, p.uv_material_and_gloss.w);
//	float clarity = GetClarity(p);
	// -> to do, develop the HLSL console log in mhost.app, hyperverse iteration is too slow
//	texel.xyz *= clarity;
		
//	float2 uv = p.uv_material_and_gloss.xy;
	
//	texel = ModulateWithEnvMap(texel, incident.xyz, normal, p.uv_material_and_gloss.w);
	
//	texel = float4(noiseX, noiseX, noiseX, 1.0f);
	
//	return float4(SignedToUnsignedV3(normal), 1.0f);
//	return float4(SignedToUnsignedV3(reflectionVector), 1.0f);

   // return texel;
	
    GBufferSVTarget t;
    t.colour = texel;
	return t;
}

