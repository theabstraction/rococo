#include "mplat.api.hlsl"

struct BaseVertexData
{
	float2 uv;
	float fontBlend; // 0 -> normal triangle, 1 -> modulate with font texture
};

struct SpriteVertexData
{
	float lerpBitmapToColour; // 1.0 -> use colour, 0.0 -> use bitmap texture
	float spriteIndex; // index the sprite texture in the texture array.
	float matIndex; // index the texture in the material array
	float spriteToMatLerpFactor; // 0 -> use textureIndex, 1 -> use matIndex, lerping in between
};

float4 main(GuiPixelVertex p) : SV_TARGET
{
	float spriteIndex = p.sd.y;
	float2 uv = p.base.xy;
	float4 spriteTexel = tx_BitmapSprite.Sample(spriteSampler, float3(uv,spriteIndex));
	float4 finalColour = p.colour * spriteTexel.w;
	return finalColour;
}
