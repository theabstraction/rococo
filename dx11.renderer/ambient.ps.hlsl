#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = SampleMaterial(p.uv_material_and_gloss.xyz, p.colour.w);
	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);
	texel = ModulateWithEnvMap(texel, incident, p.normal.xyz, p.uv_material_and_gloss.w);

	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	
	texel.xyz *= clarity;

	return texel * ambience.localLight;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
