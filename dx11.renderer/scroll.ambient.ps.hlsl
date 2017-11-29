#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD; // In scroll rendering, gloss controls solid colour to font lerp
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

float4 GetFontPixel(float3 uv_blend, float4 vertexColour)
{
	float fontIntensity = lerp(1.0f, g_FontSprite.Sample(fontSampler, uv_blend.xy).x, uv_blend.z);
	return float4(vertexColour.xyz, fontIntensity);
}

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = GetFontPixel(p.uv_material_and_gloss.xyw, p.colour);

	float range = length(p.cameraSpacePosition.xyz);
	float fogging = exp(range * ambience.fogConstant);

	texel.xyz *= fogging;

	return float4(texel.xyz * ambience.localLight.xyz, texel.w * p.colour.w);
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
