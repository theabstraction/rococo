#include "mplat.api.hlsl"

float4 main(GuiPixelVertexOpaque p_opaque) : SV_TARGET
{
	GuiPixelVertex p = ToGuiPixelVertex(p_opaque);
	
	float4 spriteTexel = tx_BitmapSprite.Sample(spriteSampler, float3(p.base.uv, p.sd.spriteIndex));
	float4 materialTexel = tx_materials.Sample(matSampler, float3(p.base.uv, p.sd.matIndex));
	float4 imageColour = lerp(spriteTexel, materialTexel, p.sd.spriteToMatLerpFactor);
	float fontDistance = tx_FontSprite.Sample(fontSampler, p.base.uv).x * 255.0;

	float q = max(10.0 - fontDistance, 0) / 10.0;
	
	p.colour.w = lerp(p.colour.w, q * p.colour.w, p.base.fontBlend);
	return lerp(imageColour, p.colour, p.sd.saturation);
}
