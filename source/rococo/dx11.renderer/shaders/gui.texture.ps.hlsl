#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
    GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	float4 texel = tx_SelectedTexture.Sample(spriteSampler, p.base.uv);
	return float4(4.0f * texel.xyz, 1.0f);
}
