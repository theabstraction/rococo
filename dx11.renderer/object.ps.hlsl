struct PixelVertex
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour : COLOR0;
	float4 diffuseColour : COLOR1;
};

Texture2D g_FontSprite;
SamplerState spriteSampler;

float4 main(PixelVertex p) : SV_TARGET
{
	return p.emissiveColour;
}