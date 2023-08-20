#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	float shadowDensity = GetShadowDensity(p);
	float4 texel = SampleMaterial(p);
	
	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);
	
	// We dont apply the environment here, because by definition the environment is lit by ambient light only
	
	float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
	float3 lightToPixelDir = normalize(lightToPixelVec);

	float intensity = GetSpotlightIntensity(lightToPixelDir);
	float diffuse = GetDiffuse(p, lightToPixelVec, lightToPixelDir);
	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	float specular = GetSpecular(p, incident, lightToPixelDir);
	float I = (diffuse + specular) * intensity * clarity;

	texel.xyz *= I;
	texel.xyz *= light.colour.xyz;

	return  lerp(float4 (texel.xyz, 1.0f), float4(0.0f, 0.0f, 0.0f, 0.0f), shadowDensity);
}
