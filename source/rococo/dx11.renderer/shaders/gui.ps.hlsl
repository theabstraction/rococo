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
	SpriteVertexData svd;
	svd.lerpBitmapToColour = p.sd.x;
	svd.spriteIndex = p.sd.y;
	svd.matIndex = p.sd.z;
	svd.spriteToMatLerpFactor = p.sd.w;

	BaseVertexData base;
	base.uv = p.base.xy;
	base.fontBlend = p.base.z;

	float4 spriteTexel = tx_BitmapSprite.Sample(spriteSampler, float3(base.uv.x, base.uv.y, svd.spriteIndex));
	float4 materialTexel = tx_materials.Sample(matSampler, float3(base.uv.x, base.uv.y, svd.matIndex));
	float4 imageColour = lerp(spriteTexel, materialTexel, svd.spriteToMatLerpFactor);
	float fontDistance = tx_FontSprite.Sample(fontSampler, base.uv).x * 255.0;

	float q = max(10.0 - fontDistance, 0) / 10.0;

	p.colour.w = lerp(p.colour.w, q * p.colour.w, base.fontBlend);
	return lerp(imageColour, p.colour, svd.lerpBitmapToColour);
}
