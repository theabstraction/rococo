struct PixelVertex
{
	float4 position : SV_POSITION;
	float4 emissiveColour : COLOR0;
	float4 diffuseColour : COLOR1; // w gives texture saturation
	float2 uv: TEXCOORD;
};

Texture2D g_Texture: register(t1);
SamplerState txSampler;

float4 main(PixelVertex p) : SV_TARGET
{
	float4 texel = g_Texture.Sample(txSampler, p.uv).xyzw;
	return lerp(p.emissiveColour, texel, p.diffuseColour.w);
}