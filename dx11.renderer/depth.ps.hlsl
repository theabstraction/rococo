struct PixelVertex
{
   float4 position : SV_POSITION;
   float4 emissiveColour : COLOR0;
   float4 diffuseColour : COLOR1; // w gives texture saturation
   float3 uv: TEXCOORD;
};

Texture2D g_Texture: register(t1);
SamplerState txSampler;

float4 main(PixelVertex p) : SV_TARGET
{
   float z = 1.0f - p.position.z;
   float4 texel = g_Texture.Sample(txSampler, p.uv.xy).xyzw;
   return lerp(float4(z, z, z, 1.0f), texel, 0.01f);
}