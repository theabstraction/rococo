#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	if (!IsInShadow(p.shadowPos))
	{
		float4 texel = SampleMaterial(p.uv_material_and_gloss.xyz, p.colour);
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

		return float4 (texel.xyz, 1.0f);
	}
	else
	{
		return float4(0,0,0,1.0f);
	}
}
