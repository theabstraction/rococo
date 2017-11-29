struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float2 materialIndexAndGloss  : TEXCOORD1;
};


struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

#pragma pack_matrix(row_major)

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

cbuffer InstanceData: register(b4)
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

struct DepthRenderDesc
{
	float4x4 worldToCamera;
	float4x4 worldToScreen;
	float4 eye;
	float4 direction;
	float4 right;
	float4 up;
	float nearPlane;
	float farPlane;
	float fov; // radians
	float time;
	float4 randoms;
};

cbuffer DepthRenderData:  register(b3)
{
	DepthRenderDesc d;
};

ScreenVertex main(ObjectVertex v)
{
	float4 instancePos = mul(instanceMatrix, v.position);

	ScreenVertex sv;
	sv.position = mul(d.worldToScreen, instancePos);
	sv.worldPosition = instancePos;
	return sv;
}