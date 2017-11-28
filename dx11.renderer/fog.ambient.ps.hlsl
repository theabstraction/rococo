struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cmeraSpacePosition: TEXCOORD3;
	float4 shadowPosition: TEXCOORD2;
	float3 worldPosition: TEXCOORD1;
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

cbuffer globalState: register(b1)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
	float4 viewDir;
	float4 aspect;
};

Texture2D g_ShadowMap: register(t2);

SamplerState shadowSamplerLookup
{
	// sampler state
	Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	AddressU = MIRROR;
	AddressV = MIRROR;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float r = dot(p.uv, p.uv);
	float intensity = 0.2f * clamp(1 - r, 0, 1);
	float4 texel = float4(p.colour.xyz, intensity * p.colour.w);

	float range = length(p.worldPosition.xyz);
	float fogging = exp(range * ambience.fogConstant);

	texel.xyz *= fogging;

	return texel * ambience.localLight;
}

