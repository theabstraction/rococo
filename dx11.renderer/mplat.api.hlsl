#include "mplat.types.hlsl"

cbuffer GlobalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
}

cbuffer Spotlight: register(b1)
{
	Light light;
};

cbuffer AmbienceState: register(b2)
{
	AmbientData ambience;
}

cbuffer DepthRenderDesc : register(b3)
{
	DepthRenderDesc drd;
}

cbuffer InstanceState: register(b4)
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

Texture2D g_FontSprite: register(t0);
Texture2D g_ShadowMap: register(t2);
TextureCube g_cubeMap: register(t3);
Texture2DArray g_materials: register(t6);
Texture2DArray g_BitmapSprite: register(t7);

