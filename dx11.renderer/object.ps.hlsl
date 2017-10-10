struct PixelVertex
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL; 
	float2 uv: TEXCOORD;
   float4 worldPosition: TEXCOORD1;
};

cbuffer globalState
{
   float4x4 worldMatrixAndProj;
   float4x4 worldMatrix;
   float4 lightDir;
   float4 lightPos;
};

Texture2D g_Texture: register(t1);
SamplerState txSampler;

float4 main(PixelVertex p) : SV_TARGET
{
   // float fogBlend = p.position.w * 0.03f;
	float4 texel = g_Texture.Sample(txSampler, p.uv).xyzw;
   // float3 texel2 = lerp(texel.xyz * p.colour.xyz, p.colour.xyz, p.colour.w);
   // float4 srcColour = float4(texel2.xyz, 1.0f);

   float3 lightDelta = p.worldPosition.xyz - lightPos.xyz;

   float g = dot(lightDelta, lightDelta);

   float intensity = 0.1f + 10.0f / (g + 0.1f);

   float3 lightDir = normalize(lightDelta);
   float f = - dot(lightDir,  normalize( p.normal.xyz ));

   float mag = clamp(f * intensity, 0, 1.0f);

   return float4 (texel.x * mag, texel.y * mag, texel.z * mag, 1.0f );
}