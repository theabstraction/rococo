#include "mplat.api.hlsl"

float4 main(GuiPixelVertex p) : SV_TARGET
{
	float s = tx_SelectedTexture.Sample(matSampler, p.base.xy).x;
	float r = pow(s, 60.0f);
	return float4(r, r, r, 1.0f);
}
