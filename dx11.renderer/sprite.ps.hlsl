struct PixelVertex
{
	float4 position : SV_POSITION;
	float3 tx : TEXCOORD0;
};

Texture2DArray g_TextureArray: register(t0);
SamplerState spriteSampler;

float4 main(PixelVertex p) : SV_TARGET
{
   return g_TextureArray.Sample(spriteSampler, p.tx);
}