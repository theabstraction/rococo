#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
    GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	
	float spriteIndex = p.sd.spriteIndex;
    float4 spriteTexel = tx_BitmapSprite.Sample(spriteSampler, float3(p.base.uv, spriteIndex));
	float4 finalColour = p.colour * spriteTexel.w;
	return finalColour;
}
