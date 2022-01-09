#include "mplat.api.hlsl"

float4 main(GuiPixelVertex p) : SV_TARGET
{
	float4 fontTexel = tx_GlyphArray.Sample(glyphSampler, float3(p.base.xy, p.sd.y));
	return float4(p.colour.xyz, p.colour.w * fontTexel.x);
}
