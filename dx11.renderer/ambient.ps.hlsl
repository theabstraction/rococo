struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal: NORMAL;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 cameraSpacePosition: TEXCOORD1;
	float4 worldPosition: TEXCOORD2;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
};

#pragma pack_matrix(row_major)

Texture2DArray g_materials: register(t6);
SamplerState txSampler;

TextureCube g_cubeMap: register(t3);


struct AmbientData
{
	float4 localLight;
	float fogConstant; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a;
	float b;
	float c;
	float4 eye;
};

cbuffer ambience: register(b0)
{
	AmbientData ambience;
}

cbuffer globalState: register(b1)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
};

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(txSampler, p.uv_material_and_gloss.xyz);
	texel.xyz = lerp(p.colour.xyz, texel.xyz, p.colour.w);

	float3 incident = normalize(p.worldPosition.xyz - eye.xyz);

	float3 reflectionVector = reflect(normalize(incident.xyz), normalize(p.normal.xyz));
	float4 reflectionColor = g_cubeMap.Sample(txSampler, reflectionVector);

	texel.xyz = lerp(texel.xyz, reflectionColor.xyz, p.uv_material_and_gloss.w);

	float range = length(p.cameraSpacePosition.xyz);	
	float fogging = exp( range * ambience.fogConstant);
	
	texel.xyz *= fogging;

	return texel * ambience.localLight;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
