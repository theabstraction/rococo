struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

#pragma pack_matrix(row_major)

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

float4 main(PixelVertex p) : SV_TARGET
{
	float r = dot(p.uv, p.uv);
	float intensity = 0.2f * clamp(1 - r, 0, 1);
	float4 texel = float4(p.colour.xyz, intensity * p.colour.w);

	float range = length(p.cameraSpacePosition.xyz);
	float fogging = exp(range * ambience.fogConstant);

	texel.xyz *= fogging;
	texel.xyz * ambience.localLight.xyz;

	return float4(texel.xyz, texel.w);
	
}

