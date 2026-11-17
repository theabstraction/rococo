#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float3 uv_and_gloss: TEXCOORD; // In scroll rendering, gloss controls solid colour to font lerp
	float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
    float2 textureIndices : TEXCOORD5;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

float4 per_pixel_lighting(PixelVertex p)
{
    float3 uvw = float3(p.uv_and_gloss.xy, p.textureIndices.x);
	float4 texel = GetFontPixel(uvw, p.colour);
	float clarity = GetClarityAcrossSpan(p.cameraSpacePosition.xyz);

	texel.xyz *= clarity;
	texel.xyz *= ambience.localLight.xyz;

	return float4(texel.xyz, texel.w * p.colour.w);
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
