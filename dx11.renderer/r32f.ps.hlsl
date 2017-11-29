#include "mplat.api.hlsl"

struct PixelVertex
{
	float4 position			: SV_POSITION;
	float3 base				: TEXCOORD0;		// xy give uv. 
	float4 sd				: TEXCOORD1;
	float4 colour			: COLOR;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float s = tx_SelectedTexture.Sample(matSampler, p.base.xy).x;
	float r = pow(s, 60.0f);
	return float4(r, r, r, 1.0f);
}
