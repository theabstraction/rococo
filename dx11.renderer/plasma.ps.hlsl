struct PixelVertex
{
	float4 position : SV_POSITION0;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

struct AmbientData
{
	float4 localLight;
	float fogConstant; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a;
	float b;
	float c;
	float4 eye;
};

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOSpriteWidth;
};

struct GlobalState
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
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

cbuffer ambience : register(b2)
{
	AmbientData ambience;
}

float4 main(PixelVertex p) : SV_TARGET
{
	float r = dot(p.uv, p.uv);
	float intensity = 0.2f * clamp(1 - r, 0, 1);
	return float4(p.colour.xyz, intensity * p.colour.w);
}
