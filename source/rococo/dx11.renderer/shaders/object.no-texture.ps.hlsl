#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float shadowDensity = GetShadowDensity(p);
	float4 texel = p.colour;
    float3 incident = ComputeEyeToWorldDirection(p);
    float3 lightToPixelVec = GetLightToWorldPosition(p);
	float3 lightToPixelDir = normalize(lightToPixelVec);

	float intensity = GetSpotlightIntensity(lightToPixelDir);
	float diffuse = GetDiffuse(p, lightToPixelVec, lightToPixelDir);
	float clarity = GetClarity(p);
	float specular = GetSpecular(p, incident, lightToPixelDir);
	float I = (diffuse + specular) * intensity * clarity;

	texel.xyz *= I;
	texel.xyz *= light.colour.xyz;

    return lerp(float4(texel.xyz, 1.0f), float4(0, 0, 0, 0), shadowDensity);
}
