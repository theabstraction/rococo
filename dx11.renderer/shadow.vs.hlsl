struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;	
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
	float2 uv: TEXCOORD;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
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
	return sv;
}