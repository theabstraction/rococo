#include <mplat.api.hlsl>

float4 main(ObjectPixelVertex p) : SV_TARGET
{
    float shadowDensity = GetShadowDensity(p);
	
	float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
	float R2 = dot(lightToPixelVec, lightToPixelVec);

    float4 texel = GetPointSpriteTexel(p.uv_material_and_gloss.xy, p.colour);

	float3 lightToPixelDir = normalize(lightToPixelVec);

	float intensity = GetSpotlightIntensity(lightToPixelDir);

	float clarity = GetClarity(p.cameraSpacePosition.xyz);

	float diffuse = pow(R2, light.attenuationRate);
	float I = diffuse * intensity * clarity;

	texel.xyz *= I;
	texel.xyz *= light.colour.xyz;

	return lerp(float4 (texel.xyz, 1.0f), float4(0.0f, 0.0f, 0.0f, 0.0f), shadowDensity);
}

