#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);

	if (!IsInShadow(p.shadowPos))
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float3 lightToPixelDir = normalize(lightToPixelVec);

		float intensity = GetSpotlightIntensity(lightToPixelDir);
		float clarity = GetClarity(p.cameraSpacePosition.xyz);
		float diffuse = GetDiffuse(p, lightToPixelVec, lightToPixelDir);
		float I = diffuse * intensity * clarity;

		return float4 (texel.xyz * I * light.colour.xyz, texel.w);
	}
	else
	{
		return float4(0, 0, 0, texel.w);
	}
}

