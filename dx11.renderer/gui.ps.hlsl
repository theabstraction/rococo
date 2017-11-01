struct Tx
{
	float u;
	float v;
	float saturation;
	float fontBlend;
};

struct PixelVertex
{
	float4 position : SV_POSITION;
	float4 colour : COLOR;
	Tx tx : TEXCOORD0;
};

Texture2D g_FontSprite: register(t0);
Texture2D g_BitmapSprite: register(t1);
SamplerState spriteSampler;

struct TextureDescState
{
	float width;
	float height;
	float inverseWidth;
	float inverseHeight;
	float redActive;
	float greenActive;
	float blueActive;
	float alphaActive;
};

cbuffer depthRenderData: register(b7)
{
	TextureDescState txDesc;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float fontAlpha = g_FontSprite.Sample(spriteSampler, float2(p.tx.u, p.tx.v)).x;
	float4 spritePixel = g_BitmapSprite.Sample(spriteSampler, float2(p.tx.u, p.tx.v));

	spritePixel.w = txDesc.alphaActive * spritePixel.w + (1.0f - txDesc.alphaActive);

	float saturatedAlpha = lerp(p.colour.w, fontAlpha * p.colour.w, p.tx.fontBlend);
	p.colour.w = saturatedAlpha;
	return lerp(spritePixel, p.colour, p.tx.saturation);
}
