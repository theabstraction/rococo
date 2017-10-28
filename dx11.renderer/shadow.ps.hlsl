struct PixelVertex
{
	float4 position : SV_POSITION;
};

cbuffer depthRenderData
{
	float4x4 worldToCamera;
	float4x4 worldToScreen;
	float3 eye;
	float3 direction;
	float3 right;
	float3 up;
	float nearPlane;
	float farPlane;
	float fov; // radians
	float unused;
};

float4 main(PixelVertex p) : SV_TARGET
{ 
	return float4(p.position.z, 0, 0, 1.0f);
}