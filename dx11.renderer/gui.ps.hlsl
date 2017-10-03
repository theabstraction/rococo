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

Texture2D g_GuiSprite: register(t0);
Texture2D g_BitmapSprite: register(t1);
SamplerState spriteSampler;

float4 main(PixelVertex p) : SV_TARGET
{
	float fontAlpha = g_GuiSprite.Sample(spriteSampler, float2(p.tx.u, p.tx.v)).x;
	float4 spritePixel = g_BitmapSprite.Sample(spriteSampler, float2(p.tx.u, p.tx.v)).xyzw;
	float saturatedAlpha = lerp(p.colour.w, fontAlpha * p.colour.w, p.tx.fontBlend);
	p.colour.w = saturatedAlpha;
	return lerp(spritePixel, p.colour, p.tx.saturation);
}
