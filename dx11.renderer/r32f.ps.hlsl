struct PixelVertex
{
	float4 position			: SV_POSITION;
	float3 base				: TEXCOORD0;		// xy give uv. 
	float4 sd				: TEXCOORD1;
	float4 colour			: COLOR;
};

Texture2D g_Texture: register(t4);	// Custom shaders all put texture in t4

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

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

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOSpriteWidth;
};

cbuffer GlobalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
}

cbuffer textureState: register(b5)
{
	TextureDescState state;
}

float4 main(PixelVertex p) : SV_TARGET
{
	float s = g_Texture.Sample(matSampler, p.base.xy).x;
	float r = pow(s, 60.0f);
	return float4(r, r, r, 1.0f);
}
