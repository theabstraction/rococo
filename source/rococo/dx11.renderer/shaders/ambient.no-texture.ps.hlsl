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

float4 main(PixelVertex p): SV_TARGET
{
	float4 texel = p.colour;
	float3 incident = normalize(p.worldPosition.xyz - global.eye.xyz);

	float clarity = GetClarity(p.cameraSpacePosition.xyz);
	
	texel.xyz *= clarity;

	return texel * ambience.localLight;
}

