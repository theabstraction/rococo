#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
    GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	float4 spriteTexel = tx_BitmapSprite.Sample(spriteSampler, float3(p.base.uv, p.sd.spriteIndex));
    return p.colour * spriteTexel.w;
}
