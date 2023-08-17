#include "mplat.api.hlsl"

float4 main(ObjectPixelVertex p) : SV_TARGET
{
	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);

	float shadowDensity = GetShadowDensity(p.shadowPos);
	float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
	float3 lightToPixelDir = normalize(lightToPixelVec);

	float intensity = GetSpotlightIntensity(lightToPixelDir);
	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	float diffuse = GetDiffuse(p, lightToPixelVec, lightToPixelDir);
	float I = diffuse * intensity * clarity;

	float3 c1 = float3 (texel.xyz * I * light.colour.xyz);
	float3 b = float3(0,0,0);

	return float4(lerp(c1, b, shadowDensity), texel.w * shadowDensity);
}

