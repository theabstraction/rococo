#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

float4 main(PixelVertex p) : SV_TARGET
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

