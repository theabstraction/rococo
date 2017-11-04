struct PixelVertex
{
	float4 position			: SV_POSITION;
	float3 base				: TEXCOORD0;		// xy give uv. 
	float4 sd				: TEXCOORD1;
	float4 colour			: COLOR;
};

Texture2D g_Texture: register(t4);	// Custom shaders all put texture in t4
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

cbuffer textureState: register(b7)
{
	TextureDescState state;
}

float4 main(PixelVertex p) : SV_TARGET
{
	float s = g_Texture.Sample(spriteSampler, p.base.xy).x;
	float r = pow(s, 60.0f);
	return float4(r, r, r, 1.0f);
}
