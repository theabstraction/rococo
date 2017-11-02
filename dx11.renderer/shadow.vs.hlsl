struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float  materialIndex : TEXCOORD1;
};


struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

#pragma pack_matrix(row_major)

cbuffer depthRenderData
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

cbuffer perInstanceData
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

ScreenVertex main(ObjectVertex v)
{
	float4 instancePos = mul(instanceMatrix, v.position);

	ScreenVertex sv;
	sv.position = mul(worldToScreen, instancePos);
	sv.worldPosition = instancePos;
	return sv;
}