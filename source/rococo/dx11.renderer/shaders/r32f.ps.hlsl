#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
	GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	
	float s = tx_SelectedTexture.Sample(matSampler, p.base.uv).x;
	float r = pow(abs(s), 60.0f);
	return float4(r, r, r, 1.0f);
}
