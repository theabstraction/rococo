#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float shadowDensity = GetShadowDensity(p.shadowPos);
	
	float4 texel = SampleMaterial(p.uv_material_and_gloss.xyz, p.colour);
	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);
	float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
	float3 lightToPixelDir = normalize(lightToPixelVec);

	float intensity = GetSpotlightIntensity(lightToPixelDir);
	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	float diffuse = GetDiffuse(p, lightToPixelVec, lightToPixelDir);
	float specular = GetSpecular(p, incident, lightToPixelDir);

	float I = (diffuse + specular) * intensity * clarity;

	texel.xyz *= I;

    return lerp(texel * light.colour, float4(0, 0, 0, 0), shadowDensity);
}
