struct PixelVertex
{
	float4 position			: SV_POSITION;
	float3 base				: TEXCOORD0;
	float4 sd				: TEXCOORD1;
	float4 colour			: COLOR;
};

Texture2D g_FontSprite: register(t0);
Texture2DArray g_BitmapSprite: register(t7);
Texture2DArray g_MaterialTextureArray: register(t6);

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOSpriteWidth;
};

cbuffer GlobalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
}

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

float4 main(PixelVertex p) : SV_TARGET
{
	SpriteVertexData svd;
	svd.lerpBitmapToColour = p.sd.x;
	svd.spriteIndex = p.sd.y;
	svd.matIndex = p.sd.z;
	svd.spriteToMatLerpFactor = p.sd.w;

	BaseVertexData base;
	base.uv = p.base.xy;
	base.fontBlend = p.base.z;

	float4 spriteTexel = g_BitmapSprite.Sample(spriteSampler, float3(base.uv.x, base.uv.y, svd.spriteIndex));
	float4 materialTexel = g_MaterialTextureArray.Sample(matSampler, float3(base.uv.x, base.uv.y, svd.matIndex));
	float4 imageColour = lerp(spriteTexel, materialTexel, svd.spriteToMatLerpFactor);
	float fontAlpha = g_FontSprite.Sample(fontSampler, base.uv).x;
	p.colour.w = lerp(p.colour.w, fontAlpha * p.colour.w, base.fontBlend);
	return lerp(imageColour, p.colour, svd.lerpBitmapToColour);
}
