#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
    GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	float4 fontTexel = tx_GlyphArray.Sample(glyphSampler, float3(p.base.uv, p.sd.spriteIndex));
	return float4(p.colour.xyz, p.colour.w * fontTexel.x);
}
