struct PixelVertex
{
	float4 position : SV_POSITION;
	float4 colour : COLOR0; // w gives texture saturation
	float2 uv: TEXCOORD;
};

Texture2D g_Texture: register(t1);
SamplerState txSampler;

float4 main(PixelVertex p) : SV_TARGET
{
   float fogBlend = p.position.z * 0.01f;
	float4 texel = g_Texture.Sample(txSampler, p.uv).xyzw;
   float3 texel2 = lerp(texel.xyz * p.colour.xyz, p.colour.xyz, p.colour.w);
	float4 srcColour = float4(texel2.xyz, 1.0f);
   return lerp(srcColour, float4(1.0f, 1.0f, 1.0f, 1.0f), fogBlend);
}