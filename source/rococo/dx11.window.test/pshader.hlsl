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

Texture2D g_FontSprite;
SamplerState spriteSampler;

float4 main(PixelVertex p) : SV_TARGET
{
	float alpha = g_FontSprite.Sample(spriteSampler, float2(p.tx.u, p.tx.v)).x;
	return (1.0f - p.tx.fontBlend) * p.colour + float4(p.colour.x, p.colour.y, p.colour.z, alpha * p.colour.w) * p.tx.fontBlend;
}