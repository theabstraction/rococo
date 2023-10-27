#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
	GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	float4 spriteTexel = tx_SelectedTexture.Sample(spriteSampler, float2(p.base.uv));
	return lerp(spriteTexel, p.colour, p.sd.saturation);
}
